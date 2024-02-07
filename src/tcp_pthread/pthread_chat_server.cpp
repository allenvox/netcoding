#include <iostream>
#include <vector>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>

const int MAX_CLIENTS = 10;
int PORT = 0; // Здесь будем хранить порт

std::vector<int> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handleClient(void *arg) {
    int clientSocket = *((int *)arg);
    char buffer[1024];

    // Принимаем имя пользователя
    int bytesReceived = recv(clientSocket, buffer, 1024, 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving username\n";
        close(clientSocket);
        pthread_exit(nullptr);
    }

    std::string username(buffer, bytesReceived); // -1 to remove '\n'
    std::cout << username << " connected\n";

    while (true) {
        bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived <= 0) {
            // Клиент отключился
            pthread_mutex_lock(&clients_mutex);
            auto it = std::find(clients.begin(), clients.end(), clientSocket);
            if (it != clients.end()) {
                clients.erase(it);
            }
            pthread_mutex_unlock(&clients_mutex);
            close(clientSocket);
            std::cout << username << " disconnected\n";
            pthread_exit(nullptr);
        }

        // Отправка сообщения всем клиентам
        pthread_mutex_lock(&clients_mutex);
        for (int client : clients) {
            if (client != clientSocket) {
                send(client, buffer, bytesReceived, 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Отображение сообщения на сервере
        buffer[bytesReceived] = '\0';
        std::cout << "[chat] " << buffer << '\n';
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket\n";
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(0); // Здесь указываем 0 для автоматического выбора порта

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket\n";
        return 1;
    }

    // Получаем порт, который был назначен автоматически
    socklen_t len = sizeof(serverAddr);
    getsockname(serverSocket, (struct sockaddr *)&serverAddr, &len);
    PORT = ntohs(serverAddr.sin_port);
    std::cout << "Server started. Listening on port " << PORT << std::endl;

    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        std::cerr << "Error listening on socket\n";
        return 1;
    }

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        clients.push_back(clientSocket);
        pthread_mutex_unlock(&clients_mutex);

        pthread_t thread;
        if (pthread_create(&thread, nullptr, handleClient, (void *)&clientSocket) != 0) {
            std::cerr << "Error creating thread\n";
        }
    }
    close(serverSocket);
    return 0;
}
