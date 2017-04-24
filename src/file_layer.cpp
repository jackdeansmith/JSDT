
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
#include <time.h>

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
    bool action_determined = false;
    bool filename_determined = false;
    bool length_determined = false;

    string action_string;
    string length_string;

    filename.clear();
    data.clear();
    size_t length = 0;

    //Loop forever
    for(;;){

        while(iter == recv_vect.end()){
            //Minimize the busy waiting by puting some delay in here
            timespec t;
            t.tv_sec = 0;
            t.tv_nsec = 500000000;
            nanosleep(&t, nullptr);
            recv_vect = stream.recv(); 
            iter = recv_vect.begin();
        }

        //Determine the action type
        if(!action_determined){
            if(*iter != '\n'){
                action_string.push_back(*iter);     
            }
            else{
                action_determined = true; 
                if(action_string == request_str){
                    action = action_type::REQUEST; 
                }
                else if(action_string == deny_str){
                    action = action_type::DENY; 
                }
                else if(action_string == data_str){
                    action = action_type::DATA; 
                }
            }
            iter++;
        }

        else if(!filename_determined){
            if(*iter != '\n'){
                filename.push_back(*iter);     
            }
            else{
                filename_determined = true; 
            }
            iter++;
        }

        else if(!length_determined){
            if(*iter != '\n'){
                length_string.push_back(*iter);     
            }
            else{
                length_determined = true;
                length = stoi(length_string);
            }
            iter++;
        }

        else if(length > 0){
            data.reserve(length);
            data.push_back(*iter);        
            iter++;
            length--;
        }

        else{
            break;
        }
    
    }


    /* //Get the action string and translate it to an action */
    /* c = ' '; */
    /* string action_str; */
    /* do{ */
    /*     size_t read = stream.recv(&c, 1); */ 
    /*     if(read){ */
    /*         action_str.push_back(c); */ 
    /*     } */

    /* } while(c != '\n'); */

    /* if(action_str == request_str){ */
    /*     action = action_type::REQUEST; */ 
    /* } */
    /* else if(action_str == deny_str){ */
    /*     action = action_type::DENY; */ 
    /* } */
    /* else if(action_str == data_str){ */
    /*     action = action_type::DATA; */ 
    /* } */
    /* else{ */
    /*     //Error condition, TODO handle this */ 
    /* } */

    /* //Get the filename */
    /* c = ' '; */
    /* filename.clear(); */
    /* do{ */
    /*     size_t read = stream.recv(&c, 1); */ 
    /*     if(read){ */
    /*         filename.push_back(c); */ 
    /*     } */

    /* } while(c != '\n'); */

    /* //Get the length and convert it to a number */
    /* c = ' '; */
    /* string length_str; */
    /* do{ */
    /*     size_t read = stream.recv(&c, 1); */ 
    /*     if(read){ */
    /*         action_str.push_back(c); */ 
    /*     } */

    /* } while(c != '\n'); */
    /* size_t length = stoi(length_str); */

    /* //Get the data */
    /* data.clear(); */
    /* while(length > 0){ */
    /*     size_t read = stream.recv(&c, 1); */ 
    /*     if(read){ */
    /*         data.push_back(c); */ 
    /*         length--; */
    /*     } */ 
    /* } */
}
