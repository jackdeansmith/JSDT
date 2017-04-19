/* This file defines the interface for the jstp_segment. This segment is used
 * for all communication between hosts in the jstp protocol.
 */

/* A JSTP segment contains the following information in order:
 *     A 32 bit sequence number
 *     A 32 bit ack number
 *     A 32 bit window size
 *     A 32 bit length field (Length of the payload in bytes)
 *     A 16 bit flag field   (described below)
 *     A 16 bit port field   (usually unused)
 *     A variable ammount of payload data
 * All multibyte fields are manipulated in host byte ordering but when
 * serialized will be represented in a compatible format.
 */

/* The JSTP flag field consists of 16 bits. 
 * The first three most sygnificant bits represent the SYN, ACK, and CLOSE
 * flags, the remaining bits are reserved and unused.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include "udp_socket.hpp"

//Inherit serializable so that we can esily send and receive over UDP
class jstp_segment: public serializable{
    public:

        //Constants which define the max segment size and the max payload size
        //for the segment. These values differ by exactly the size of the
        //headers. Length of headers = 20 bytes.
        static const size_t MAX_SEGMENT_SIZE = 1024;
        static const size_t HEADER_SIZE = 20;
        static const size_t MAX_PAYLOAD_SIZE = 1004;

        //Explicitly only the default constructor, default move copy etc. should
        //all be just fine, we just use STL in this class.
        jstp_segment() = default;

        //Getters for header data
        uint32_t get_sequence();
        uint32_t get_ack();
        uint32_t get_window();
        uint32_t get_length();
        uint16_t get_port();
        bool get_syn_flag();
        bool get_ack_flag();
        bool get_close_flag();

        //Setters for header data
        void set_sequence(uint32_t);
        void set_ack(uint32_t);
        void set_window(uint32_t);
        //note: there is no setter for length, it is determined by the payload
        void set_port(uint16_t);
        void set_syn_flag();
        void set_ack_flag();
        void set_close_flag();
        void reset_syn_flag();
        void reset_ack_flag();
        void reset_close_flag();

        //Interact with the payload
        void clear_payload();
        void set_payload(const std::vector<uint8_t>&);
        const std::vector<uint8_t> get_payload();
        std::vector<uint8_t>::const_iterator payload_begin();
        std::vector<uint8_t>::const_iterator payload_end();

        //The key functionality, serializes the segment in a compleetly platform
        //independant fashion.
        std::vector<uint8_t> serialize();
        void deserialize(const std::vector<uint8_t>&);

        //Get strings which summarize the data in the headers and in the
        //payload, this is useful for debugging.
        std::string header_str();
        std::string payload_str();

    private:

        //Header data, the length header is not maintained separatly because it
        //is simply the size of the payload std::vector.
        uint32_t sequence;
        uint32_t ack;
        uint32_t window;
        uint16_t flags;
        uint16_t port;

        //Payload data
        std::vector<uint8_t> payload;
};
