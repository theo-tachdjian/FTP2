#pragma once

#include <iostream>
#include "LPTF_Net/LPTF_Socket.hpp"

using namespace std;


typedef struct {
    bool ok;
    string msg;
} ACTION_STATUS;


ACTION_STATUS download_file(LPTF_Socket *clientSocket, string outfile, string filename);

ACTION_STATUS upload_file(LPTF_Socket *clientSocket, string filename, string targetfile);

ACTION_STATUS delete_file(LPTF_Socket *clientSocket, string filename);

ACTION_STATUS list_directory(LPTF_Socket *clientSocket, string pathname);

ACTION_STATUS create_directory(LPTF_Socket *clientSocket, string folder);

ACTION_STATUS remove_directory(LPTF_Socket *clientSocket, string folder);

ACTION_STATUS rename_directory(LPTF_Socket *clientSocket, string newname, string path);

ostringstream list_tree(LPTF_Socket *clientSocket);
