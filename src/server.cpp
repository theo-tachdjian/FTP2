#include <iostream>

#include <vector>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include <sstream>

#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/server_actions.hpp"
#include "../include/file_utils.hpp"
#include "../include/logger.hpp"

using namespace std;


// borrowed from https://www.geeksforgeeks.org/thread-pool-in-cpp/
class ThreadPool { 
public: 
    ThreadPool(size_t num_threads = thread::hardware_concurrency()) {
        // Creating worker threads
        for (size_t i = 0; i < num_threads; ++i) { 
            threads_.emplace_back([this] { 
                while (true) { 
                    function<void()> task; 
                    { 
                        // Lock queue so that data 
                        // can be shared safely
                        unique_lock<mutex> lock( 
                            queue_mutex_); 
  
                        // Waiting until there is a task to 
                        // execute or the pool is stopped
                        cv_.wait(lock, [this] { 
                            return !tasks_.empty() || stop_; 
                        }); 
  
                        // exit the thread in case the pool 
                        // is stopped and there are no tasks 
                        if (stop_ && tasks_.empty()) { 
                            return; 
                        } 
  
                        // Get the next task from the queue 
                        task = move(tasks_.front()); 
                        tasks_.pop(); 
                    } 
  
                    task(); 
                } 
            }); 
        } 
    } 
  
    ~ThreadPool() 
    { 
        {
            // Lock the queue to update the stop flag safely 
            unique_lock<mutex> lock(queue_mutex_); 
            stop_ = true; 
        }

        cv_.notify_all(); 
  
        // Joining all worker threads to ensure they have 
        // completed their tasks
        for (auto& thread : threads_) { 
            thread.join(); 
        } 
    } 
  
    // Enqueue task for execution by the thread pool 
    void enqueue(function<void()> task) 
    { 
        { 
            unique_lock<mutex> lock(queue_mutex_); 
            tasks_.emplace(move(task)); 
        } 
        cv_.notify_one(); 
    } 
  
private: 
    // Vector to store worker threads 
    vector<thread> threads_;
    // Queue of tasks 
    queue<function<void()> > tasks_;
    // Mutex to synchronize access to shared data 
    mutex queue_mutex_;
    // Condition variable to signal changes in the state of 
    // the tasks queue 
    condition_variable cv_;
    // Flag to indicate whether the thread pool should stop 
    // or not 
    bool stop_ = false;
};


string wait_for_login(LPTF_Socket *serverSocket, int clientSockfd) {
    // wait for login

    while (true) {
        // listen for login packet
        LPTF_Packet pckt = serverSocket->recv(clientSockfd, 0);

        if (pckt.type() == LOGIN_PACKET) {
            try {
                // string username = "Erwan";
                string username = string((const char *)pckt.get_content(), pckt.get_header().length);

                // create folders
                check_user_root_folder(username);

                pckt = build_reply_packet(LOGIN_PACKET, (void*)"OK", 2);
                serverSocket->send(clientSockfd, pckt, 0);

                return username;
            } catch (const exception &ex) {
                string err_msg;
                err_msg.append("Error when logging in client: ");
                err_msg.append(ex.what());

                LPTF_Packet error_pckt = build_error_packet(pckt.type(), ERR_CMD_UNKNOWN, err_msg);
                serverSocket->send(clientSockfd, error_pckt, 0);
                
                cout << err_msg << endl;
                return "";
            }
        } else {
            string err_msg = "You must log in to perform this action.";
            LPTF_Packet error_pckt = build_error_packet(pckt.type(), ERR_CMD_UNKNOWN, err_msg);
            serverSocket->send(clientSockfd, error_pckt, 0);

            return "";
        }
    }
}


Logger *get_user_logger(string username) {
    try {
        return new Logger(get_server_logs_folder() / (username + ".txt"));
    } catch (const runtime_error &ex) {
        cerr << "Logger not available: " << ex.what() << endl;
    }
    return nullptr;
}


