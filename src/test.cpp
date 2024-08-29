#include "../include/file_utils.hpp"
#include <iostream>
#include <fstream>

#include <filesystem>

using namespace std;

namespace fs = std::filesystem;


int main()
{
    fs::path uroot = get_user_root("TESTUSER");

    cout << "fs::exists(uroot): " << fs::exists(uroot) << endl;
    cout << "fs::is_directory(uroot): " << fs::is_directory(uroot) << endl;

    fs::path filepath = uroot;
    filepath /= "test-out.bin";

    cout << "filepath: " << filepath<<endl;

    ofstream outfile;
    outfile.open(filepath, ios::binary | ios::out);

    cout << "outfile open: " << outfile.is_open() << endl;
    cout << "outfile tellp: " << outfile.tellp() << endl;

    outfile.write("abc!", 4);

    outfile.close();

    string path = "server_root/Erwan/hello.txt";
    cout << path << " size: " << get_file_size(path) << endl;

    fs::path fspath = ("server_root/Erwan/hello.txt");
    cout << path << " size: " << get_file_size(fspath) << endl;

    cout << "filepath.native(): "<< filepath.native() << endl;

    filepath.append("abc");
    cout << "filepath.append(\"abc\"): " << filepath << endl;

    return 0;
}
