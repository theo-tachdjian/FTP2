#ifndef SERVER_ACTIONS_H
#define SERVER_ACTIONS_H

#include "../include/LPTF_Net/LPTF_Socket.hpp"

using namespace std;

bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username);

bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize, string username);

bool delete_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username);

bool list_directory(LPTF_Socket *serverSocket, int clientSockfd, string path, string username);

#endif