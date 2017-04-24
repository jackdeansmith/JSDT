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

    //Now use init to start the threads, because we already burned out isn on
    //the syn packet, incriment our sequence by one. Similarly for the server
    //ack number because we ack the next byte we expect.
    init(our_isn + 1, server_isn + 1);
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

    //TODO wait for a normal ack?

    //Start the threads, similarly to the above constructor.
    init(our_isn + 1, other_isn + 1);
}

//Destructor
jstp_stream::~jstp_stream(){
    cout << "Set running to false!" << endl;
    //Stop the running variable, this will cause each of the worker threads to
    //do their own cleanup and then return.
    threads_running = false;

    //Join both the thread
    sender_thread.join();
    receiver_thread.join();
    loader_thread.join();
    cout << "Worker threads exited!" << endl;
}

//Put user data into the sock pair to be sent
size_t jstp_stream::send(void* buffer, size_t len){
    return write(sockpair[0], buffer, len);    
}

//Take user data out of the sock pair
size_t jstp_stream::recv(void* buffer, size_t len){ 
    return read(sockpair[0], buffer, len);
}

void jstp_stream::sender(){
    while(threads_running){
        //First, load app data
        cout << "Sender thread at top of loop" << endl;

        //Figure out how much data we are allowed to send
        size_t allowed_to_send;

        /* size_t count = min(jstp_segment::MAX_PAYLOAD_SIZE, allowed_to_send); */
        jstp_segment seg;
        vector<uint8_t> payload; 

    }
    //TODO closing protocol
}

void jstp_stream::receiver(){
    while(threads_running){
        jstp_segment seg; 
        stream_sock.recv(seg);      //TODO, needs a timeout
        if(seg.get_ack_flag()){
            ack_num = seg.get_ack() + seg.get_length();
        }

        //Copy the data out to our user, TODO what if this fails somehow?
        const vector<uint8_t> v = seg.get_payload();
        write(sockpair[1], v.data(), v.size());

        //TODO deal with the close flag
    }
    //TODO closing protocol
}

//Thread which simply loads data into the send buffer as is appropriate
void jstp_stream::loader(){
    while(threads_running){

        //First, we wait around for a while to see if there is some data for us
        //to send. Every .5 sec we timeout so that the thread can exit if we
        //need it to.
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(sockpair[1], &readset);
        int flag = select(sockpair[1] + 1, &readset, nullptr, nullptr, &tv);

        //If flag is 0, we waited our whole timeout and nothing happened, that
        //means we just try again.
        if(flag == 0){
            continue;        
        }

        //Once we get to this point, we know that we have data that needs to be
        //sent.
        cout << "loader thread: I found some data to put in the buffer" << endl;
    
        //Get exclusive access to the send buffer
        send_buffer_mutex.lock();

        //Figure out how much more we can cram in the send buffer
        size_t available = BUFF_CAPACITY - send_buff.size();

        //Try to read that much into the load buffer
        int num_read = read(sockpair[1], load_buff, available);
        cout << "Loaded " << num_read << " bytes of data." << endl;

        //Copy the data we read (if any) into the send buffer deque
        copy(load_buff, load_buff + num_read, back_inserter(send_buff));

        //Now let other people use the send buffer
        send_buffer_mutex.unlock();
    }
    return;
}

//Init function, takes the sequence number and the ack number we should kick
//things off with
void jstp_stream::init(uint32_t seq, uint32_t ack){
    //First, lets create the socket pair which we use to interact with the
    //application layer.
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockpair);

    //Put them in non-blocking mode, this is very convinient
    /* fcntl(sockpair[0], F_SETFL, fcntl(sockpair[0], F_GETFL, 0) | O_NONBLOCK); */
    /* fcntl(sockpair[1], F_SETFL, fcntl(sockpair[1], F_GETFL, 0) | O_NONBLOCK); */

    //Now, lets set the sequence and ack numbers appropriatly
    base_seq_num = seq;
    ack_num = ack;

    //Sorry it looks shitty, it starts the threads though.
    threads_running = true;
    sender_thread = thread([this] {sender();});
    receiver_thread = thread([this] {receiver();});
    loader_thread = thread([this] {loader();});
}
