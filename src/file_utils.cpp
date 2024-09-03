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


bool is_path_in_folder(fs::path contained, fs::path container) {
    // compare the relative path for contained and container
    fs::path relative_path = std::filesystem::relative(contained, container);
    return !relative_path.empty() && relative_path.native()[0] != '.';
}