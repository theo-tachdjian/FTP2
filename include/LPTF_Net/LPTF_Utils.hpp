#ifndef LPTF_UTILS_H
#define LPTF_UTILS_H

using namespace std;

#include <iostream>

#include "LPTF_Packet.hpp"
#include "LPTF_Structs.hpp"

LPTF_Packet build_reply_packet(uint8_t repfrom, void *repcontent, uint16_t contentsize);
LPTF_Packet build_message_packet(const string &message);
LPTF_Packet build_command_packet(const string &cmd, const string &arg);
LPTF_Packet build_error_packet(uint8_t errfrom, uint8_t err_code, string &errmsg);

LPTF_Packet build_file_upload_request_packet(const string filepath, uint32_t filesize);
LPTF_Packet build_file_download_request_packet(const string filepath);

LPTF_Packet build_file_part_packet(void *data, uint16_t datalen);

string get_message_from_message_packet(LPTF_Packet &packet);
string get_command_from_command_packet(LPTF_Packet &packet);
string get_arg_from_command_packet(LPTF_Packet &packet);
uint8_t get_refered_packet_type_from_reply_packet(LPTF_Packet &packet);
string get_reply_content_from_reply_packet(LPTF_Packet &packet);
uint8_t get_refered_packet_type_from_error_packet(LPTF_Packet &packet);
uint8_t get_error_code_from_error_packet(LPTF_Packet &packet);
string get_error_content_from_error_packet(LPTF_Packet &packet);

FILE_UPLOAD_REQ_PACKET_STRUCT get_data_from_file_upload_request_packet(LPTF_Packet &packet);
string get_file_from_file_download_request_packet(LPTF_Packet &packet);

FILE_PART_PACKET_STRUCT get_data_from_file_data_packet(LPTF_Packet &packet);

#endif