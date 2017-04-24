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
        static const size_t BUFF_CAPACITY = 64000;  //64kB TODO tune this
        static const size_t RETRANSMIT_DELAY = 1;        //Arbitrary TODO set
        
        //Constructed from either an acceptor or a connector, no default.
        jstp_stream(jstp_acceptor&, double loss_probability);
        jstp_stream(jstp_connector&, double loss_probability);

        //Can't be coppied or moved
        jstp_stream(jstp_stream& other) = delete;

        //Destructor does cleanup TODO
        ~jstp_stream();

        //Interface to the streams, looks a lot like TCP for a reason
        size_t send(const std::vector<uint8_t>&);
        std::vector<uint8_t> recv();

    private:
        //The socket which we will use to communicate with our peer
        udp_socket stream_sock;

        //Data which must be available to everyone
        std::atomic<uint32_t> expected_sequence_number;
        std::atomic<bool> force_send;

        //The send buffer and associated things
        std::mutex send_buffer_mutex;
        std::deque<uint8_t> send_buffer;
        uint32_t sender_base_sequence;
        size_t offset;

        //The receiver buffer and assiciated things
        std::mutex recv_buffer_mutex;
        std::deque<uint8_t> recv_buffer;

};
