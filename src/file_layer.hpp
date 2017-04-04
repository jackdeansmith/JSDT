#ifndef FILE_LAYER
#define FILE_LAYER

#include <iostream>
using std::istream; using std::ostream;
#include <string>
using std::string;
#include <vector>
using std::vector;

//Request a file, Deny sending a file, Send data
enum action_type{REQUEST, DENY, DATA};

static const string request_string = "REQUEST";

//Each file transfer message contains the following information
struct file_message{
    action_type action;
    string filename;
    vector<unsigned char> data;
};

class file_agent{
    private:

        //Contains two messages, one for incoming and one for outgoing
        //Outgoing can be modified and then sent
        //Incoming can be received and then queried
        file_message outgoing;
        file_message incoming;

        //Helper function 

    public:

        void set_outgoing_filename();
        string get_filename();

        //Attach data to the outgoing message and extract data from the incoming
        //message.
        void attach_data(istream&);
        void extract_data(ostream&);

        //Send the outgoing message or receive into the incoming message
        void send_out();
        void recv_in();
};

#endif
