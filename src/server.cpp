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


// borrowed from https://www.geeksforgeeks.org/thread-pool-in-cpp/
class ClientPool { 
public: 
    ClientPool(size_t num_threads = std::thread::hardware_concurrency()) {
        std::cout << "Num Threads: " << num_threads << std::endl;
        // Creating worker threads
        for (size_t i = 0; i < num_threads; ++i) { 
            threads_.emplace_back([this] { 
                while (true) { 
                    std::function<void()> task; 
                    // The reason for putting the below code 
                    // here is to unlock the queue before 
                    // executing the task so that other 
                    // threads can perform enqueue tasks 
                    { 
                        // Locking the queue so that data 
                        // can be shared safely 
                        std::unique_lock<std::mutex> lock( 
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
  
    ~ClientPool() 
    { 
        { 
            // Lock the queue to update the stop flag safely 
            std::unique_lock<std::mutex> lock(queue_mutex_); 
            stop_ = true; 
        } 
  
        // Notify all threads 
        cv_.notify_all(); 
  
        // Joining all worker threads to ensure they have 
        // completed their tasks 
        for (auto& thread : threads_) { 
            thread.join(); 
        } 
    } 
  
    // Enqueue task for execution by the thread pool 
    void enqueue(std::function<void()> task) 
    { 
        { 
            std::unique_lock<std::mutex> lock(queue_mutex_); 
            tasks_.emplace(std::move(task)); 
        } 
        cv_.notify_one(); 
    } 
  
private: 
    // Vector to store worker threads 
    std::vector<std::thread> threads_;
  
    // Queue of tasks 
    std::queue<std::function<void()> > tasks_;
  
    // Mutex to synchronize access to shared data 
    std::mutex queue_mutex_;
  
    // Condition variable to signal changes in the state of 
    // the tasks queue 
    std::condition_variable cv_;
  
    // Flag to indicate whether the thread pool should stop 
    // or not 
    bool stop_ = false;
}; 


class Server {
private:
    std::unique_ptr<LPTF_Socket> serverSocket;
    int maxClients = 10;

public:
    Server(int port) {
        serverSocket = std::make_unique<LPTF_Socket>();

        struct sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        serverSocket->bind(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
        serverSocket->listen(maxClients);   // limit number of clients
    }

    void start() {
        ClientPool clientPool(maxClients);

        while (true) {
            std::cout << "Waiting for new client..." << std::endl;
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientSockfd = serverSocket->accept(reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);

            if (clientSockfd == -1) throw std::runtime_error("Error on accept connection !");

            // handle_client(clientSockfd, clientAddr, clientAddrLen);

            clientPool.enqueue([this, clientSockfd, clientAddr, clientAddrLen] { handle_client(clientSockfd, clientAddr, clientAddrLen); });

        }
    }

    void handle_client(int clientSockfd, struct sockaddr_in clientAddr, socklen_t clientAddrLen) {
        std::cout << "Handling client: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " (" << clientAddrLen << ")" << std::endl;

        try {
            // listen for "ping" message
            LPTF_Packet msg = serverSocket->recv(clientSockfd, 0);
            std::string content = get_message_from_message_packet(msg);

            std::cout << "Received: " << content << std::endl;

            LPTF_Packet resp;

            if (strcmp(content.c_str(), "ping") == 0) {
                std::cout << "Sending pong message." << std::endl;
                resp = build_message_packet("pong");
            } else {
                std::cout << "Sending pong? message." << std::endl;
                resp = build_message_packet("pong?");
            }

            ssize_t ret = serverSocket->send(clientSockfd, resp, 0);

            if (ret == -1) { std::cout << "Message not sent !" << std::endl; }

        } catch (const std::exception &ex) {
            std::cout << "Error when handling client " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << " : " << ex.what() << std::endl;    
        }

        std::cout << "Closing client connection" << std::endl;
        close(clientSockfd);
    }
};

int main() {
    try {
        Server server(12345);
        server.start();
    } catch (const std::exception &ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}