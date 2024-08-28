#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/commands.hpp"

using namespace std;


int main() {
    char ip[] = "127.0.0.1";
    int port = 12345;

    try {
        LPTF_Socket clientSocket = LPTF_Socket();

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip);
        serverAddr.sin_port = htons(port);

        clientSocket.connect(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));

        // Etape 1
        // send message
        // cout << "Sending ping message." << endl;

        // LPTF_Packet msg = build_message_packet("ping");
        // ssize_t ret = clientSocket.write(msg);

        // if (ret != -1) {
        //     // read server response
        //     LPTF_Packet resp = clientSocket.read();

        //     cout << "Received: " << get_message_from_message_packet(resp) << endl;
        // }

        upload_file(&clientSocket, "hello.txt", "Tests/hello.txt");

        // download_file(&clientSocket, "hello.txt");


    } catch (const exception &ex) {
        cerr << "Exception: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
