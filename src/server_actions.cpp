#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/file_utils.hpp"

#include <iostream>
#include <fstream>

#include <filesystem>

using namespace std;

namespace fs = std::filesystem;


bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username) {

    fs::path filepath = get_user_root(username);
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!fs::is_regular_file(filepath)) {
        string msg("The file doesn't exist.");
        cout << msg << endl;
        LPTF_Packet error_packet = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, msg);
        serverSocket->send(clientSockfd, error_packet, 0);
        return false;
    }

    uint32_t filesize = get_file_size(filepath);

    cout << filesize << endl;

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
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_FAILURE, msg);
        serverSocket->send(clientSockfd, pckt, 0);
        file.close();
        return false;
    }

    return true;
}


bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize, string username) {

    fs::path filepath(get_user_root(username));
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    ofstream outfile;
    outfile.open(filepath, ios::binary | ios::out);

    streampos curr_pos = outfile.tellp();

    LPTF_Packet pckt;

    // cannot open file for output
    if (curr_pos == -1) {
        string err_msg = "Could not create file !";
        cout << "Error: " << err_msg << endl;
        pckt = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, err_msg);
        serverSocket->send(clientSockfd, pckt, 0);
        outfile.close();
        return false;
    }

    pckt = build_reply_packet(COMMAND_PACKET, (void *)FILE_TRANSFER_REP_OK, strlen(FILE_TRANSFER_REP_OK));
    serverSocket->send(clientSockfd, pckt, 0);

    cout << "Start receiving file" << endl;

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
            
            // bad file
            if (curr_pos == -1) { throw runtime_error("File stream pos invalid!"); }

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
        pckt = build_error_packet(FILE_PART_PACKET, ERR_CMD_UNKNOWN, msg);
        serverSocket->send(clientSockfd, pckt, 0);
        outfile.close();
    }

    if (curr_pos == -1 || curr_pos != filesize) {
        cout << "File transfer encountered an error: file size and intended file size don't match (Curr. Pos: " << curr_pos << ", FSize: " << filesize << ")." << endl;
        // if (fs::exists(filepath)) {
        //     cout << "Removing file " << filepath << "." << endl;
        //     fs::remove(filepath);
        // }
        return false;
    } else {
        cout << "File transfer done. Curr. Pos: " << curr_pos << ", Filesize: " << filesize << endl;
        return true;
    }
}


bool delete_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username) {
    fs::path filepath = get_user_root(username);
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!fs::is_regular_file(filepath)) {
        string msg("The file doesn't exist.");
        cout << msg << endl;
        LPTF_Packet error_packet = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, msg);
        serverSocket->send(clientSockfd, error_packet, 0);
        return false;
    }

    // delete the file

    if (fs::remove(filepath)) {
        uint8_t status = 1;
        LPTF_Packet reply = build_reply_packet(COMMAND_PACKET, (void*)&status, sizeof(status));
        serverSocket->send(clientSockfd, reply, 0);

        return true;
    } else {
        string err_msg = "The file could not be removed.";
        cout << "Error when removing file " << filepath << ": " << err_msg << endl;
        LPTF_Packet err_pckt = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, err_msg);
        serverSocket->send(clientSockfd, err_pckt, 0);

        return false;
    }
}


bool list_directory(LPTF_Socket *serverSocket, int clientSockfd, string path, string username) {
    fs::path folderpath = get_user_root(username);
    folderpath /= path;

    cout << "Folderpath: " << folderpath << endl;

    if (!fs::is_directory(folderpath) || (path.size() > 0 && (path.at(0) == '/' || path.at(0) == '\\'))) {
        string msg("The folder doesn't exist.");
        cout << msg << endl;
        LPTF_Packet error_packet = build_error_packet(COMMAND_PACKET, ERR_CMD_FAILURE, msg);
        serverSocket->send(clientSockfd, error_packet, 0);
        return false;
    }

    const char *entry_prefix[] = {
        "<FILE>\t",
        "<DIR>\t"
    };

    // list directory content

    string result = "";
    for (fs::directory_entry const& dir_entry : fs::directory_iterator{folderpath}) {
        bool is_dir = fs::is_directory(dir_entry);
        result.append(entry_prefix[is_dir]);
        if (is_dir)
            result.append(dir_entry.path().stem().string());
        else
            result.append(dir_entry.path().filename().string());
        result.append("\n");
    }

    if (result.size() == 0) result.append("(empty)");

    LPTF_Packet reply = build_reply_packet(COMMAND_PACKET, (void*)result.c_str(), result.size());
    serverSocket->send(clientSockfd, reply, 0);

    return true;
}