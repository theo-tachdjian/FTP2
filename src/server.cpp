#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "../include/LPTF_Net/LPTF_Socket.hpp"
#include "../include/LPTF_Net/LPTF_Utils.hpp"
#include "../include/commands.hpp"

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

// Etape 1
// void handle_client(LPTF_Socket *serverSocket, int clientSockfd, struct sockaddr_in clientAddr, socklen_t clientAddrLen) {
//     cout << "Handling client: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " (" << clientAddrLen << ")" << endl;

//     try {
//         // listen for "ping" message
//         LPTF_Packet msg = serverSocket->recv(clientSockfd, 0);
//         string content = get_message_from_message_packet(msg);

//         cout << "Received: " << content << endl;

//         LPTF_Packet resp;

//         if (strcmp(content.c_str(), "ping") == 0) {
//             cout << "Sending pong message." << endl;
//             resp = build_message_packet("pong");
//         } else {
//             cout << "Sending pong? message." << endl;
//             resp = build_message_packet("pong?");
//         }

//         ssize_t ret = serverSocket->send(clientSockfd, resp, 0);

//         if (ret == -1) { cout << "Message not sent !" << endl; }

//     } catch (const exception &ex) {
//         cout << "Error when handling client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " : " << ex.what() << endl;    
//     }

//     cout << "Closing client connection" << endl;
//     close(clientSockfd);
// }

void handle_client(LPTF_Socket *serverSocket, int clientSockfd, struct sockaddr_in clientAddr, socklen_t clientAddrLen) {
    cout << "Handling client: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " (" << clientAddrLen << ")" << endl;

    try {
        // listen for commands

        LPTF_Packet req = serverSocket->recv(clientSockfd, 0);
        if (req.type() == COMMAND_PACKET) {
            string cmd = get_command_from_command_packet(req);

            cout << "Received command " << cmd << endl;

            if (strcmp(cmd.c_str(), UPLOAD_FILE_COMMAND) == 0) {
                FILE_UPLOAD_REQ_PACKET_STRUCT transfer_args = get_data_from_file_upload_request_packet(req);

                cout << "Arguments: " << transfer_args.filepath << ", " << transfer_args.filesize << endl;
                // req.print_specs();

                receive_file(serverSocket, clientSockfd, transfer_args.filepath, transfer_args.filesize);

            } else if (strcmp(cmd.c_str(), DOWNLOAD_FILE_COMMAND) == 0) {
                string filepath = get_file_from_file_download_request_packet(req);

                cout << "File to send: " << filepath << endl;

                send_file(serverSocket, clientSockfd, filepath);
            } else {
                
            }
        }

    } catch (const exception &ex) {
        cout << "Error when handling client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " : " << ex.what() << endl;    
    }

    cout << "Closing client connection" << endl;
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