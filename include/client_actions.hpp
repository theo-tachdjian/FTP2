#ifndef CLIENT_ACTIONS_H
#define CLIENT_ACTIONS_H

#include <iostream>
#include "../include/LPTF_Net/LPTF_Socket.hpp"

using namespace std;

bool download_file(LPTF_Socket *clientSocket, string filename);

bool upload_file(LPTF_Socket *clientSocket, string filename, string targetfile);

#endif