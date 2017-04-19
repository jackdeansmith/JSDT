#pragma once

#include <iostream>
using std::istream; using std::ostream;
#include <string>
using std::string;
#include <vector>
using std::vector;

#include "jstp_streams.hpp"

//The message has three action types, request, deny, and data. This enum is used
//to specify which should be/has been used.
namespace action_type{
    enum Enum{REQUEST, DENY, DATA};
};

//Each message, incoming or outgoing, contains the following
class file_message{
    public:
        string str();       //Obtain a string representation of the messsage

    protected:

        //Data which fully defines the message
        action_type::Enum action;
        string filename;
        vector<unsigned char> data;

        //The strings which specify the action type
        const string request_str = "REQUEST";
        const string deny_str = "DENY";
        const string data_str = "DATA";
};

//Outgoing message type, only allows modification of fields and sending
class outgoing_message: public file_message{
    public:
        void set_action(const action_type::Enum&);
        void set_filename(const string&);
        void attach_data(istream&);
        void send(jstp_stream&);
};

//Incoming message type, only allows receiving and getting fields
class incoming_message: public file_message{
    public:
        action_type::Enum get_action();
        string get_filename();
        void extract_data(ostream&);
        void recv(jstp_stream&);
};
