#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

uint32_t get_file_size(string filepath);
uint32_t get_file_size(fs::path filepath);

string list_directory_content(fs::path folderpath);

void check_server_root_folder();
void check_user_root_folder(string username);

fs::path get_server_root();
fs::path get_user_root(string username);

#endif