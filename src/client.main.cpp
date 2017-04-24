#include <iostream>
using std::cout; using std::cerr; using std::endl;
#include <fstream>
using std::ofstream;
#include <string>
using std::string; using std::stoi;
//INSERT POINT
#include <stdexcept>

//My headers for reliable data transfer and for file transfer
#include "file_layer.hpp"
#include "jstp_streams.hpp"

//Main function for the client
int main(int argc, char* argv[]){
    //Receives the parameters sender_hostname, sender_portnumber, filename

    //The first step is checking for errors in the user's input.
    //Check that the number of args received is correct
    if(argc != 4){
        cerr << "Expected three args, Received " << argc -1 << endl;
        return 1;
    }

    //Try to convert the sender_portnumber arg to an int
    int sender_portnum;
    try{
        sender_portnum = stoi(argv[2]); 
    }
    catch(std::invalid_argument e){
        cerr << "The argument \"" << argv[2] 
             << "\" could not be converted to a valid port number" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }
    catch(std::out_of_range e){
        cerr << "The argument \"" << argv[2] 
             << "\" was out of range, port number range from 0 to 65536" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }

    //Verify that the portnumber is in the valid range
    if(sender_portnum > 65536){
        cerr << "The argument \"" << argv[2] 
             << "\" was out of range, port number range from 0 to 65536" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }

    //Warning if the user is using a reserved portnumber
    if(sender_portnum <= 1024){
        cerr << "Warning: The portnumber you have specified is reserved,"
                " you will not be able to use this port unless you are running"
                " with superuser privelages" << endl << endl;
    }

    //Grab the remaining fields
    string sender_hostname(argv[1]);
    string filename(argv[3]);
    //End error handling
    
    //Print a message to the user summarizing their intent
    cout << "You have requested that I use the following information." << endl;
    cout << "    Sender Hostname: " << sender_hostname << endl;
    cout << "    Sender Port    : " << sender_portnum << endl;
    cout << "    Filename       : " << filename << endl;
    cout << endl;

    //Establish a connection with the server
    jstp_connector connector(sender_hostname, sender_portnum);
    jstp_stream stream(connector, 0);      //TODO make this lose some packets

    //Craft the request message
    outgoing_message request;
    request.set_action(action_type::REQUEST);
    request.set_filename(filename);

    //Send the request message over the stream
    request.send(stream);

    //Get a response message back
    incoming_message response;
    response.recv(stream);

    //If the response is a DENY message
    if(response.get_action() == action_type::DENY){
        //Then print an error and exit.
        cout << "Sorry, the server said it didn't have that file available."
             << endl; 
        cout << "Exiting." << endl;
        return 1;
    }

    //If that wasn't the case but they also aren't sending data...
    else if(response.get_action() != action_type::DATA){
        //...I (the programmer) did something wrong. 
        cerr << "Hey there buddy, this shoudn't happen!" << endl;
        return 1;
    }

    //Now we know that they sent us the file we wanted. 
    //Lets open a file on our system so we can save it. Open it in trunc mode to
    //discard the file's current contents if it already exists.
    ofstream ofs;
    ofs.open(filename, std::ios::trunc);

    //If the file failed to open...
    if(!ofs.is_open()){
        //Print an error message and exit
        cerr << "Error: Data was received from the server but no local file "
                "could be opened to save the data. Check the permissions of "
                "any existing files of the same name and try again." << endl; 
        return 1;
    }

    //Alright, we now have an open file. All we have to do is extract our data
    //into it and then close.
    response.extract_data(ofs);
    ofs.close();

    //We are done! print a message telling the user.
    cout << "The file was sucessfully received and was written "
            "to the filesystem. Exiting." << endl;

    return 0;
};
