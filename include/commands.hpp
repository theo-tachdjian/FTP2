#ifndef COMMANDS_H
#define COMMANDS_H

#include <iostream>

std::string get_host_info();
std::string get_running_processes();
std::string execute_shell_command(const std::string &command);

#endif