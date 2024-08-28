#include <iostream>
#include <cstring>

#include <netinet/in.h>

#include "../../include/LPTF_Net/LPTF_Packet.hpp"
#include "../../include/LPTF_Net/LPTF_Utils.hpp"

using namespace std;


// repfrom is the packet type this reply refers to
LPTF_Packet build_reply_packet(uint8_t repfrom, void *repcontent, uint16_t contentsize) {
    uint8_t *rawcontent = (uint8_t*)malloc(sizeof(uint8_t)+contentsize);

    if (!rawcontent)
        throw runtime_error("Memory allocation failed !");

    memcpy(rawcontent, &repfrom, sizeof(uint8_t));
    memcpy(rawcontent + sizeof(uint8_t), repcontent, contentsize);

    LPTF_Packet packet(REPLY_PACKET, rawcontent, sizeof(uint8_t)+contentsize);

    free(rawcontent);
    return packet;
}


LPTF_Packet build_message_packet(const string &message) {
    LPTF_Packet packet(MESSAGE_PACKET, (void*)message.c_str(),
                       message.size()+1);     // include null term
    return packet;
}


LPTF_Packet build_command_packet(const string &cmd, const string &arg) {
    uint8_t *rawcontent = (uint8_t*)malloc(cmd.size() + arg.size() + 2);

    if (!rawcontent)
        throw runtime_error("Memory allocation failed !");
    
    memcpy(rawcontent, cmd.c_str(), cmd.size()+1);
    memcpy(rawcontent + cmd.size() + 1, arg.c_str(), arg.size()+1);

    LPTF_Packet packet(COMMAND_PACKET, rawcontent,
                       cmd.size() + arg.size() + 2);
    free(rawcontent);
    return packet;
}


// errfrom is the packet type this error refers to
LPTF_Packet build_error_packet(uint8_t errfrom, uint8_t err_code, string &errmsg) {
    uint8_t *rawcontent = (uint8_t*)malloc(sizeof(uint8_t)*2 + errmsg.size()+1);

    if (!rawcontent)
        throw runtime_error("Memory allocation failed !");
    
    memcpy(rawcontent, &errfrom, sizeof(uint8_t));
    memcpy(rawcontent + sizeof(uint8_t), &err_code, sizeof(uint8_t));
    memcpy(rawcontent + sizeof(uint8_t)*2, errmsg.c_str(), errmsg.size()+1);

    LPTF_Packet packet(ERROR_PACKET, rawcontent,
                       sizeof(uint8_t)*2 + errmsg.size()+1);
    free(rawcontent);
    return packet;
}


LPTF_Packet build_file_upload_request_packet(const string filepath, uint32_t filesize) {
    string command = UPLOAD_FILE_COMMAND;
    size_t size = command.size()+1 + filepath.size()+1 + sizeof(filesize);
    uint8_t *rawcontent = (uint8_t*)malloc(size);

    if (!rawcontent)
        throw runtime_error("Memory allocation failed !");

    filesize = htonl(filesize);

    memcpy(rawcontent, command.c_str(), command.size()+1);
    memcpy(rawcontent + command.size()+1, filepath.c_str(), filepath.size()+1);
    memcpy(rawcontent + command.size()+1 + filepath.size()+1, &filesize, sizeof(filesize));

    LPTF_Packet packet(COMMAND_PACKET, rawcontent, size);

    free(rawcontent);
    return packet;
}


LPTF_Packet build_file_download_request_packet(const string filepath) {
    return build_command_packet(DOWNLOAD_FILE_COMMAND, filepath);
}


LPTF_Packet build_file_part_packet(void *data, uint16_t datalen) {
    LPTF_Packet packet(FILE_PART_PACKET, data, datalen);

    return packet;
}


string get_message_from_message_packet(LPTF_Packet &packet) {
    string message;

    if (packet.type() != MESSAGE_PACKET) throw runtime_error("Invalid packet (type or length)");

    message = string((const char *)packet.get_content(), packet.get_header().length);

    // Null term check
    if (message.at(packet.get_header().length-1) != '\0')
        message.append("\0");

    return message;
}


string get_command_from_command_packet(LPTF_Packet &packet) {
    string cmd;

    if (packet.type() != COMMAND_PACKET) throw runtime_error("Invalid packet (type or length)");

    const char *content = (const char *)packet.get_content();

    int i = 0;
    while (i < packet.get_header().length) {
        if (content[i] == '\0') {
            cmd = string(content, i+1);
            break;
        } else if (i == packet.get_header().length-1) {
            cmd = string(content, i+1);
            cmd.append("\0");
        }

        i++;
    }

    return cmd;
}


