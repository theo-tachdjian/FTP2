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

#include <experimental/filesystem>

using namespace std;

namespace fs = std::experimental::filesystem;


void print_help() {
    cout << "Usage:" << endl;
    cout << "\tlpf <ip>:<port> <command> [args]" << endl;
    cout << endl << "Available Commands:" << endl;
    cout << "\t-upload <file>" << endl;
    cout << "\t-download <file>" << endl;
}


bool check_command(int argc, char const *argv[]) {
    if (argc < 4)
        return false;

    if (strcmp(argv[2], "-upload") == 0) {
        if (argc != 4) {
            cout << "Too much arguments !" << endl;
        } else {
            return true;
        }
    } else if (strcmp(argv[2], "-download") == 0) {
        if (argc != 4) {
            cout << "Too much arguments !" << endl;
        } else {
            return true;
        }
    } else {
        cout << "Unknown command !" << endl;
    }

    return false;
}


int main(int argc, char const *argv[]) {
    string ip;
    int port;

    if (argc < 4) {
        cout << "Too few arguments !" << endl;
        print_help();
        return 2;
    // print help
    } else if (argc == 2 && (strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_help();
        return 0;
    }

    string serv_arg = argv[1];
    size_t sep_index = serv_arg.find(':');

    if (sep_index == string::npos) {
        cout << "Server address is wrong !" << endl;
        print_help();
        return 2;
    }

    ip = serv_arg.substr(0, sep_index);
    port = atoi(serv_arg.substr(sep_index+1, serv_arg.size()).c_str());

    if (ip.size() == 0)
        ip = "127.0.0.1";
    if (port == 0)
        port = 12345;

    cout << "IP: " << ip << ", Port: " << port <<endl;

    // check if command + args are valid before connecting to the server
    if (!check_command(argc, argv)) {
        return 2;
    }

    // // char file[] = "Tests/hello.txt";
    // // char outfile[] = "hello.txt";
    // char file_to_dl[] = "hello.txt";

    try {
        LPTF_Socket clientSocket = LPTF_Socket();

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAddr.sin_port = htons(port);

        clientSocket.connect(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));

        if (strcmp(argv[2], "-upload") == 0) {

            string outfile = fs::path(argv[3]).filename();      // TODO replace when server has folders
            string file = argv[3];

            cout << "Uploading File " << file << " as " << outfile << endl;

            upload_file(&clientSocket, outfile, file);

        } else if (strcmp(argv[2], "-download") == 0) {

            string file = argv[3];

            cout << "Downloading File " << file << endl;

            download_file(&clientSocket, file);

        }

    } catch (const exception &ex) {
        cerr << "Exception: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
