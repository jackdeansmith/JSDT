/* Implimentation of the class defined in udp_socket.hpp. */
#include "udp_socket.hpp"

//CSTD library and UNIX stuff
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
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
#include <utility>
using std::swap;

//Construct a socket with support for segments of up to mss in size.
udp_socket::udp_socket(size_t mss, double p): loss_probability(p){

    //Seed the random number generator with the time of day
    rand_engine.seed(time(nullptr));

    //Set the max segment size
    max_segment_size = mss;

    //Use the socket syscall to request a socket for use with UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    //If the socket could not be created for whatever reason...
    if(fd < 0){
        //Throw an exception, TODO
    }

    //Allocate space for the receiving buffer
    recv_buffer = new uint8_t[max_segment_size];
}

//Copy constructor
udp_socket::udp_socket(udp_socket& other){
    //Importantly, we can't just copy the file descriptor, we need to duplicate
    //it.
    fd = dup(other.fd);

    //We can copy the other parameters verbatim.
    has_peer = other.has_peer;
    bound = other.bound;
    max_segment_size = other.max_segment_size;
    peer_addr = other.peer_addr;
    local_addr = other.local_addr;

    //Finally, we just need to allocate space for the recv buffer
    recv_buffer = new uint8_t[max_segment_size];
}

//Swap operation
void udp_socket::swap(udp_socket& l, udp_socket& r){
    using std::swap;
    swap(l.fd, r.fd);
    swap(l.has_peer, r.has_peer);
    swap(l.bound, r.bound);
    swap(l.max_segment_size, r.max_segment_size);
    swap(l.peer_addr, r.peer_addr);
    swap(l.local_addr, r.local_addr);
    swap(l.recv_buffer, r.recv_buffer);
}

//Copy assignment operator, using copy swap idiom
udp_socket& udp_socket::operator=(udp_socket other){
    swap(*this, other);
    return *this;
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

void udp_socket::set_peer(const sockaddr_in& addr){
    peer_addr = addr;
    has_peer = true;
}

const sockaddr_in udp_socket::get_local_addr(){
    return local_addr;
}
const sockaddr_in udp_socket::get_peer_addr(){
    return peer_addr;
}
const sockaddr_in udp_socket::get_last_addr(){
    return last_recvd_addr;
}


//Allows the setting of the loss probability "mid-flight"
void udp_socket::set_loss_probability(double prob){
    loss_probability = prob;
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
vector<uint8_t> udp_socket::recv(bool timeout, timeval tv){
    //If the socket isn't bound...
    if(!bound){
        //... then we clearly shoudln't be allowed to receive anything.
        //TODO throw exception. 
    }

    //First, create an output vector, leave it empty for now.
    vector<uint8_t> output;

    //If the user requested that we do our processing with a timout, we need to
    //do a bit of extra work.
    if(timeout){

        //Create a set of file descriptors we will watch for activity, in this
        //case, just the udp socket
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(fd, &readset);

        //Wait up to the specified time or untill one of our watched files gets
        //updated. Return the number of updated files.
        int flag = select(fd + 1, &readset, nullptr, nullptr, &tv);

        //If the flag is 0, that means we waited our whole timeout window.
        if(flag == 0){
            return output; 
        }
        //Otherwise, we're good to go. The recvfrom below is guarenteed not to
        //block.
    }

    //Make a call to recv from, place the address of the person we received from
    //into the last_recvd_addr struct
    int len = sizeof(last_recvd_addr);
    size_t count = recvfrom(fd, recv_buffer, max_segment_size, 0, 
            (struct sockaddr *) &last_recvd_addr, (socklen_t *)&len);

    //If the loss simulation parameters tell us that the packet wasn't dropped
    if(!was_dropped()){

        //... then reserve space in the output vector and copy the data before
        //returning it.
        output.reserve(count);
        copy(recv_buffer, recv_buffer + count, back_inserter(output));
    }

    return output;
}

//Send a serializable object TODO error checking
void udp_socket::send(serializable& obj){
    send(obj.serialize());
}

//Recv a serializable object
bool udp_socket::recv(serializable& obj, bool timeout, timeval tv){
    vector<uint8_t> recvd = recv(timeout, tv);
    if(recvd.size() == 0){
        return false; 
    }
    else{
        obj.deserialize(recvd);
        return true;
    }
}

//Returns true if we should intentionally drop a packet
bool udp_socket::was_dropped(){
    //Create a bernouli distribution and use it to determine if we should
    //drop the packet or not.
    std::bernoulli_distribution dist(loss_probability);
    return dist(rand_engine);
}
