#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"

#include <iostream>
#include <fstream>

using namespace std;

uint32_t get_file_size(string filepath) {
    streampos begin, end;
    ifstream f (filepath, ios::binary);
    begin = f.tellg();
    f.seekg (0, ios::end);
    end = f.tellg();
    f.close();
    return static_cast<uint32_t>(end-begin);
}


// CLIENT Functions

bool download_file(LPTF_Socket *clientSocket, string filename) {
    printf("%d", clientSocket->listen(0));
    cout << filename;
    return false;
}


bool upload_file(LPTF_Socket *clientSocket, string filename, string targetfile) {
    uint32_t filesize = get_file_size(targetfile);
    cout << "File Size: " << filesize << endl;

    LPTF_Packet pckt = build_file_transfer_request_packet(filename, filesize, false);
    clientSocket->write(pckt);

    LPTF_Packet reply = clientSocket->read();

    uint16_t file_id;

    // check server reply
    if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply)) {
        reply.print_specs();
        file_id = get_file_id_from_file_transfer_reply_packet(reply);

        cout << "File ID: " << file_id << endl;

    } else if (reply.type() == ERROR_PACKET) {
        cout << "Error reply from server (" << get_error_content_from_error_packet(reply) << ")" << endl;
        return false;
    } else {
        cout << "Unexpected reply from server (" << reply.type() << ")" << endl;
        return false;
    }

    char buffer[MAX_FILE_PART_BYTES];

    ifstream file(targetfile, ios::binary);

    streampos begin = file.tellg();
    file.seekg (0, ios::end);
    streampos end = file.tellg();
    file.seekg(0, ios::beg);

    try {

        streampos curr_pos = begin;
        do {
            uint16_t read_size;
            if (MAX_FILE_PART_BYTES + curr_pos > end) {
                file.read(buffer, end - curr_pos);
                read_size = end - curr_pos;
            } else {
                file.read(buffer, MAX_FILE_PART_BYTES);
                read_size = MAX_FILE_PART_BYTES;
            }

            pckt = build_file_part_packet(file_id, static_cast<uint32_t>(curr_pos), buffer, static_cast<uint16_t>(read_size));
            clientSocket->write(pckt);

            curr_pos = file.tellg();

            // wait for server reply
            // this is required to not overflow? the socket
            reply = clientSocket->read();
            
            if (reply.type() != REPLY_PACKET) {
                cerr << "Unexpected packet type!" << endl;
                reply.print_specs();
                break;
            }

        } while (curr_pos < end);

        file.close();

    } catch (const exception &ex) {
        string msg = ex.what();
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        clientSocket->write(pckt);
        file.close();
        return false;
    }

    return true;

}


// SERVER Functions

bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename) {
    printf("%d", serverSocket->listen(0));
    cout << filename << clientSockfd;
    return false;
}

bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize) {
    cout << "Start receiving file" << endl;
    
    // TODO validate user and file

    uint32_t file_id = 123;

    LPTF_Packet pckt = build_reply_packet(COMMAND_PACKET, &file_id, sizeof(uint32_t));
    serverSocket->send(clientSockfd, pckt, 0);

    ofstream outfile(filename, ios::binary);

    streampos curr_pos = outfile.tellp();

    try {

        while (static_cast<uint32_t>(curr_pos) < filesize) {
            pckt = serverSocket->recv(clientSockfd, 0);

            if (pckt.type() != FILE_PART_PACKET) {
                cerr << "Packet is not a File Part Packet ! (" << pckt.type() << ")" << endl;
                pckt.print_specs();
                break;
            }

            FILE_PART_PACKET_STRUCT data = get_data_from_file_data_packet(pckt);

            cout << "File part data: " << "F. ID: " << data.file_id << " Offset: " << data.offset << " Len: " << data.len << endl;

            if (data.file_id != file_id) {
                cerr << "Invalid File ID ! (" << data.file_id << ")" << endl;
                break;
            }

            outfile.write((const char*)data.data, data.len);

            curr_pos = outfile.tellp();

            // send reply to client
            // this is required to not overflow? the socket
            char repc = 0;
            pckt = build_reply_packet(FILE_PART_PACKET, &repc, 1);
            serverSocket->send(clientSockfd, pckt, 0);
        }

        cout << "File transfer done. Curr. Pos: " << curr_pos << ", Filesize: " << filesize << endl;

        outfile.close();
    } catch (const exception &ex) {
        string msg = ex.what();
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        serverSocket->send(clientSockfd, pckt, 0);
        outfile.close();
        return false;
    }

    return true;
}