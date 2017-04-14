//Implimentation of jstp_segment.hpp

#include <vector>
using std::vector;

#include "jstp_segment.hpp"

//Getters for header data:
uint32_t jstp_segment::get_sequence(){

}

uint32_t jstp_segment::get_ack(){

}

uint32_t jstp_segment::get_window(){

}

uint32_t jstp_segment::get_length(){

}

uint16_t jstp_segment::get_port(){

}

bool jstp_segment::get_syn_flag(){

}

bool jstp_segment::get_ack_flag(){

}

bool jstp_segment::get_close_flag(){

}

//Setters for header data:
void jstp_segment::set_sequence(uint32_t){

}

void jstp_segment::set_ack(uint32_t){

}

void jstp_segment::set_window(uint32_t){

}

void jstp_segment::set_port(uint16_t){

}

void jstp_segment::set_syn_flag(){

}

void jstp_segment::set_ack_flag(){

}

void jstp_segment::set_close_flag(){

}

void jstp_segment::reset_syn_flag(){

}

void jstp_segment::reset_ack_flag(){

}

void jstp_segment::reset_close_flag(){

}

//Interface for payload
void jstp_segment::clear_payload(){

}

void jstp_segment::set_payload(vector<uint8_t>){

}

vector<uint8_t>::iterator jstp_segment::payload_begin(){

}

vector<uint8_t>::iterator jstp_segment::payload_end(){

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
