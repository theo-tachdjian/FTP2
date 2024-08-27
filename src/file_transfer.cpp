#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"

#include <iostream>

// CLIENT Functions

void download_file(LPTF_Socket clientSock, uint32_t file_id, string filename) {

}


void upload_file(LPTF_Socket clientSock, uint32_t file_id, string targetfile) {

}


// SERVER Functions

void send_file(LPTF_Socket serverSocket, int clientSockfd, uint32_t file_id) {

}

void receive_file(LPTF_Socket serverSocket, int clientSockfd, uint32_t file_id) {

}