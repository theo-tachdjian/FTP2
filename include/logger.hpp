#pragma once

#include <iostream>
#include <fstream>

#define LOG_DEFAULT_SIZE_LIMIT 16384

using namespace std;

class Logger {

private:
    unsigned int size_limit;
    string filename;
    ofstream log_file;

    virtual void log(const string &level, const string &message);
    virtual void check_purge();

public:
    Logger();
    Logger(const string &filename);
    Logger(const string &filename, unsigned int size_limit);

    ~Logger();

    void info(const string &message);
    void info(const ostringstream &message);
    void debug(const string &message);
    void debug(const ostringstream &message);
    void warn(const string &message);
    void warn(const ostringstream &message);
    void error(const string &message);
    void error(const ostringstream &message);

};

void log_info(ostringstream &msg, Logger *logger);
void log_info(const string &msg, Logger *logger);
void log_debug(ostringstream &msg, Logger *logger);
void log_debug(const string &msg, Logger *logger);
void log_warn(ostringstream &msg, Logger *logger);
void log_warn(const string &msg, Logger *logger);
void log_error(ostringstream &msg, Logger *logger);
void log_error(const string &msg, Logger *logger);