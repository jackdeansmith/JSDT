/* This file defines the interface to the jstp protocol through three objects,
 * acceptors, connectors, and streams. Streams are the only objects which send
 * user data but they can only do so once they have established a connection
 * with another host. Streams are constructed from a connector on the client
 * side and from an acceptor on the server side. Over the course of the
 * handshake, the server and client will both chose an ephemral port to exchange
 * all future data over and thus the acceptor and connector can be reused to
 * construct other streams.
 */

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
        size_t send(void* buffer, size_t length);
        size_t recv(void* buffer, size_t length);

    private:
        //Called by the constructors to do things like start threads, allocate
        //space for buffers etc... This work is shared no matter if we are
        //initiating the connection or accepting it.
        void init(uint32_t seq, uint32_t ack);

        //The udp socket used to communicate with the network and the socket
        //pair used to communicate with the application layer. We will by
        //convention use sockpair[0] for the application side and
        //sockpair[1] for the transport side
        udp_socket stream_sock;
        int sockpair[2];

        //Helper function which loads data from the socket pair into the send
        //buffer. Needs some buffer area for its own use
        void load_app_data();
        uint8_t load_buff[BUFF_CAPACITY];

        //Atomics for keeping track of variaus states
        std::atomic<uint32_t> ack_num;
        std::atomic<uint32_t> other_rwnd;

        //Atomic which allows the supervising thread to stop the workers
        std::atomic<bool> running;

        //The send buffer variables, should only be acessed by one thread at a
        //time.
        std::mutex send_buffer_mutex;
        std::deque<uint8_t> send_buff;
        uint32_t base_seq_num;
        uint32_t next_send_index;

        //The two threads which run to make our data transfer happen
        std::thread sender_thread;
        std::thread receiver_thread;
};
