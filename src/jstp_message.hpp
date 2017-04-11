#pragma once

#include <iostream>
using std::istream; using std::ostream;
#include <string>
using std::string;
#include <vector>
using std::vector;

/**
 * Class that implements JSTP_Message
 */
class JSTP_Message {

    public:
        /// Default constructor
        JSTP_Message() {}

        /// Copy constructor (disabled)
        JSTP_Message(const JSTP_Message&) = delete;

        /// Assignment operator (disabled)
        void operator=(const JSTP_Message&) = delete;

        /// Destructor
        virtual ~JSTP_Message() {}

    public:
        /**
         * Message types
         *
         * The message has three action types, request, deny, and data. This enum is used
         * to specify which should be/has been used.
         */
        enum Actions {Request, Deny, Data};

        /// Simplified writing to streams
        friend ostream& operator<<(ostream&, const JSTP_Message&);

        /// Simplified reading from streams
        friend istream& operator>>(istream&, const JSTP_Message&);

        /*
        /// Encoding
        JSTP_Message(string); //TODO: decide params

        /// Decoding
        JSTP_Message(ostream&); //TODO: errors to check?

        - OR -

        /// Encode message
        //void Encode(Actions, string,); //TODO: handled by setters
        */

    private: //protected:
        //
        // Message Header
        //

        /// Message action-type
        Actions _action;

        /// Associated filename
        string _filename;

        /// Length of message body
        long _length;

        //
        // Message Body
        //

        /// Core content to send
        vector<unsigned char> _data;

    public:
        /**
         * Get action-type
         *
         * \returns Action-type
         */
        Actions GetAction() { return _action; }

        /// Set action-type
        void SetAction(Actions action) { _action = action; }

        /**
         * Get filename
         *
         * \returns Filename
         */
        string GetFilename() { return _filename; }

        /// Set filename
        void SetFilename(string filename) { _filename = filename; }

        /**
         * Get length of message body
         *
         * \returns Length of message body
         */
        long GetLength() { return _length; }

        /// Set length for message body
        void SetLength(long length) { _length = length; }

        /**
         * Get data from message body
         *
         * \returns Data from message body
         */
        vector<unsigned char> GetData() { return _data; }

        /// Set data in message body
        void SetData(vector<unsigned char> data) { _data = data; }
};
