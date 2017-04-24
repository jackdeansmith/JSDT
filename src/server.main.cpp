//TODO boilerplate
//The main file for the sender program
#include <string>
using std::string; using std::stoi;
#include <iostream>
using std::cout; using std::cerr; using std::endl;
#include <fstream>
using std::ifstream;
#include <stdexcept>
//My headers for reliable data transfer and for file transfer
#include "file_layer.hpp"
#include "jstp_streams.hpp"

//Usage, <executable> portnumber
int main(int argc, char* argv[]){

    //The first step is checking for errors in the user's input.
    //Check that the number of args received is correct
    if(argc != 2){
        cerr << "Expected one arg, Received " << argc -1 << endl;
        return 1;
    }

    //Try to convert the portnumber arg to an int
    int portnum;
    try{
        portnum = stoi(argv[1]); 
    }
    catch(std::invalid_argument e){
        cerr << "The argument \"" << argv[1] 
             << "\" could not be converted to a valid port number" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }
    catch(std::out_of_range e){
        cerr << "The argument \"" << argv[1] 
             << "\" was out of range, port number range from 0 to 65536" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }

    //Verify that the portnumber is in the valid range
    if(portnum > 65536){
        cerr << "The argument \"" << argv[1] 
             << "\" was out of range, port number range from 0 to 65536" << endl;
        cerr << "Exiting with status code 1" << endl;
        return 1;
    }

    //Warning if the user is using a reserved portnumber
    if(portnum <= 1024){
        cerr << "Warning: The portnumber you have specified is reserved,"
                " you will not be able to use this port unless you are running"
                " with superuser privelages" << endl << endl;
    }

    //Print a message to the user summarizing user intent
    cout << "You have requested that I use the following information." << endl;
    cout << "Listening Port Number: " << portnum << endl;
    cout << endl;

    //Create an acceptor object, use it to try and open a stream
    jstp_acceptor acceptor(portnum);
    jstp_stream stream(acceptor, 0);    //TODO use real loss
    cout << "Stream Created!" << endl;

    //Next, we need to accept the segment the client is sending
    cout << "Attempting to receive a request message..." << endl;
    incoming_message req;
    req.recv(stream);
    cout << "    ...request message received!" << endl;

    //Print out some diagnostic messages to the server output
    cout << "The client requested a file by the name of \""
         << req.get_filename() << "\'" << endl;

    //Try to open a file stream by the name of the user request
    ifstream ifs;
    ifs.open(req.get_filename());

    //If the file could not be opened...
    if(!ifs.is_open()){
        //...Print a message and send a deny back to the client
        cout << "Unfortunatly, the file could not be found."
                "Sending back a deny message" << endl; 
        outgoing_message deny;
        deny.set_action(action_type::DENY);
        deny.set_filename(req.get_filename());
        deny.send(stream);
    }

    //Otherwise, we have a good file to send
    cout << "The requested file was opened without any trouble!" << endl;

    cout << "Attempting to craft a response..." << endl;
    outgoing_message data_msg;
    data_msg.set_action(action_type::DATA);
    data_msg.set_filename(req.get_filename());
    data_msg.attach_data(ifs);
    cout << "    ... response crafted!" << endl;

    cout << "Attempting to reply..." << endl;
    data_msg.send(stream);
    cout << "    reply sent. Exiting now." << endl;

    return 0;
}
