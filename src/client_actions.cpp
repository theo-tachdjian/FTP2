#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/file_utils.hpp"

#include <iostream>
#include <fstream>

#include <filesystem>

using namespace std;

namespace fs = std::filesystem;


// check server reply
// return true if packet is a REPLY packet for a Command Packet, otherwise false.
bool wait_for_server_reply(LPTF_Socket *clientSocket) {
    LPTF_Packet reply = clientSocket->read();
    
    if (reply.type() == REPLY_PACKET && is_command_packet(get_refered_packet_type_from_reply_packet(reply))) {
        cout << get_reply_content_from_reply_packet(reply) << endl;
        return true;
    } else if (reply.type() == ERROR_PACKET) {
        cout << "Error reply from server (" << get_error_content_from_error_packet(reply) << ")" << endl;
        return false;
    } else {
        cout << "Unexpected reply from server (" << reply.type() << ")" << endl;
        return false;
    }
}


bool download_file(LPTF_Socket *clientSocket, string filename) {

    cout << "Downloading file \"" << filename << "\"" << endl;

    LPTF_Packet pckt = build_file_download_request_packet(filename);
    clientSocket->write(pckt);

    // check server reply
    LPTF_Packet reply = clientSocket->read();

    uint32_t filesize;
    
    if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply) == DOWNLOAD_FILE_COMMAND) {
        
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

    ofstream outfile(fs::path(filename).filename(), ios::binary);

    streampos curr_pos = outfile.tellp();

    try {

        do {
            pckt = clientSocket->read();

            if (pckt.type() != BINARY_PART_PACKET) {
                cerr << "Packet is not a Binary Part Packet ! (" << pckt.type() << ")" << endl;
                break;
            }

            BINARY_PART_PACKET_STRUCT data = get_data_from_binary_part_packet(pckt);

            // cout << "File part Data Len: " << data.len << endl;

            outfile.write((const char*)data.data, data.len);

            curr_pos = outfile.tellp();

            // notify server
            // this is required to not overflow? the socket
            char repc = 0;
            pckt = build_reply_packet(BINARY_PART_PACKET, &repc, 1);
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
    
    if (!fs::is_regular_file(targetfile)) {
        cout << "File \"" << targetfile << "\" doesn't exist !" << endl;
        return false;
    }

    uint32_t filesize = get_file_size(targetfile);
    cout << "File Size: " << filesize << endl;

    LPTF_Packet pckt = build_file_upload_request_packet(filename, filesize);
    clientSocket->write(pckt);

    // check server reply
    LPTF_Packet reply = clientSocket->read();
    
    if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply) == UPLOAD_FILE_COMMAND) {
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

    cout << "Sending file to server..." << endl;

    char buffer[MAX_BINARY_PART_BYTES];

    ifstream file(targetfile, ios::binary);

    streampos begin = file.tellg();
    file.seekg (0, ios::end);
    streampos end = file.tellg();
    file.seekg(0, ios::beg);

    try {

        streampos curr_pos = begin;
        do {
            uint16_t read_size;
            if (MAX_BINARY_PART_BYTES + curr_pos > end) {
                read_size = end - curr_pos;
            } else {
                read_size = MAX_BINARY_PART_BYTES;
            }

            file.read(buffer, read_size);

            pckt = build_binary_part_packet(buffer, static_cast<uint16_t>(read_size));
            clientSocket->write(pckt);

            curr_pos = file.tellg();

            // wait for server reply
            // this is required to not overflow? the socket
            reply = clientSocket->read();
            
            if (reply.type() != REPLY_PACKET && reply.type() != ERROR_PACKET) {
                cerr << "Unexpected packet type!" << endl;
                // reply.print_specs();
                file.close();
                return false;
            } else if (reply.type() == ERROR_PACKET) {
                cout << "Error reply from server: " << get_error_content_from_error_packet(reply) << endl;
                file.close();
                return false;
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

    cout << "Upload done." << endl;

    return true;

}


bool delete_file(LPTF_Socket *clientSocket, string filename) {

    cout << "Removing file \"" << filename << "\"" << endl;

    LPTF_Packet pckt = build_file_delete_request_packet(filename);
    clientSocket->write(pckt);

    // check server reply
    return wait_for_server_reply(clientSocket);
}


bool list_directory(LPTF_Socket *clientSocket, string pathname) {
    
    cout << "Listing directory \"" << pathname << "\"" << endl;

    LPTF_Packet pckt = build_list_directory_request_packet(pathname);
    clientSocket->write(pckt);

    // check server reply
    return wait_for_server_reply(clientSocket);
}


bool create_directory(LPTF_Socket *clientSocket, string folder) {

    cout << "Creating directory \"" << folder << "\"" << endl;

    LPTF_Packet pckt = build_create_directory_request_packet(folder);
    clientSocket->write(pckt);

    // check server reply
    return wait_for_server_reply(clientSocket);
}


bool remove_directory(LPTF_Socket *clientSocket, string folder) {

    cout << "Removing directory " << folder << endl;

    LPTF_Packet pckt = build_remove_directory_request_packet(folder);
    clientSocket->write(pckt);

    // check server reply
    return wait_for_server_reply(clientSocket);
}


bool rename_directory(LPTF_Socket *clientSocket, string newname, string path) {

    cout << "Renaming directory \"" << path << "\" to \"" << newname << "\"" << endl;

    LPTF_Packet pckt = build_rename_directory_request_packet(newname, path);
    clientSocket->write(pckt);

    // check server reply
    return wait_for_server_reply(clientSocket);
}


bool list_tree(LPTF_Socket *clientSocket) {

    cout << "Listing user directory tree" << endl;

    LPTF_Packet pckt = build_command_packet(USER_TREE_COMMAND, "");
    clientSocket->write(pckt);

    cout << "Start receiving directory tree from server" << endl;

    try {

        do {
            pckt = clientSocket->read();

            if (pckt.type() != BINARY_PART_PACKET) {
                cerr << "Packet is not a Binary Part Packet ! (" << pckt.type() << ")" << endl;
                break;
            }

            BINARY_PART_PACKET_STRUCT data = get_data_from_binary_part_packet(pckt);

            cout.write((const char*)data.data, data.len);
            cout.flush();

            // notify server
            // this is required to not overflow? the socket
            char repc = 0;
            pckt = build_reply_packet(BINARY_PART_PACKET, &repc, 1);
            clientSocket->write(pckt);

            // break if len of data is smaller than MAX_BINARY_PART_BYTES
            if (data.len != MAX_BINARY_PART_BYTES) break;

        } while (true);

    } catch (const exception &ex) {
        string msg = ex.what();
        cout << "Error when receiving dir tree: " << msg << endl;
        pckt = build_error_packet(ERROR_PACKET, ERR_CMD_UNKNOWN, msg);
        clientSocket->write(pckt);
    }

    cout << "Done" << endl;

    return true;
}