#ifndef LPTF_STRUCTS_H
#define LPTF_STRUCTS_H

#include <iostream>
using namespace std;

typedef struct {
    string filepath;
    uint32_t filesize;
} FILE_TRANSFER_REQ_PACKET_STRUCT;

typedef struct {
    uint32_t file_id;
    uint32_t offset;
    const void *data;
    uint16_t len;
} FILE_PART_PACKET_STRUCT;

#endif