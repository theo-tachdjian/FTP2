#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Packet.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/file_utils.hpp"

#include <iostream>
#include <fstream>

#include <filesystem>

using namespace std;

namespace fs = std::filesystem;


void send_error_message(LPTF_Socket *serverSocket, int clientSockfd, uint8_t errfrom, string message, uint8_t errcode = ERR_CMD_FAILURE) {
    cout << message << endl;
    LPTF_Packet error_packet = build_error_packet(errfrom, errcode, message);
    serverSocket->send(clientSockfd, error_packet, 0);
}


bool send_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, string username) {

    fs::path user_root = get_user_root(username);
    fs::path filepath = user_root;
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!is_path_in_folder(filepath, user_root) || !fs::is_regular_file(filepath)) {
        send_error_message(serverSocket, clientSockfd, DOWNLOAD_FILE_COMMAND, "The file doesn't exist.");
        return false;
    }

    uint32_t filesize = get_file_size(filepath);

    cout << filesize << endl;

    LPTF_Packet pckt = build_reply_packet(DOWNLOAD_FILE_COMMAND, (void *)&filesize, sizeof(filesize));
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
        send_error_message(serverSocket, clientSockfd, DOWNLOAD_FILE_COMMAND, ex.what());
        file.close();
        return false;
    }

    return true;
}


bool receive_file(LPTF_Socket *serverSocket, int clientSockfd, string filename, uint32_t filesize, string username) {

    fs::path user_root = get_user_root(username);
    fs::path filepath = user_root;
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!is_path_in_folder(filepath, user_root) || !fs::is_directory(fs::path(filepath).remove_filename())) {
        send_error_message(serverSocket, clientSockfd, UPLOAD_FILE_COMMAND, "Target directory doesn't exist !");
        return false;
    }

    ofstream outfile;
    outfile.open(filepath, ios::binary | ios::out);

    streampos curr_pos = outfile.tellp();

    LPTF_Packet pckt;

    // cannot open file for output
    if (curr_pos == -1) {
        send_error_message(serverSocket, clientSockfd, UPLOAD_FILE_COMMAND, "Could not create file !");
        outfile.close();
        return false;
    }

    pckt = build_reply_packet(UPLOAD_FILE_COMMAND, (void *)FILE_TRANSFER_REP_OK, strlen(FILE_TRANSFER_REP_OK));
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
        send_error_message(serverSocket, clientSockfd, UPLOAD_FILE_COMMAND, ex.what());
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
    fs::path user_root = get_user_root(username);
    fs::path filepath = user_root;
    filepath /= filename;

    cout << "Filepath: " << filepath << endl;

    if (!is_path_in_folder(filepath, user_root) || !fs::is_regular_file(filepath)) {
        send_error_message(serverSocket, clientSockfd, DELETE_FILE_COMMAND, "The file doesn't exist.");
        return false;
    }

    // delete the file

    if (fs::remove(filepath)) {
        uint8_t status = 1;
        LPTF_Packet reply = build_reply_packet(DELETE_FILE_COMMAND, (void*)&status, sizeof(status));
        serverSocket->send(clientSockfd, reply, 0);

        return true;
    } else {
        send_error_message(serverSocket, clientSockfd, DELETE_FILE_COMMAND, "The file could not be removed.");
        return false;
    }
}


bool list_directory(LPTF_Socket *serverSocket, int clientSockfd, string path, string username) {
    fs::path user_root = get_user_root(username);
    fs::path folderpath = user_root;
    if (!path.empty())
        folderpath /= path;

    cout << "Folderpath: " << folderpath << endl;

    if (!fs::equivalent(user_root, folderpath) && (!is_path_in_folder(folderpath, user_root) || !fs::is_directory(folderpath)
        || (path.size() > 0 && (path.at(0) == '/' || path.at(0) == '\\')))) {
        send_error_message(serverSocket, clientSockfd, LIST_FILES_COMMAND, "The folder doesn't exist.");
        return false;
    }

    // list directory content

    string result = list_directory_content(folderpath);

    if (result.size() == 0) result.append("(empty)");

    LPTF_Packet reply = build_reply_packet(LIST_FILES_COMMAND, (void*)result.c_str(), result.size());
    serverSocket->send(clientSockfd, reply, 0);

    return true;
}


