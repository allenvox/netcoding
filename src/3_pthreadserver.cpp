#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fstream>

const int BACKLOG = 5;
const int BUF_SIZE = 64;

// For transferring data to filestream
struct ThreadData {
    int client_socket;
    std::ofstream* file_stream;
};

void* handle_client(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    int client_socket = data->client_socket;
    std::ofstream* file_stream = data->file_stream;

    char buffer[BUF_SIZE];
    int num_bytes_recv;

    // Read, print & write client data to file
    while ((num_bytes_recv = recv(client_socket, buffer, BUF_SIZE - 1, 0)) > 0) {
        buffer[num_bytes_recv] = '\0';
        if (!(*file_stream << buffer << std::endl)) {
            std::cerr << "Failed to write to file\n";
        }
        std::cout << "pthread_server: Written message \"" << buffer << "\" from client\n";
    }

    if (num_bytes_recv == 0) {
        std::cout << "Connection closed by client\n";
    } else if (num_bytes_recv == -1) {
        std::cerr << "Failed receiving client data\n";
    }

    // Close socket, free memory
    close(client_socket);
    delete data;
    pthread_exit(NULL);
}

int main() {
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Configure a socket
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        std::cerr << "Failed to set socket options\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = 0;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind socket to address & port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        std::cerr << "Failed to bind socket\n";
        return 1;
    }

    // Get & print server port
    socklen_t len = sizeof(server_addr);
    getsockname(sockfd, (struct sockaddr *)&server_addr, &len);
    std::cout << "Server started on port: " << ntohs(server_addr.sin_port)
              << std::endl;

    if (listen(sockfd, BACKLOG) == -1) {
        std::cerr << "Failed to listen input message\n";
        return 1;
    }

    // Output file
    std::ofstream file("data.txt", std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    // Handle in-connections
    while (true) {
        // Accept in-connection
        struct sockaddr_in client_addr;
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_socket;
        if ((client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            std::cerr << "Failed accepting client request\n";
            continue;
        }
        std::cout << "Connection from " << inet_ntoa(client_addr.sin_addr) << '\n';

        // Create a pthread to handle client connection
        pthread_t thread;
        ThreadData* data = new ThreadData;
        data->client_socket = client_socket;
        data->file_stream = &file;
        if (pthread_create(&thread, NULL, handle_client, data) != 0) {
            std::cerr << "Failed to create thread\n";
            delete data;
            close(client_socket);
            continue;
        }
    }
    // Close socket & out file
    close(sockfd);
    file.close();
    return 0;
}
