#pragma once

#include <iostream>
using namespace std;

typedef struct {
    string filepath;
    uint32_t filesize;
} FILE_UPLOAD_REQ_PACKET_STRUCT;

typedef struct {
    string dirname;
    string path;
} CREATE_DIR_REQ_PACKET_STRUCT;

typedef struct {
    string newname;
    string path;
} RENAME_DIR_REQ_PACKET_STRUCT;

typedef struct {
    const void *data;
    uint16_t len;
} FILE_PART_PACKET_STRUCT;