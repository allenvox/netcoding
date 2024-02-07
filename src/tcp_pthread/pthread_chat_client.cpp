#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int clientSocket;
pthread_t receiveThread;
std::string username;

void *receiveMessages(void *arg) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Error receiving message from server\n";
            break;
        }
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }
    close(clientSocket);
    pthread_exit(nullptr);
}

void *sendMessage(void *arg) {
    while (true) {
        std::string text;
        std::getline(std::cin, text);
        std::cout << "\033[A\33[2K\r";
        std::string message = username + ": " + text;
        if (send(clientSocket, message.c_str(), message.size(), 0) == -1) {
            std::cerr << "Error sending message\n";
            break;
        }
        std::cout << message << '\n';
    }
    pthread_cancel(receiveThread);
    close(clientSocket);
    pthread_exit(nullptr);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port> <username>\n";
        return 1;
    }
    const char *serverIp = argv[1];
    int PORT = atoi(argv[2]);
    username = argv[3];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server\n";
        return 1;
    }
    std::cout << "[server] Connected\n";

    // Отправляем имя пользователя на сервер
    send(clientSocket, username.c_str(), username.size(), 0);

    pthread_create(&receiveThread, nullptr, receiveMessages, nullptr);
    pthread_t sendThread;
    pthread_create(&sendThread, nullptr, sendMessage, nullptr);
    pthread_join(receiveThread, nullptr);
    pthread_cancel(sendThread);
    return 0;
}
