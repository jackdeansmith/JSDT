/* Implementation of the classes defined in jstp_streams.hpp */

#include "jstp_streams.hpp"
#include "jstp_segment.hpp"

#include <string>
using std::string;

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

//The JSTP stream is obviously a multithreaded component, these globals are
//needed in order to manage the state of these threads.
bool threads_running;

//Constructor for the jstp_stream on the client side
jstp_stream::jstp_stream(jstp_connector& connector, double probability_loss):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE, probability_loss){
    
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

    //TODO use the real numbers we got
    init(42, 42);
}

//Constructor for JSTP stream on the server side
jstp_stream::jstp_stream(jstp_acceptor& acceptor, double probability_loss):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE, probability_loss){

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

    //TODO wait for normal ack back
    init(42, 42);       //TODO make this real
}

//Function which initalizes all variables and starts threads for both
//constructors
void jstp_stream::init(uint32_t init_seq, uint32_t init_ack){
    
    //Start the threads, make sure this is the last thing init does
    threads_running = true;
    sender_thread = thread(&jstp_stream::sender_main, this);
    receiver_thread = thread(&jstp_stream::receiver_main, this);
}

//Destructor
jstp_stream::~jstp_stream(){
    //First set the thread running variable to false
    threads_running = false;

    //The receiver has it's own timeout loop, but the sender will need to be
    //notified. This will cause it to execute another loop of it's programming
    //and then return.
    sender_condition_var.notify_one();

    //Now we need to join both threads
    sender_thread.join();
    receiver_thread.join();
    cout << "Threads exited sucessfully" << endl;
}

//Sender thread main function
void jstp_stream::sender_main(){
    //Sender only runs while the threads are running, obviously
    while(threads_running){

        //The first thing we do is wait for someone to notify us that we have a
        //job to do.
        unique_lock<mutex> l(sender_notify_lock);
        sender_condition_var.wait(l);

        //Now we need to figure out if we have any data to send, to do this we
        //need exclusive acess to the send buffer.
    
    }
}

//Receiver thread main function
void jstp_stream::receiver_main(){
    //Receiver only runs while the threads are running, duh
    while(threads_running){

        //First thing we do is try to get a segment out of the socket, we only
        //wait at most one timout interval because we probably have other things
        //to do at that point even if we don't get a segment.
        jstp_segment incoming_seg;
        timeval tv;
        tv.tv_sec = TIMEOUT_SECONDS;
        tv.tv_usec = TIMEOUT_USECS;
        bool got_something = stream_sock.recv(incoming_seg, true, tv);

        //Now we need to do some stuff if we did indeed get something
        if(got_something){
        
        }
    
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

    copy(v.begin(), v.end(), back_inserter(send_buffer));
    send_buffer_mutex.unlock();
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
