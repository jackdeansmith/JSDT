//Implimentation of jstp_segment.hpp

//C std lib
#include <netinet/in.h>
#include <cstring>

//STL
#include <vector>
using std::vector;
#include <iterator>
using std::back_inserter;
#include <algorithm>
using std::copy;
#include <string>
using std::string;
#include <sstream>
using std::ostringstream; using std::endl;

//Our source
#include "jstp_segment.hpp"

const size_t jstp_segment::MAX_PAYLOAD_SIZE;
const size_t jstp_segment::MAX_SEGMENT_SIZE;

//Getters for header data:
uint32_t jstp_segment::get_sequence(){
    return sequence;
}

uint32_t jstp_segment::get_ack(){
    return ack;
}

uint32_t jstp_segment::get_window(){
    return window;
}

uint32_t jstp_segment::get_length(){
    return payload.size();
}

bool jstp_segment::get_syn_flag(){
    return (flags >> 15) & 1;
}

bool jstp_segment::get_ack_flag(){
    return (flags >> 14) & 1;
}

bool jstp_segment::get_exit_flag(){
    return (flags >> 13) & 1;
}

//Setters for header data:
void jstp_segment::set_sequence(uint32_t in){
    sequence = in;
}

void jstp_segment::set_ack(uint32_t in){
    ack = in;
}

void jstp_segment::set_window(uint32_t in){
    window = in;
}

void jstp_segment::set_syn_flag(){
    flags |= 1 << 15;
}

void jstp_segment::set_ack_flag(){
    flags |= 1 << 14;
}

void jstp_segment::set_exit_flag(){
    flags |= 1 << 13;
}

void jstp_segment::reset_syn_flag(){
    flags &= ~(1 << 15);
}

void jstp_segment::reset_ack_flag(){
    flags &= ~(1 << 14);
}

void jstp_segment::reset_exit_flag(){
    flags &= ~(1 << 13);
}

//Interface for payload
void jstp_segment::clear_payload(){
    payload.clear();
}

//Set the payload from an input vector
void jstp_segment::set_payload(const vector<uint8_t>& in){
    if(in.size() > MAX_PAYLOAD_SIZE){
        //TODO error condition 
    }
    payload.reserve(in.size());
    copy(in.begin(), in.end(), back_inserter(payload));
}

const vector<uint8_t> jstp_segment::get_payload(){
    return payload;
}

vector<uint8_t>::const_iterator jstp_segment::payload_begin(){
    return payload.cbegin();
}

vector<uint8_t>::const_iterator jstp_segment::payload_end(){
    return payload.cend();
}

//Serialize and Deserialize methods, required in order to make this class
//serializable.
vector<uint8_t> jstp_segment::serialize(){
    //First reserve the necessary size for the vector
    vector<uint8_t> out;
    out.reserve(HEADER_SIZE + payload.size());

    //For each header field, we need to convert the number to network order then
    //cast the number to uint8s and write them to the data output.
    uint8_t arr[4];

    uint32_t sequence_net = htonl(sequence);
    memcpy(arr, &sequence_net, 4);
    copy(arr, arr + 4, back_inserter(out));

    uint32_t ack_net = htonl(ack);
    memcpy(arr, &ack_net, 4);
    copy(arr, arr + 4, back_inserter(out));

    uint32_t window_net = htonl(window);
    memcpy(arr, &window_net, 4);
    copy(arr, arr + 4, back_inserter(out));

    uint32_t length_net = htonl(payload.size());
    memcpy(arr, &length_net, 4);
    copy(arr, arr + 4, back_inserter(out));

    uint16_t flags_net = htons(flags);
    memcpy(arr, &flags_net, 2);
    copy(arr, arr + 2, back_inserter(out));

    //Lastly, copy the payload over into the serialized data and return it
    copy(payload.begin(), payload.end(), back_inserter(out));
    return out;
}

void jstp_segment::deserialize(const vector<uint8_t>& v){
    auto ptr = v.data();

    uint32_t sequence_net;
    memcpy(&sequence_net, ptr, 4); ptr += 4;
    sequence = ntohl(sequence_net);

    uint32_t ack_net;
    memcpy(&ack_net, ptr, 4); ptr += 4;
    ack = ntohl(ack_net);

    uint32_t window_net;
    memcpy(&window_net, ptr, 4); ptr += 4;
    window = ntohl(window_net);

    uint32_t length_net;
    memcpy(&length_net, ptr, 4); ptr += 4;
    uint32_t length = ntohl(length_net);

    uint16_t flags_net;
    memcpy(&flags_net, ptr, 2); ptr += 2;
    flags = ntohs(flags_net);

    payload.clear();
    copy(v.begin() + 18, v.begin() + 18 + length, back_inserter(payload));
}

//Get a string summarizing the headers
string jstp_segment::header_str(){
   ostringstream oss; 
   oss << "Headers for JSTP Segment:" << endl;
   oss << "    Sequence Number = " << get_sequence() << endl;
   oss << "    Ack Number      = " << get_ack() << endl;
   oss << "    Window Length   = " << get_window() << endl;
   oss << "    Payload Length  = " << get_length() << endl;

   oss << "    Flags           = ";
   if(get_syn_flag()){
       oss << "SYN, ";
   }
   if(get_ack_flag()){
        oss << "ACK, "; 
   }
   if(get_exit_flag()){
        oss << "EXIT"; 
   }
   oss << endl;
   return oss.str();
}

//Get a string summarizing the payload
string jstp_segment::payload_str(){
    ostringstream oss;
    oss << "Payload for JSTP segment:" << endl << "    ";
    string s;
    copy(payload.begin(), payload.end(), back_inserter(s));
    oss << s;
    return oss.str();
}
