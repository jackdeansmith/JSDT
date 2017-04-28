/* Implementation of the classes defined in jstp_streams.hpp */

#include "jstp_streams.hpp"
#include "jstp_segment.hpp"

#include <string>
using std::string;
#include <iterator>
using std::back_inserter;

#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

//Just for testing, TODO remove
#include <iostream>
using std::cout; using std::endl;
#include <thread>
using std::thread;
#include <mutex>
using std::mutex; using std::unique_lock;
#include <atomic>
using std::atomic;
#include <vector>
using std::vector;
#include <functional>
#include <chrono>
#include <algorithm>
using std::min;

//Function which "Randomly" choses an initial sequence number. For now lets just
//use 42.
uint32_t chose_isn(){
    return 42;
}

//Constructor, only thing we need to do for the connector class
jstp_connector::jstp_connector(string h, uint16_t p): hostname(h), port(p){}

//Constructor for the jstp acceptor, also the only thing we need to do for this
//class, the default destructor should do just fine.
jstp_acceptor::jstp_acceptor(uint16_t portno):
    acceptor_socket(jstp_segment::MAX_SEGMENT_SIZE){
    
    //Bind the socket to the specified port
    acceptor_socket.bind_local(portno);
}

//Constructor for the jstp_stream on the client side
jstp_stream::jstp_stream(jstp_connector& connector, double probability_loss, 
                         size_t w):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE, 0), 
    window_limit(w){
    
    //Bind the stream socket to any local port
    stream_sock.bind_local_any();

    //Set the peer specified by the connector
    stream_sock.set_peer(connector.hostname, connector.port);

    //Now we chose an initial sequence number
    uint32_t our_isn = chose_isn();

    //Now we need to send a syn segment and send it
    jstp_segment syn_seg;
    syn_seg.set_syn_flag();
    syn_seg.set_sequence(our_isn);
    stream_sock.send(syn_seg);

    //Nice! We need to receive a synack now. TODO add timeout to udp socket recv
    //so that we can address the loss of the syn or synack segemnt
    jstp_segment synack_seg;
    stream_sock.recv(synack_seg);

    //The server will have used a different ephemeral port to send that synack,
    //switch to the other socket. TODO update peer method?
    sockaddr_in server_stream_addr = stream_sock.get_last_addr();
    stream_sock.set_peer(server_stream_addr);

    //The synack should contain the servers initial sequence number
    uint32_t server_isn = synack_seg.get_sequence();

    stream_sock.set_loss_probability(probability_loss);
    //TODO use the real numbers we got
    init(our_isn + 1, server_isn + 1);
}

//Constructor for JSTP stream on the server side
jstp_stream::jstp_stream(jstp_acceptor& acceptor, double probability_loss,
                         size_t w):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE, 0),
    window_limit(w){

    //First, lets wait for a syn segment to come in
    jstp_segment syn_seg; 
    while(!syn_seg.get_syn_flag()){
        acceptor.acceptor_socket.recv(syn_seg); 
    }

    //Now that we got a syn segment, we know the clients isn
    uint32_t other_isn = syn_seg.get_sequence();

    //Now we can bind our socket and set our peer
    stream_sock.bind_local_any();
    stream_sock.set_peer(acceptor.acceptor_socket.get_last_addr());

    //Time to chose our own initial sequence numebr
    uint32_t our_isn = chose_isn();

    //We now need to craft a synack to send back
    jstp_segment synack_seg;
    synack_seg.set_syn_flag();
    synack_seg.set_ack_flag();
    synack_seg.set_ack(other_isn + 1);
    synack_seg.set_sequence(our_isn);

    //Send the synack back
    stream_sock.send(synack_seg);

    stream_sock.set_loss_probability(probability_loss);

    //TODO wait for normal ack back
    init(our_isn + 1, other_isn + 1);       //TODO make this real
}

//Function which initalizes all variables and starts threads for both
//constructors
void jstp_stream::init(uint32_t init_seq, uint32_t init_ack){

    //Set the initial sequence and ack numbers
    sender_base_sequence = init_seq;
    self_ack_number.store(init_ack);

    self_rwnd = BUFF_CAPACITY;
    other_rwnd = BUFF_CAPACITY;
    
    //Start the threads, make sure this is the last thing init does
    running.store(true);
    closing.store(false);
    terminating.store(false);
    sender_thread = thread(&jstp_stream::sender_main, this);
    receiver_thread = thread(&jstp_stream::receiver_main, this);
}

