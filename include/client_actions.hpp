#pragma once

#include <iostream>
#include "LPTF_Net/LPTF_Socket.hpp"

using namespace std;

bool download_file(LPTF_Socket *clientSocket, string filename);

bool upload_file(LPTF_Socket *clientSocket, string filename, string targetfile);

bool delete_file(LPTF_Socket *clientSocket, string filename);

bool list_directory(LPTF_Socket *clientSocket, string pathname);

bool create_directory(LPTF_Socket *clientSocket, string folder);

bool remove_directory(LPTF_Socket *clientSocket, string folder);

bool rename_directory(LPTF_Socket *clientSocket, string newname, string path);

bool list_tree(LPTF_Socket *clientSocket);
