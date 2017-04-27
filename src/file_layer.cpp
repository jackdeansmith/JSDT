
#include "file_layer.hpp"
#include <iostream>
using std::cout; using std::endl;
#include <string>
using std::string;
#include <algorithm>
using std::copy;
#include <iterator>
using std::ostream_iterator;
#include <sstream>
using std::ostringstream;
#include <time.h>

//The strings which specify the action type
static const string request_str = "REQUEST";
static const string deny_str = "DENY";
static const string data_str = "DATA";

//Get a string representation of the message, this might be used later for the
//send function if I'm feeling particularly lazy.
string file_message::str(){
    ostringstream oss;

    //First, lets put the action_type on the first line
    if(action == action_type::REQUEST){
        oss << request_str << endl;    
    }
    else if(action == action_type::DENY){
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

    //Quick and dirty, copy the chars to a vector of uint8
    vector<uint8_t> v;
    v.reserve(message.size());

    copy(message.begin(), message.end(), back_inserter(v));

    stream.send(v);

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

//Quick and dirty recv
void incoming_message::recv(jstp_stream& stream){
    /* char c; */
    vector<uint8_t> recv_vect;
    auto iter = recv_vect.begin();

    string action_string;
    string length_string;
    filename.clear();
    data.clear();
    size_t length = 0;

    //Loop forever
    vector<string> strings;
    strings.resize(3);
    size_t string_index = 0;
    while(string_index < 3){
        //If we are at the end of our vector, wait around for half a second and
        //grab some more data, TODO make this event driven somehow?
        while(iter == recv_vect.end()){
            timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 500000000;
            nanosleep(&t, nullptr);
            recv_vect = stream.recv(); 
            iter = recv_vect.begin();
        }

        //Fill out the strings, using newlines as a separator
        char c = *iter; 
        if(string_index < 3){
            if(c == '\n'){
                string_index++; 
            }

            else{
                strings[string_index].push_back(c); 
            }
            iter++;
        }
    }

    //Set the action type
    if(strings[0] == request_str){
        action = action_type::REQUEST; 
    } 
    else if(strings[0] == deny_str){
        action = action_type::DENY; 
    } 
    else if(strings[0] == data_str){
        action = action_type::DATA; 
    } 

    //Set the filename
    filename = strings[1];
    length = stoi(strings[2]);
    string_index++;

    //Get the remaining characters into the data vector
    data.reserve(length);
    while(length > 0){
        while(iter == recv_vect.end()){
            timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 500000000;
            nanosleep(&t, nullptr);
            recv_vect = stream.recv(); 
            iter = recv_vect.begin();
        }

        data.push_back(*iter);
        iter++;
        length--;
    }
}
