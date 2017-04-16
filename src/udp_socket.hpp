/* This header defines a wraping class for UDP sockets and an abstract class
 * which can be inherited from to indicate that an object is serializable and
 * thus can be sent over the UDP socket.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <netinet/in.h>

// Abstract class representing the concept of serializability. The udp socket is
// set up to be able to send and receive any object which is serializable given
// a large enough maximum segment size.
// Classes should only inherit from serializable if their serialized
// representation compleetly defines their state and two objects deserialized
// from the same representation are indistinguashable. 
class serializable{
    public:
        virtual std::vector<uint8_t> serialize() = 0;
        virtual void deserialize(const std::vector<uint8_t>&) = 0;
};

// A class which wraps a UDP socket and makes it play nice with c++. Using this
// class prevents buffer overflows by requiring the user specify the max segment
// size the socket can handle at constructin. Any packets longer than this are
// truncated.
//
// This wrapper allows the user to ignore endianess concerns when setting port
// numbers and addresses by doing this behind the scenes. It does NOT, however, 
// do anything to byte ordering of payload data, this problem must be dealt with
// when objects are serialized.
class udp_socket{

    public:
        //Rule of N stuff
        udp_socket(size_t mss);
        udp_socket(udp_socket& other);
        void swap(udp_socket& l, udp_socket& r);
        udp_socket& operator=(udp_socket other);
        ~udp_socket();

        //Bind the socket to a port on the local machine, bind_local_any binds
        //to an arbitrary ephemeral port on the local machine.
        void bind_local(unsigned short port);
        void bind_local_any();

        //Get info about how the socket is bound
        bool is_bound();
        unsigned short bound_to();

        //Allow the user to specify the peer, that is, where the packets go when
        //they are sent out.
        void set_peer(std::string hostname, unsigned short port);
        void set_peer(const sockaddr_in&);

        //Getters for the loacl address, peer address, and last received address
        const sockaddr_in get_local_addr();
        const sockaddr_in get_peer_addr();
        const sockaddr_in get_last_addr();

        //Primary interface, send and receive arbitrary serial data represented
        //as uint8_t vectors.
        void send(const std::vector<uint8_t>&);
        std::vector<uint8_t> recv();

        //Send and receive any sendable object
        void send(serializable&);
        void recv(serializable&);

    private:

        //The file descriptor we do all our sending on
        int fd;

        //Flags used to keep track of connection state
        bool has_peer;
        bool bound;

        //The maximum segment size and a buffer of this size used for receiving
        //raw data from the network.
        size_t max_segment_size;
        uint8_t* recv_buffer;

        //The address of our peer and ourselves as well as the last person who
        //send us something.
        sockaddr_in peer_addr;
        sockaddr_in local_addr;
        sockaddr_in last_recvd_addr;
};
