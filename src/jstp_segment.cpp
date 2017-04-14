//Implimentation of jstp_segment.hpp

#include <vector>
using std::vector;
#include <iterator>
using std::back_inserter;
#include <algorithm>
using std::copy;

#include "jstp_segment.hpp"

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

uint16_t jstp_segment::get_port(){
    return port;
}

bool jstp_segment::get_syn_flag(){
    return (flags >> 15) & 1;
}

bool jstp_segment::get_ack_flag(){
    return (flags >> 14) & 1;
}

bool jstp_segment::get_close_flag(){
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

void jstp_segment::set_port(uint16_t in){
    port = in;
}

void jstp_segment::set_syn_flag(){
    flags |= 1 << 15;
}

void jstp_segment::set_ack_flag(){
    flags |= 1 << 14;
}

void jstp_segment::set_close_flag(){
    flags |= 1 << 13;
}

void jstp_segment::reset_syn_flag(){
    flags &= ~(1 << 15);
}

void jstp_segment::reset_ack_flag(){
    flags &= ~(1 << 14);
}

void jstp_segment::reset_close_flag(){
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

vector<uint8_t>::const_iterator jstp_segment::payload_begin(){
    return payload.cbegin();
}

vector<uint8_t>::const_iterator jstp_segment::payload_end(){
    return payload.cend();
}

//Serialize and Deserialize methods, required in order to make this class
//serializable.
vector<uint8_t> jstp_segment::serialize(){

}

void jstp_segment::deserialize(const vector<uint8_t>&){

}

//Private data members I can use
/* uint32_t sequence; */
/* uint32_t ack; */
/* uint32_t window; */
/* uint16_t port; */
/* uint16_t flags; */
/* vector<uint8_t> payload; */
