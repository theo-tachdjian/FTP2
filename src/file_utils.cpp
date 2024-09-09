#include "../include/file_utils.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>

#define SERVER_DIR "server_root"
#define SERVER_LOGS_DIR "logs"

using namespace std;
namespace fs = std::filesystem;


uint32_t get_file_size(string filepath) {
    return get_file_size(fs::path(filepath));
}


uint32_t get_file_size(fs::path filepath) {
    return static_cast<uint32_t>(fs::file_size(filepath));
}


string list_directory_content(fs::path folderpath) {

    const char *entry_prefix[] = {
        "<FILE>\t",
        "<DIR>\t"
    };

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

    return result;
}


void check_server_root_folder() {
    fs::path sroot(SERVER_DIR);

    if (!fs::is_directory(sroot))
        if (!fs::create_directories(sroot))
            throw runtime_error("create_directories() failed!");
}


void check_user_root_folder(string username) {
    fs::path uroot = get_server_root();
    uroot /= username;

    if (!fs::is_directory(uroot))
        if (!fs::create_directories(uroot))
            throw runtime_error("create_directories() failed!");
}


fs::path get_server_root() {
    check_server_root_folder();
    return fs::path(SERVER_DIR);
}


fs::path get_user_root(string username) {
    check_user_root_folder(username);
    fs::path uroot = get_server_root();
    uroot /= username;
    return uroot;
}


void check_server_logs_folder() {
    fs::path lroot(SERVER_LOGS_DIR);

    if (!fs::is_directory(lroot))
        if (!fs::create_directories(lroot))
            throw runtime_error("create_directories() failed!");
}


fs::path get_server_logs_folder() {
    check_server_logs_folder();
    return fs::path(SERVER_LOGS_DIR);
}


bool is_path_in_folder(fs::path contained, fs::path container) {
    // compare the relative path for contained and container
    fs::path relative_path = std::filesystem::relative(contained, container);
    return !relative_path.empty() && relative_path.native()[0] != '.';
}


void delete_directory_content(fs::path dir) {
    for (fs::directory_entry const& dir_entry : fs::directory_iterator(dir))
        fs::remove_all(dir_entry);
}