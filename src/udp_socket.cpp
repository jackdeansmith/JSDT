/* Implimentation of the class defined in udp_socket.hpp. */
#include "udp_socket.hpp"

//CSTD library and UNIX stuff
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

//STL stuff
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <algorithm>
using std::copy;
#include <iterator>
using std::back_inserter;

//Construct a socket with support for segments of up to mss in size.
udp_socket::udp_socket(size_t mss){

    //Use the socket syscall to request a socket for use with UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    //If the socket could not be created for whatever reason...
    if(fd < 0){
        //Throw an exception, TODO
    }

    //Allocate space for the receiving buffer
    recv_buffer = new uint8_t[mss];
}

//Destruct the socket, simply close the file descriptor freeing it up and delete
//the receiving buffer.
udp_socket::~udp_socket(){
    close(fd);
    delete [] recv_buffer;
}

//Bind the socket to a local port
void udp_socket::bind_local(unsigned short port){
    //Create a new sockaddr_in to represent the new local addr, zero it out.
    sockaddr_in new_local_addr;
    memset((char *)&new_local_addr, 0, sizeof(new_local_addr));

    //Set the new address to use the specified port and any IP address the
    //system can give us.
    new_local_addr.sin_port = htons(port);
    new_local_addr.sin_family = AF_INET;
    new_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Attempt to bind the socket to our new structure, if it fails...
    if(bind(fd, (struct sockaddr*)&new_local_addr, sizeof(new_local_addr)) < 0){
        //Then throw an exception, TODO
    }

    //Now that we know we are bound to a good address, save the local address
    local_addr = new_local_addr;
    bound = true;
}

//Bind to any local port (gives an ephemeral port)
void udp_socket::bind_local_any(){
    //This is just an alias to the previous function, calling with arg 0
    //acomplishes our goal.
    bind_local(0);
}

//Return if the socket is bound to a local port
bool udp_socket::is_bound(){
    return bound;
}

//Return the port the socket is bound to
unsigned short udp_socket::bound_to(){
    return local_addr.sin_port;
}

//Set the peer which the socket will communicate with
void udp_socket::set_peer(string hostname, unsigned short port){
    //Step one, try to convert the hostname to a hostent object.
    hostent* hp;
    hp = gethostbyname(hostname.c_str());

    //If that failed...
    if(!hp){
        //... then throw an exception
    }

    //Step two, clear out data currently in the peeraddr struct and replace it 
    memset((char*)&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);

    //Use the host address we just got from the gethostbyname call for our peer
    //address
    memcpy((void *)&peer_addr.sin_addr, hp->h_addr_list[0], hp->h_length);

    //If that all worked, we should be good.
    has_peer = true;
}

//Send arbitrary data to our peer in a single segment
void udp_socket::send(const vector<uint8_t>& v){

    //If the socket isn't bound or doesn't have a peer...
    if(!has_peer || !bound){
        //... then we clearly shoudln't be allowed to send anyting. 
        //TODO throw exception. 
    }
    
    //Simply make the appropriate call to sendto
    sendto(fd, v.data(), v.size() , 0, 
           (sockaddr *) &peer_addr, sizeof(peer_addr));
}

//Receive up to mss bytes from the network, currently discards information about
//where this packet came from.
vector<uint8_t> udp_socket::recv(){
    //If the socket isn't bound...
    if(!bound){
        //... then we clearly shoudln't be allowed to receive anything.
        //TODO throw exception. 
    }

    //Make a call to recvfrom and put the data in recv_buffer, keep track of how
    //many bytes we actually got
    size_t count = recvfrom(fd, recv_buffer, mss, 0, 0, 0);

    //Create an output vector and reserve space for all the data we just got
    vector<uint8_t> output;
    output.reserve(count);

    //Copy the data to the output vector and then return it
    copy(recv_buffer, recv_buffer + count, back_inserter(output));
    return output;
}
