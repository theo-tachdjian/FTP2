#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"

#include <iostream>
#include <fstream>

#include <experimental/filesystem>

using namespace std;

namespace fs = std::experimental::filesystem;

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
    cout << "Downloading file " << filename << endl;

    LPTF_Packet pckt = build_file_download_request_packet(filename);
    clientSocket->write(pckt);

    // check server reply
    LPTF_Packet reply = clientSocket->read();

    uint32_t filesize;
    
    if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply) == COMMAND_PACKET) {
        
        // get filesize from reply
        memcpy(&filesize, (const char *) reply.get_content() + sizeof(uint8_t), sizeof(filesize));
        cout << "File size: " << filesize << endl;

    } else if (reply.type() == ERROR_PACKET) {
        cout << "Error reply from server (" << get_error_content_from_error_packet(reply) << ")" << endl;
        return false;
    } else {
        cout << "Unexpected reply from server (" << reply.type() << ")" << endl;
        return false;
    }

    cout << "Start receiving file from server" << endl;

    ofstream outfile(filename, ios::binary);

    streampos curr_pos = outfile.tellp();

    try {

        do {
            pckt = clientSocket->read();

            if (pckt.type() != FILE_PART_PACKET) {
                cerr << "Packet is not a File Part Packet ! (" << pckt.type() << ")" << endl;
                break;
            }

            FILE_PART_PACKET_STRUCT data = get_data_from_file_data_packet(pckt);

            cout << "File part Data Len: " << data.len << endl;

            outfile.write((const char*)data.data, data.len);

            curr_pos = outfile.tellp();

            // notify server
            // this is required to not overflow? the socket
            char repc = 0;
            pckt = build_reply_packet(FILE_PART_PACKET, &repc, 1);
            clientSocket->write(pckt);
        } while (static_cast<uint32_t>(curr_pos) < filesize);
        
        outfile.close();

    } catch (const exception &ex) {
        string msg = ex.what();
        cout << "Error when downloading file: " << msg << endl;
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        clientSocket->write(pckt);
        outfile.close();
    }

    if (curr_pos != filesize) {
        cout << "File download encountered an error (file size and intended file size don't match)." << endl;
        if (fs::exists(filename)) {
            cout << "Removing file " << filename << ".";
            fs::remove(filename);
        }
        return false;
    } else {
        cout << "File download done. Curr. Pos: " << curr_pos << ", Filesize: " << filesize << endl;
        return true;
    }
}


bool upload_file(LPTF_Socket *clientSocket, string filename, string targetfile) {
    uint32_t filesize = get_file_size(targetfile);
    cout << "File Size: " << filesize << endl;

    LPTF_Packet pckt = build_file_upload_request_packet(filename, filesize);
    clientSocket->write(pckt);

    // check server reply
    LPTF_Packet reply = clientSocket->read();
    
    if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply) == COMMAND_PACKET) {
       string msg = get_reply_content_from_reply_packet(reply);

       cout << "Server reply: " << msg << endl;

       if (strcmp(msg.c_str(), FILE_TRANSFER_REP_OK) != 0) {
            return false;
       }

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
                read_size = end - curr_pos;
            } else {
                read_size = MAX_FILE_PART_BYTES;
            }

            file.read(buffer, read_size);

            pckt = build_file_part_packet(buffer, static_cast<uint16_t>(read_size));
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
        cout << "Error when uploading file: " << msg << endl;
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        clientSocket->write(pckt);
        file.close();
        return false;
    }

    return true;

}


// SERVER Functions

bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename) {
    // TODO check folder and file

    fs::path filepath("server_root");
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!fs::exists(filepath)) {
        string msg("The file doesn't exist.");
        cout << msg << endl;
        LPTF_Packet error_packet = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, msg);
        serverSocket->send(clientSockfd, error_packet, 0);
        return false;
    }

    uint32_t filesize = get_file_size(filepath);

    LPTF_Packet pckt = build_reply_packet(COMMAND_PACKET, (void *)&filesize, sizeof(filesize));
    serverSocket->send(clientSockfd, pckt, 0);

    cout << "Start sending file to client" << endl;

    char buffer[MAX_FILE_PART_BYTES];

    ifstream file(filepath, ios::binary);

    streampos begin = file.tellg();
    file.seekg (0, ios::end);
    streampos end = file.tellg();
    file.seekg(0, ios::beg);

    try {

        streampos curr_pos = begin;
        do {
            uint16_t read_size;
            if (MAX_FILE_PART_BYTES + curr_pos > end) {
                read_size = end - curr_pos;
            } else {
                read_size = MAX_FILE_PART_BYTES;
            }

            file.read(buffer, read_size);

            pckt = build_file_part_packet(buffer, static_cast<uint16_t>(read_size));
            serverSocket->send(clientSockfd, pckt, 0);

            curr_pos = file.tellg();

            // wait for client reply
            // this is required to not overflow? the socket
            pckt = serverSocket->recv(clientSockfd, 0);
            
            if (pckt.type() != REPLY_PACKET) {
                cerr << "Unexpected packet type!" << endl;
                pckt.print_specs();
                break;
            }

        } while (curr_pos < end);

        file.close();

    } catch (const exception &ex) {
        string msg = ex.what();
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        serverSocket->send(clientSockfd, pckt, 0);
        file.close();
        return false;
    }

    return true;
}

bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize) {
    // TODO check folder and file

    fs::path filepath("server_root");
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    LPTF_Packet pckt = build_reply_packet(COMMAND_PACKET, (void *)FILE_TRANSFER_REP_OK, strlen(FILE_TRANSFER_REP_OK));
    serverSocket->send(clientSockfd, pckt, 0);

    cout << "Start receiving file" << endl;

    ofstream outfile(filepath, ios::binary);

    streampos curr_pos = outfile.tellp();

    try {

        do {
            pckt = serverSocket->recv(clientSockfd, 0);

            if (pckt.type() != FILE_PART_PACKET) {
                cerr << "Packet is not a File Part Packet ! (" << pckt.type() << ")" << endl;
                break;
            }

            FILE_PART_PACKET_STRUCT data = get_data_from_file_data_packet(pckt);

            cout << "File part Data Len: " << data.len << endl;

            outfile.write((const char*)data.data, data.len);

            curr_pos = outfile.tellp();

            // send reply to client
            // this is required to not overflow? the socket
            char repc = 0;
            pckt = build_reply_packet(FILE_PART_PACKET, &repc, 1);
            serverSocket->send(clientSockfd, pckt, 0);
        } while (static_cast<uint32_t>(curr_pos) < filesize);
        
        outfile.close();

    } catch (const exception &ex) {
        string msg = ex.what();
        cout << "Error when receiving file: " << msg << endl;
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        serverSocket->send(clientSockfd, pckt, 0);
        outfile.close();
    }

    if (curr_pos != filesize) {
        cout << "File transfer encountered an error (file size and intended file size don't match)." << endl;
        if (fs::exists(filepath)) {
            cout << "Removing file " << filepath << ".";
            fs::remove(filepath);
        }
        return false;
    } else {
        cout << "File transfer done. Curr. Pos: " << curr_pos << ", Filesize: " << filesize << endl;
        return true;
    }
}