string get_arg_from_command_packet(LPTF_Packet &packet) {
    string arg;

    if (packet.type() != COMMAND_PACKET) throw runtime_error("Invalid packet (type or length)");

    const char *content = (const char *)packet.get_content();

    int i = 0;
    int arg_offset = 0;
    while (i < packet.get_header().length) {
        if (content[i] == '\0') {
            arg_offset = i+1;
            i++;
            break;
        }

        i++;
    }

    while (i < packet.get_header().length) {
        if (content[i] == '\0') {
            arg = string(content+arg_offset, i+1 - arg_offset);
            break;
        } else if (i == packet.get_header().length-1) {
            arg = string(content+arg_offset, i+1 - arg_offset);
            arg.append("\0");
        }

        i++;
    }

    return arg;
}


uint8_t get_refered_packet_type_from_reply_packet(LPTF_Packet &packet) {
    uint8_t typefrom;

    if (packet.type() != REPLY_PACKET or packet.get_header().length < 1) throw runtime_error("Invalid packet (type or length)");

    typefrom = ((const uint8_t*)packet.get_content())[0];

    return typefrom;
}


string get_reply_content_from_reply_packet(LPTF_Packet &packet) {
    string message;

    if (packet.type() != REPLY_PACKET || packet.get_header().length < 2) throw runtime_error("Invalid packet (type or length)");

    message = string((const char *)packet.get_content()+1, packet.get_header().length-1);

    // Null term check
    if (message.at(packet.get_header().length-2) != '\0')
        message.append("\0");

    return message;
}


uint8_t get_refered_packet_type_from_error_packet(LPTF_Packet &packet) {
    uint8_t typefrom;

    if (packet.type() != ERROR_PACKET || packet.get_header().length < 1) throw runtime_error("Invalid packet (type or length)");

    typefrom = ((const uint8_t*)packet.get_content())[0];

    return typefrom;
}


uint8_t get_error_code_from_error_packet(LPTF_Packet &packet) {
    uint8_t code;

    if (packet.type() != ERROR_PACKET || packet.get_header().length < 2) throw runtime_error("Invalid packet (type or length)");

    code = ((const uint8_t*)packet.get_content())[1];

    return code;
}


string get_error_content_from_error_packet(LPTF_Packet &packet) {
    string message;

    if (packet.type() != ERROR_PACKET || packet.get_header().length < 2) throw runtime_error("Invalid packet (type or length)");

    message = string((const char *)packet.get_content()+1, packet.get_header().length-1);

    // Null term check
    if (message.at(packet.get_header().length-2) != '\0')
        message.append("\0");

    return message;
}


FILE_UPLOAD_REQ_PACKET_STRUCT get_data_from_file_upload_request_packet(LPTF_Packet &packet) {
    if (packet.type() != COMMAND_PACKET || packet.get_header().length < 2) throw runtime_error("Invalid packet (type or length)");

    const char *content = (const char *)packet.get_content();

    // find offset of args based on command name
    // use size() to get the offset
    int i = 0;
    size_t offset = 0;
    while (i < packet.get_header().length) {
        if (content[i] == '\0') {
            offset = i+1;
            i++;
            break;
        }

        i++;
    }

    string filepath;

    while (i < packet.get_header().length) {
        if (content[i] == '\0') {
            filepath = string(content+offset, i+1 - offset);
            break;
        } else if (i == packet.get_header().length-1) {
            filepath = string(content+offset, i+1 - offset);
            filepath.append("\0");
        }

        i++;
    }

    uint32_t filesize;
    memcpy(&filesize, content + i +1, sizeof(uint32_t));

    // cout << "filesize " << filesize << endl;

    filesize = ntohl(filesize);
    // cout << "filesize ntohl " << filesize << endl;

    return {filepath, filesize};
}


string get_file_from_file_download_request_packet(LPTF_Packet &packet) {
    return get_arg_from_command_packet(packet);
}


FILE_PART_PACKET_STRUCT get_data_from_file_data_packet(LPTF_Packet &packet) {
    if (packet.type() != FILE_PART_PACKET || packet.get_header().length < 2) throw runtime_error("Invalid packet (type or length)");

    return {(const char *)packet.get_content(), static_cast<uint16_t>(packet.get_header().length)};
}