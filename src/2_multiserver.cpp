#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

const int BACKLOG = 5;
const int BUF_SIZE = 64;

void sigchld_handler(int signum) {
    // To avoid creating zombie-processes
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

int main() {
    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Configure socket
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

    // Get & print port number
    socklen_t len = sizeof(server_addr);
    getsockname(sockfd, (struct sockaddr *)&server_addr, &len);
    std::cout << "Server started on port: " << ntohs(server_addr.sin_port)
              << std::endl;

    // Listen to input messages
    if (listen(sockfd, BACKLOG) == -1) {
        std::cerr << "Failed to listen input message\n";
        return 1;
    }

    // Set SIGCHLD signal handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (true) {
        socklen_t sin_size = sizeof(struct sockaddr_in);

        // Accept input connection
        int new_fd;
        struct sockaddr_in client_addr;
        if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

        std::cout << "Server: got connection from " << inet_ntoa(client_addr.sin_addr) << '\n';

        // Create child process to handle client request
        if (!fork()) {
            close(sockfd); // Close socket read descriptor in child proc

            // Read data from client
            char buffer[BUF_SIZE];
            int num_bytes_recv;
            while ((num_bytes_recv = recv(new_fd, buffer, BUF_SIZE - 1, 0)) > 0) {
                buffer[num_bytes_recv] = '\0';
                std::cout << "Received: " << buffer << std::endl;
            }

            if (num_bytes_recv == 0) {
                std::cout << "Connection closed by client." << std::endl;
            } else if (num_bytes_recv == -1) {
                perror("recv");
            }

            close(new_fd); // Close child proc's socket
            exit(0);
        }
        close(new_fd); // Close socket read descriptor in parent proc
    }
    return 0;
}