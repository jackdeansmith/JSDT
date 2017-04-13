#include <iostream>
using std::cout; using std::cerr; using std::endl;
#include <string>
using std::string; using std::stoi;
//INSERT POINT
#include <stdexcept>

//My headers for reliable data transfer and for file transfer
#include "file_layer.hpp"

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

    return 0;
};
