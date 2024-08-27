#include <iostream>
#include <unistd.h>

#include <fstream>

#include <pwd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sstream>

#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"


#define SHELL_BUFFER_SIZE 1024


std::string get_host_info() {
    struct utsname info;
    if (uname(&info) != 0) {
        return "Failed to retrieve host information";
    }
    std::string hostInfo = "Hostname: " + std::string(info.nodename) + "\n";
    hostInfo += "Username: " + std::string(getpwuid(getuid())->pw_name) + "\n";
    hostInfo += "Operating System: " + std::string(info.sysname);
    return hostInfo;
}

std::string get_running_processes() {
    std::stringstream processList;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("/proc")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            const char *d_name = ent->d_name;
            if (isdigit(d_name[0])) {
                processList << d_name;

                // open status file to get the process name
                
                std::string status_fp("/proc/");
                status_fp += d_name;
                status_fp += "/status";
                std::ifstream status_f(status_fp);

                std::string line;
                while (getline (status_f, line)) {
                    if (strncmp("Name:", line.c_str(), 5) == 0) {
                        processList << " " << line.c_str()+5;
                        break;
                    }
                }

                processList << std::endl;
            }
        }
        closedir(dir);
    } else {
        return "Failed to retrieve running processes";
    }
    return processList.str();
}

std::string execute_shell_command(const std::string &command) {
    std::stringstream result;
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "Failed to execute command";
    }
    char buffer[SHELL_BUFFER_SIZE];
    while (!feof(pipe)) {
        if (fgets(buffer, SHELL_BUFFER_SIZE, pipe) != NULL) {
            result << buffer;
        }
    }
    pclose(pipe);
    return result.str();
}