//Destructor
jstp_stream::~jstp_stream(){
    closing.store(true);

    //Now we need to join both threads
    sender_thread.join();
    receiver_thread.join();
}

//The thread for the sender function
void jstp_stream::sender_main(){

    //How many times we will try to send a terminate message before quitting our
    //of frustration
    int termination_tries = 5;

    //The main sender loop runs as long as the running var is set
    bool nap = true;
    while(running.load()){

        //At the top of every loop, there is an option to nap, this halts the
        //thread untill another part of the program notifies it. After every one
        //of these pauses, we set nap back to false so that it must be
        //explicitly turned on in the loop in order to catch on the next
        //iteration.
        if(nap){
            unique_lock<mutex> l(sender_notify_lock);
            sender_condition_var.wait(l);
            nap = false;
        }

        //Next, we dicide what to do depending on if the program is currently
        //terminating or not.
        if(!terminating){
            //In the nonterminating case, we need access to the send buffer
            send_buffer_mutex.lock();

            //Update the flushed codition variable when we are compleetly
            //cleared
            if(send_buffer.size() == 0){
                flushed.notify_all(); 
            }

            //Our first check should be to see if we are closing...
            if(closing.load()){
                //... if we are, then we should do a bit of cleanup before
                //entering the terminating state.
                send_buffer.clear(); 
                self_exit_number.store(sender_base_sequence + offset);
                terminating.store(true);
                send_buffer_mutex.unlock();
                continue;
            }

            //Do some simple math to get the length of the longest payload we
            //are legally allowd to send at this very instant.
            size_t buffered_data = send_buffer.size() - offset;
            size_t flow_limit = other_rwnd.load() - offset;
            size_t wndlim = window_limit - offset; 
            flow_limit = min(flow_limit, wndlim);
            size_t payload_size = min(flow_limit, buffered_data);
            payload_size = min(payload_size, jstp_segment::MAX_PAYLOAD_SIZE);

            //If we dont have a payload and we arent being forced to send...
            if(!(payload_size > 0) && !force_send.load()){
                //... then we skip the rest of the loop and nap
                nap = true; 
                send_buffer_mutex.unlock();
                continue;
            }

            jstp_segment outgoing_seg;

            //Set all the headers appropriatly
            if(payload_size > 0){
                outgoing_seg.set_sequence(sender_base_sequence + offset);
            }
            else{
                outgoing_seg.set_sequence(0);
            }
            outgoing_seg.set_ack(self_ack_number.load());
            outgoing_seg.set_ack_flag();
            outgoing_seg.set_window(self_rwnd.load());

            //Create the payload for the segment
            vector<uint8_t> outgoing_paylaod;
            outgoing_paylaod.reserve(payload_size);
            copy(send_buffer.begin() + offset, 
                 send_buffer.begin() + offset + payload_size, 
                 back_inserter(outgoing_paylaod));

            //Attach the paylaod to the segment
            outgoing_seg.set_payload(outgoing_paylaod);

            //Send the segment
            stream_sock.send(outgoing_seg);

            //Advance the offset by the specified ammount
            offset += payload_size;
            
            //If the length of the segment was nonzero...
            if(payload_size != 0){
                data_on_wire.store(true); 
            }

            //Turn off the force send flag
            force_send.store(false);

            cout_mutex.lock();
            cout << "Sending this segment:" << endl;
            cout << outgoing_seg.header_str() << endl;
            cout_mutex.unlock();
            send_buffer_mutex.unlock();
        }

        //The situation in which we are terminating
        else{
            if(termination_tries == 0){
                running.store(false); 
            }
            jstp_segment seg;
            seg.set_sequence(self_exit_number.load());
            seg.set_ack_flag();
            seg.set_exit_flag();
            stream_sock.send(seg);
            termination_tries--;
            nap = true;    
        }
    }

    //TODO remove this printout
    cout << "Sender quit" << endl;
}

