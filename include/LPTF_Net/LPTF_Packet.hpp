#ifndef LPTF_PACKET_H
#define LPTF_PACKET_H

#include <stdlib.h>
#include <stdint.h>


#define REPLY_PACKET 0
#define MESSAGE_PACKET 1
#define COMMAND_PACKET 2
#define BINARY_PACKET 3
#define FILE_PART_PACKET 4

#define ERROR_PACKET 0xFF   // a packet type should not be higher than this value


// commands available for COMMAND packet
#define UPLOAD_FILE_COMMAND "UPLOAD"
#define DOWNLOAD_FILE_COMMAND "DOWNLOAD"
#define DELETE_FILE_COMMAND "RMFILE"
#define LIST_FILES_COMMAND "LIST"
#define CREATE_FOLDER_COMMAND "MKDIR"
#define DELETE_FOLDER_COMMAND "RMDIR"
#define RENAME_FOLDER_COMMAND "RNDIR"

// command error codes
#define ERR_CMD_FAILURE 0
#define ERR_CMD_UNKNOWN 1


typedef struct {
    uint16_t length;
    uint8_t type;
    uint8_t reserved;
} PACKET_HEADER;


#define FILE_TRANSFER_REP_OK "OK"

// constexpr uint16_t MAX_FILE_PART_BYTES = UINT16_MAX - sizeof(PACKET_HEADER) - sizeof(uint32_t) - sizeof(uint32_t);
constexpr uint16_t MAX_FILE_PART_BYTES = 8000;


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