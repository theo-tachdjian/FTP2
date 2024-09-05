#pragma once

#include "LPTF_Net/LPTF_Socket.hpp"
#include "logger.hpp"

using namespace std;

bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username, Logger *logger);

bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize, string username, Logger *logger);

bool delete_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username, Logger *logger);

bool list_directory(LPTF_Socket *serverSocket, int clientSockfd, string path, string username, Logger *logger);

bool create_directory(LPTF_Socket *serverSocket, int clientSockfd, string dirname, string path, string username, Logger *logger);

bool remove_directory(LPTF_Socket *serverSocket, int clientSockfd, string folder, string username, Logger *logger);

bool rename_directory(LPTF_Socket *serverSocket, int clientSockfd, string newname, string path, string username, Logger *logger);