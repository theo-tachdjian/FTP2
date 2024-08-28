#ifndef COMMANDS_H
#define COMMANDS_H

#include <iostream>

using namespace std;

// CLIENT Functions
bool download_file(LPTF_Socket *clientSocket, string filename);

bool upload_file(LPTF_Socket *clientSocket, string filename, string targetfile);


// SERVER Functions

bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename);

bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize);

#endif