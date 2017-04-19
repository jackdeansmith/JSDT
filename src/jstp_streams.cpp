/* Implementation of the classes defined in jstp_streams.hpp */

#include "jstp_streams.hpp"
#include "jstp_segment.hpp"

#include <string>
using std::string;

#include <cstdint>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

//Just for testing, TODO remove
#include <iostream>
using std::cout; using std::endl;
#include <thread>
using std::thread;
#include <atomic>
using std::atomic;
#include <functional>

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
    
    cout << "Attempting to create acceptor" << endl;
    //Bind the socket to the specified port
    acceptor_socket.bind_local(portno);
}

//Constructor for the jstp_stream on the client side
jstp_stream::jstp_stream(jstp_connector& connector):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE){
    
    //Bind the stream socket to any local port
    stream_sock.bind_local_any();

    //Set the peer specified by the connector
    stream_sock.set_peer(connector.hostname, connector.port);

    //Now we chose an initial sequence number
    uint32_t initial_sequence_number = chose_isn();
    //TODO assign internal variables as needed

    //Now we need to send a syn segment and send it
    jstp_segment syn_seg;
    syn_seg.set_syn_flag();
    syn_seg.set_sequence(initial_sequence_number);
    stream_sock.send(syn_seg);

    //Nice! We need to receive a synack now. TODO add timeout to udp socket recv
    //so that we can address the loss of the syn or synack segemnt
    jstp_segment synack_seg;
    stream_sock.recv(synack_seg);

    //The server will have used a different ephemeral port to send that synack,
    //switch to the other socket. TODO update peer method?
    sockaddr_in server_stream_addr = stream_sock.get_last_addr();
    stream_sock.set_peer(server_stream_addr);

    //Now that segment should contain the servers initial sequence number, lets
    //grab it TODO do something with it
    cout << "Server ISN: " << synack_seg.get_sequence() << endl;

    //We should be good now. 
    //TODO change to the actual vlaues
    init(0,0);
}

//Constructor for JSTP stream on the server side
jstp_stream::jstp_stream(jstp_acceptor& acceptor):
    stream_sock(jstp_segment::MAX_SEGMENT_SIZE){

    //First, lets wait for a syn segment to come in
    jstp_segment syn_seg; 
    while(!syn_seg.get_syn_flag()){
        acceptor.acceptor_socket.recv(syn_seg); 
    }

    //Now that we got a syn segment, we know the clients isn
    //TODO set internal state based on this
    uint32_t other_isn = syn_seg.get_sequence();
    cout << "Received a syn, the ISN is: " << syn_seg.get_sequence() << endl;

    //Now we can bind our socket and set our peer
    stream_sock.bind_local_any();
    stream_sock.set_peer(acceptor.acceptor_socket.get_last_addr());

    //Time to chose our own initial sequence numebr
    //TODO update internal state
    uint32_t our_isn = chose_isn();

    //We now need to craft a synack to send back
    jstp_segment synack_seg;
    synack_seg.set_syn_flag();
    synack_seg.set_ack_flag();
    synack_seg.set_ack(other_isn + 1);
    synack_seg.set_sequence(our_isn);

    //Send the synack back
    stream_sock.send(synack_seg);

    //TODO wait for a normal ack?

    //TODO change to the actual vlaues
    init(0,0);
}

//Destructor
jstp_stream::~jstp_stream(){
    cout << "Set running to false!" << endl;
    //Stop the running variable, this will cause each of the worker threads to
    //do their own cleanup and then return.
    running = false;

    //Join both the thread
    sender_thread.join();
    receiver_thread.join();
    cout << "Both threads exited!" << endl;
}

void sender(atomic<bool>& running){
    cout << "I'm the sender, I'm starting!" << endl;
    while(running){
    
    }
    cout << "I'm the sender thread and I just got told to stop" << endl;
}

void receiver(atomic<bool>& running){
    cout << "I'm the receiver, I'm starting!" << endl;
    while(running){
    
    }
    cout << "I'm the receiver thread and I just got told to stop" << endl;
}

//Init function, takes the sequence number and the ack number we should kick
//things off with
void jstp_stream::init(uint32_t seq, uint32_t ack){
    //First, lets create the socket pair which we use to interact with the
    //application layer.
    cout << "Attempting to create a socketpair" << endl;
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);
    cout << "Socketpair created" << endl;

    //Now, lets set the sequence and ack numbers appropriatly
    base_seq_num = seq;
    ack_num = ack;

    //And now for the big show, we start up the threads!
    running = true;
    sender_thread = thread(sender, std::ref(running));
    receiver_thread = thread(receiver, std::ref(running));
}


//Get data from the socket pair and put it in the send buffer. Focus on making
//this function fast.
void jstp_stream::load_app_data(){
    //Get exclusive access to the send buffer
    send_buffer_mutex.lock();

    //Figure out how much more we can cram in the send buffer
    size_t available = BUFF_CAPACITY - send_buff.size();

    //Try to read that much into the load buffer
    size_t num_read = read(sockpair[1], load_buff, available);

    //Copy the data we read (if any) into the send buffer deque
    copy(load_buff, load_buff + num_read, back_inserter(send_buff));

    //Now let other people use the send buffer
    send_buffer_mutex.unlock();
}

