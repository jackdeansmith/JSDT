
#include "file_layer.hpp"
#include <iostream>
using std::endl;
#include <string>
using std::string;
#include <algorithm>
using std::copy;
#include <iterator>
using std::ostream_iterator;
#include <sstream>
using std::ostringstream;

//Get a string representation of the message, this might be used later for the
//send function if I'm feeling particularly lazy.
string file_message::str(){
    ostringstream oss;

    //First, lets put the action_type on the first line
    if(action == action_type::REQUEST){
        oss << request_str << endl;    
    }
    if(action == action_type::DENY){
        oss << deny_str << endl;
    }
    else{
        oss << data_str << endl; 
    }

    //Next, the filename
    oss << filename << endl;

    //Then the length of the data, as a string
    oss << data.size() << endl;

    //Finally, the data itself
    copy(data.begin(), data.end(), ostream_iterator<unsigned char>(oss));
    
    return oss.str();
}


//Set the action of an outgoing_message
void outgoing_message::set_action(const action_type::Enum& action_in){
    action = action_in;
}

//Set the filename of an outgoing_message
void outgoing_message::set_filename(const string& filename_in){
    filename = filename_in;
}

//Quick and dirty way to attach data to an outgoing_message
void outgoing_message::attach_data(istream& is){
    data.clear();
    while(!is.eof()){
        data.push_back(is.get()); 
    }
    data.pop_back();
}

//Quick and dirty send, TODO make it copy free?
void outgoing_message::send(jstp_stream& stream){
    
    //First, get the whole message as a string
    string message = str();
    stream.send(&message[0], message.size());
    
}

//Get the action type of an incoming_message
action_type::Enum incoming_message::get_action(){
    return action;
}

//Get the filename associated with an incoming_message
string incoming_message::get_filename(){
    return filename;
}

//Extract the data from an incoming_message
void incoming_message::extract_data(ostream& os){
    copy(data.begin(), data.end(), ostream_iterator<unsigned char>(os));
}

//TODO impl
void incoming_message::recv(jstp_stream& stream){

}