//Receiver thread main function
void jstp_stream::receiver_main(){
    //Receiver only runs while the threads are running, duh
    while(running){

        //First thing we do is try to get a segment out of the socket, we only
        //wait at most one timout interval because we probably have other things
        //to do at that point even if we don't get a segment.
        jstp_segment incoming_seg;
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT_USECS;
        bool got_segment = stream_sock.recv(incoming_seg, true, tv);

        //In this block we process whatever segment we received
        if(got_segment){
            
            //If the incoming segment carries an exit flag...
            if(incoming_seg.get_exit_flag()){
                //First and foremost, make sure we are closing down our own
                //connection when this happens.
                closing.store(true);

                //Store the sequence number they are sending us, they won't quit
                //until they see us send it back.
                peer_exit_number.store(incoming_seg.get_sequence());

                //If they sent us our own closing sequence number, and we are
                //already in the termination state, then we are clear to exit.
                if(incoming_seg.get_ack() == self_exit_number.load() &&
                   terminating.load()){
                    running.store(false); 
                }
            }

            //If the incoming segment doesn't have an exit flag then we know if
            //can carry some ammount of data and or other informatin we care
            //about.
            else{
            
                //TODO commentary
                other_rwnd.store(incoming_seg.get_window());
                send_buffer_mutex.lock();
                size_t new_acked_bytes = incoming_seg.get_ack() 
                                         - sender_base_sequence;
                sender_base_sequence += new_acked_bytes;
                offset -= new_acked_bytes;
                send_buffer.erase(send_buffer.begin(), 
                                  send_buffer.begin() + new_acked_bytes);
                if(offset == 0){
                    data_on_wire.store(false); 
                }
                send_buffer_mutex.unlock();
                //If the number of new acked bytes was nonzero...
                if(new_acked_bytes != 0){
                    //... that means we got a new ack. Our timeout timepoint should
                    //be adjusted.
                    last_new_ack = std::chrono::monotonic_clock::now(); 
                }

                //If it was the segment we expected
                if(incoming_seg.get_sequence() == self_ack_number.load()){

                    //The first thing we need to check is if we have room to buffer
                    //it. Lock the recv buffer
                    recv_buffer_mutex.lock();

                    //If there is space...
                    size_t available_space = BUFF_CAPACITY - recv_buffer.size();
                    if(available_space > incoming_seg.get_length()){

                        //We need to update the sequence number we expect
                        uint32_t new_expected = self_ack_number.load() + 
                                                incoming_seg.get_length();
                        self_ack_number.store(new_expected);


                        //We need to reduce the size of our rwnd
                        self_rwnd -= incoming_seg.get_length();

                        //Finally, we should copy the data into our recv buffer
                        copy(incoming_seg.payload_begin(), 
                             incoming_seg.payload_end(),
                             back_inserter(recv_buffer));

                        force_send.store(true);

                    }
                    recv_buffer_mutex.unlock();
                }
            }
        }
        //Now, we do all the regular tasks which happen even if we didn't get a
        //segment.

        //Figure out what time it is now and how long it has been since the last
        //timeout.
        std::chrono::monotonic_clock::time_point now = 
                                          std::chrono::monotonic_clock::now();
        size_t diff = std::chrono::duration_cast<std::chrono::microseconds>
                      (now - last_new_ack).count();

        //If the difference is over the constant threshold and there is some
        //ammount of data on the wire which is unacked...
        if(diff > jstp_stream::TIMEOUT_USECS && data_on_wire.load()){
            //... then wind back the sender window.
            send_buffer_mutex.lock(); 
            offset = 0;
            data_on_wire.store(false);
            send_buffer_mutex.unlock(); 
            force_send.store(true);
            cout << "Timeout event" << endl;
        }

        //Finally, the last thing we do is wake the sender thread. This
        //guarentees that it gets woken up at least once per timeout interval
        //and at least once per packet recvd.
        sender_condition_var.notify_one();
    }
}

//Send and recv methods, relatively simple in retrospect
bool jstp_stream::send(const vector<uint8_t>& v){
    //Aquire the send buffer mutex
    send_buffer_mutex.lock();

    //Figure out how much space is left in the buffer
    size_t available = BUFF_CAPACITY - send_buffer.size();

    //If there isn't enough space, then report back false
    if(available < v.size()){
        send_buffer_mutex.unlock();
        return false; 
    }

    //Put the data in the buffer
    copy(v.begin(), v.end(), back_inserter(send_buffer));
    send_buffer_mutex.unlock();

    //Signal the sender that something needs to be sent
    sender_condition_var.notify_one();

    //Finally, wait for the send buffer to be fully flushed before returning
    std::unique_lock<mutex> l(flush_lock);
    flushed.wait(l);

    return true;
}

vector<uint8_t> jstp_stream::recv(){
    recv_buffer_mutex.lock();
    vector<uint8_t> out;
    out.reserve(recv_buffer.size());
    copy(recv_buffer.begin(), recv_buffer.end(), back_inserter(out));
    recv_buffer.clear();
    recv_buffer_mutex.unlock();
    return out;
}
