#include "../include/file_utils.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>

#define SERVER_ROOT "server_root"

using namespace std;
namespace fs = std::filesystem;


uint32_t get_file_size(string filepath) {
    streampos begin, end;
    ifstream f (filepath, ios::binary);
    begin = f.tellg();
    f.seekg (0, ios::end);
    end = f.tellg();
    f.close();
    return static_cast<uint32_t>(end-begin);
}


uint32_t get_file_size(fs::path filepath) {
    return get_file_size(filepath.generic_string());
}


void check_server_root_folder() {
    fs::path sroot(SERVER_ROOT);

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
    return fs::path(SERVER_ROOT);
}


fs::path get_user_root(string username) {
    check_user_root_folder(username);
    fs::path uroot = get_server_root();
    uroot /= username;
    return uroot;
}
