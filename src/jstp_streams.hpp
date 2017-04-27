/* This file defines the interface to the jstp protocol through three objects,
 * acceptors, connectors, and streams. Streams are the only objects which send
 * user data but they can only do so once they have established a connection
 * with another host. Streams are constructed from a connector on the client
 * side and from an acceptor on the server side. Over the course of the
 * handshake, the server and client will both chose an ephemral port to exchange
 * all future data over and thus the acceptor and connector can be reused to
 * construct other streams.
 */

#pragma once

//Project specific headers
#include "udp_socket.hpp"
#include "jstp_segment.hpp"

//STL includes
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <condition_variable>

//Cstd includes
#include <cstdint>

//The connector class, used to construct streams on the client side.
class jstp_connector{
    friend class jstp_stream;
    public:
        jstp_connector(std::string hostname, uint16_t portno);
    private:
        std::string hostname;
        uint16_t port;
};

//The acceptor class, used to construct streams on the server side
class jstp_acceptor{
    friend class jstp_stream;
    public:
        jstp_acceptor(uint16_t portno);
    private:
        udp_socket acceptor_socket;
};

//The stream class, symetric once constructed. Used for transfering user data to
//and from both end systems.
class jstp_stream{
    public:
        //Settings
        static const size_t BUFF_CAPACITY = 64000000;  //64kB TODO tune this
        static const size_t TIMEOUT_SECONDS = 0;
        static const size_t TIMEOUT_USECS = 500000;
        
        //Constructed from either an acceptor or a connector, no default.
        jstp_stream(jstp_acceptor&, double loss_probability, size_t window);
        jstp_stream(jstp_connector&, double loss_probability, size_t window);

        //Can't be coppied or moved
        jstp_stream(jstp_stream& other) = delete;

        //Destructor does cleanup TODO
        ~jstp_stream();

        //Interface to the streams, looks a lot like TCP for a reason
        bool send(const std::vector<uint8_t>&);
        std::vector<uint8_t> recv();

    private:
        //The socket which we will use to communicate with our peer
        udp_socket stream_sock;

        //Used to manage the activities of the two threads
        std::atomic<bool> running; 
        std::atomic<bool> closing;
        std::atomic<bool> terminating;

        //Used in the termination process
        std::atomic<uint32_t> self_exit_number;
        std::atomic<uint32_t> peer_exit_number;

        //Used during data transfer
        std::atomic<uint32_t> self_ack_number;
        std::atomic<uint32_t> self_rwnd;
        std::atomic<uint32_t> other_rwnd;
        std::atomic<bool> force_send;
        std::atomic<bool> data_on_wire;
        size_t window_limit;

        //The send buffer and associated things
        std::mutex send_buffer_mutex;
        std::deque<uint8_t> send_buffer;
        uint32_t sender_base_sequence;
        size_t offset;
        
        //Used to determine if the send buffer has been fully flushed
        std::mutex flush_lock;
        std::condition_variable flushed;

        //The receiver buffer and assiciated things
        std::mutex recv_buffer_mutex;
        std::deque<uint8_t> recv_buffer;

        //Timeval which indicates when the next timeout will happen
        std::chrono::steady_clock::time_point last_new_ack;

        //Sender thread support
        std::thread sender_thread;
        std::mutex sender_notify_lock;
        std::condition_variable sender_condition_var;
        void sender_main();

        //Receiver thread support
        std::thread receiver_thread;
        void receiver_main();

        //Function which starts threads and inits variables, used by the
        //constructor
        void init(uint32_t, uint32_t);

        //Needed to make output for debug messages at all readable
        std::mutex cout_mutex;
};
