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

//STL includes
#include <string>

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
        //Constructed from either an acceptor or a connector, no default.
        jstp_stream(jstp_acceptor&);
        jstp_stream(jstp_connector&);

        //Can't be coppied or moved
        jstp_stream(jstp_stream& other) = delete;

        //Destructor does cleanup
        ~jstp_stream();

        //TODO send and receive interface
    private:
        udp_socket stream_sock;
        //TODO lots and lots of impl details here
};