void execute_command(LPTF_Socket *serverSocket, int clientSockfd, LPTF_Packet &req, string username, Logger *logger) {

    log_info("Received command packet", logger);

    switch (req.type()) {
        case UPLOAD_FILE_COMMAND:
        {
            FILE_UPLOAD_REQ_PACKET_STRUCT transfer_args = get_data_from_file_upload_request_packet(req);

            ostringstream msg;
            msg << "UPLOAD_FILE_COMMAND: \"" << transfer_args.filepath << "\", " << transfer_args.filesize;
            log_info(msg, logger);

            receive_file(serverSocket, clientSockfd, transfer_args.filepath, transfer_args.filesize, username, logger);
            break;
        }
        case DOWNLOAD_FILE_COMMAND:
        {
            string filepath = get_file_from_file_download_request_packet(req);

            ostringstream msg;
            msg << "DOWNLOAD_FILE_COMMAND: \"" << filepath << "\"";
            log_info(msg, logger);

            send_file(serverSocket, clientSockfd, filepath, username, logger);
            break;
        }
        
        case DELETE_FILE_COMMAND:
        {
            string filepath = get_file_from_file_delete_request_packet(req);

            ostringstream msg;
            msg << "DELETE_FILE_COMMAND: \"" << filepath << "\"";
            log_info(msg, logger);

            delete_file(serverSocket, clientSockfd, filepath, username, logger);
            break;
        }
        
        case LIST_FILES_COMMAND:
        {
            string path = get_path_from_list_directory_request_packet(req);

            ostringstream msg;
            msg << "LIST_FILES_COMMAND: \"" << path << "\"";
            log_info(msg, logger);

            list_directory(serverSocket, clientSockfd, path, username, logger);
            break;
        }
        
        case CREATE_FOLDER_COMMAND:
        {
            CREATE_DIR_REQ_PACKET_STRUCT args = get_data_from_create_directory_request_packet(req);

            ostringstream msg;
            msg << "CREATE_FOLDER_COMMAND: \"" << args.dirname << "\", \"" << args.path << "\"";
            log_info(msg, logger);

            create_directory(serverSocket, clientSockfd, args.dirname, args.path, username, logger);
            break;
        }
        
        case DELETE_FOLDER_COMMAND:
        {
            string folder = get_path_from_remove_directory_request_packet(req);

            ostringstream msg;
            msg << "DELETE_FOLDER_COMMAND: \"" << folder << "\"";
            log_info(msg, logger);

            remove_directory(serverSocket, clientSockfd, folder, username, logger);
            break;
        }
        
        case RENAME_FOLDER_COMMAND:
        {
            RENAME_DIR_REQ_PACKET_STRUCT args = get_data_from_rename_directory_request_packet(req);

            ostringstream msg;
            msg << "RENAME_FOLDER_COMMAND: \"" << args.path << "\", \"" << args.newname << "\"";
            log_info(msg, logger);

            rename_directory(serverSocket, clientSockfd, args.newname, args.path, username, logger);
            break;
        }
        
        default:
        {
            log_error("Got unexpected command from client", logger);

            string err_msg = "Not Implemented";
            LPTF_Packet err_pckt = build_error_packet(req.type(), ERR_CMD_UNKNOWN, err_msg);
            serverSocket->send(clientSockfd, err_pckt, 0);
            break;
        }
    }
}


void handle_client(LPTF_Socket *serverSocket, int clientSockfd, struct sockaddr_in clientAddr, socklen_t clientAddrLen) {
    cout << "Handling client: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " (len:" << clientAddrLen << ")" << endl;

    string username;

    try {
        username = wait_for_login(serverSocket, clientSockfd);

        if (username.size() == 0) {
            cout << "Closing client connection" << endl;
            close(clientSockfd);
            return;
        }

        cout << "Client logged in as \"" << username << "\"" << endl;
    } catch (const exception &ex) {
        cout << "Error on client login: " << ex.what() << endl << "Closing client connection" << endl;
        close(clientSockfd);
        return;
    }

    // username = "Erwan";

    // can be null
    Logger *logger = get_user_logger(username);
    
    ostringstream msg;
    msg << "User \"" << username << "\": " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " (len:" << clientAddrLen << ")";
    log_info(msg, logger);

    try {
        // listen for command

        LPTF_Packet req = serverSocket->recv(clientSockfd, 0);

        if (is_command_packet(req)) {

            execute_command(serverSocket, clientSockfd, req, username, logger);

        } else {
            ostringstream msg;
            msg << "Got non-command packet from client: \"" << req.type() << "\"";
            log_error(msg, logger);
            
            string err_msg = "Not Implemented";
            LPTF_Packet err_pckt = build_error_packet(req.type(), ERR_CMD_UNKNOWN, err_msg);
            serverSocket->send(clientSockfd, err_pckt, 0);
        }

    } catch (const exception &ex) {
        ostringstream msg;
        msg << "Error when handling client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " : " << ex.what();
        log_error(msg, logger);
    }

    log_info("Closing client connection", logger);
    close(clientSockfd);
}

int main() {
    int port = 12345;
    int max_clients = 10;

    try {
        ThreadPool clientPool(max_clients);

        LPTF_Socket serverSocket = LPTF_Socket();

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        serverSocket.bind(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
        serverSocket.listen(max_clients);   // limit number of clients

        cout << "Server running: " << inet_ntoa(serverAddr.sin_addr) << ":" << ntohs(serverAddr.sin_port) << endl;

        while (true) {
            cout << "Waiting for new client..." << endl;
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSockfd = serverSocket.accept(reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);

            if (clientSockfd == -1) throw runtime_error("Error on accept connection !");

            clientPool.enqueue([&serverSocket, clientSockfd, clientAddr, clientAddrLen] { handle_client(&serverSocket, clientSockfd, clientAddr, clientAddrLen); });

        }

    } catch (const exception &ex) {
        cerr << "Exception: " << ex.what() << endl;
        return 1;
    }

    return 0;
}