bool create_directory(LPTF_Socket *serverSocket, int clientSockfd, string dirname, string path, string username) {
    fs::path user_root = get_user_root(username);
    fs::path folderpath = user_root;
    if (!path.empty())
        folderpath /= path;
    
    if (!fs::is_directory(folderpath) || (path.size() > 0 && (path.at(0) == '/' || path.at(0) == '\\'))) {
        send_error_message(serverSocket, clientSockfd, CREATE_FOLDER_COMMAND, "The folder doesn't exist.");
        return false;
    }

    if (dirname.empty() || !is_path_in_folder(folderpath / dirname, user_root)
        || (dirname.at(0) == '/' || dirname.at(0) == '\\')) {
        send_error_message(serverSocket, clientSockfd, CREATE_FOLDER_COMMAND, "Invalid directory name.");
        return false;
    }

    if (fs::is_directory(folderpath / dirname)) {
        send_error_message(serverSocket, clientSockfd, CREATE_FOLDER_COMMAND, "Directory already exists.");
        return false;
    }

    // create directory

    if (!fs::create_directory(folderpath / dirname)) {
        send_error_message(serverSocket, clientSockfd, CREATE_FOLDER_COMMAND, "Failed to create directory !");
        return false;
    } else {
        bool status = 1;
        LPTF_Packet reply = build_reply_packet(CREATE_FOLDER_COMMAND, &status, sizeof(status));
        serverSocket->send(clientSockfd, reply, 0);

        return true;
    }
}


bool remove_directory(LPTF_Socket *serverSocket, int clientSockfd, string folder, string username) {
    fs::path user_root = get_user_root(username);
    fs::path folderpath = user_root / folder;

    // check if not user root folder
    if (fs::equivalent(user_root, folderpath) || !is_path_in_folder(folderpath, user_root)
        || (folder.size() > 0 && (folder.at(0) == '/' || folder.at(0) == '\\'))) {
        send_error_message(serverSocket, clientSockfd, DELETE_FOLDER_COMMAND, "Invalid folder.");
        return false;
    }

    if (!fs::is_directory(folderpath)) {
        send_error_message(serverSocket, clientSockfd, DELETE_FOLDER_COMMAND, "The directory doesn't exist.");
        return false;
    }

    // remove dir

    if (fs::remove_all(folderpath) == 0 /* if nothing removed */) {
        send_error_message(serverSocket, clientSockfd, DELETE_FOLDER_COMMAND, "The directory could not be removed !");
        return false;
    } else {
        bool status = 1;
        LPTF_Packet reply = build_reply_packet(DELETE_FOLDER_COMMAND, &status, sizeof(status));
        serverSocket->send(clientSockfd, reply, 0);

        return true;
    }
}


bool rename_directory(LPTF_Socket *serverSocket, int clientSockfd, string newname, string path, string username) {
    fs::path user_root = get_user_root(username);
    fs::path folderpath = user_root;
    if (!path.empty())
        folderpath /= path;
    
    if (fs::equivalent(user_root, folderpath) || !fs::is_directory(folderpath)
        || (path.size() > 0 && (path.at(0) == '/' || path.at(0) == '\\'))) {
        send_error_message(serverSocket, clientSockfd, RENAME_FOLDER_COMMAND, "The folder doesn't exist.");
        return false;
    }

    fs::path newfolderpath = folderpath.parent_path() / newname;

    if (newname.empty() || !is_path_in_folder(newfolderpath, user_root)
        || (newname.at(0) == '/' || newname.at(0) == '\\' || newname.compare("..") == 0)) {
        send_error_message(serverSocket, clientSockfd, RENAME_FOLDER_COMMAND, "Invalid directory name.");
        return false;
    }

    if (fs::is_directory(newfolderpath)) {
        send_error_message(serverSocket, clientSockfd, RENAME_FOLDER_COMMAND, "A directory with the same name already exists.");
        return false;
    }

    // rename directory

    try {
        fs::rename(folderpath, newfolderpath);
        bool status = 1;
        LPTF_Packet reply = build_reply_packet(RENAME_FOLDER_COMMAND, &status, sizeof(status));
        serverSocket->send(clientSockfd, reply, 0);
        return true;
    } catch(const fs::filesystem_error &ex) {
        send_error_message(serverSocket, clientSockfd, RENAME_FOLDER_COMMAND, ex.what());
        return false;
    }
}