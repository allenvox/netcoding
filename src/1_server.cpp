#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

const int BUF_SIZE = 1024;

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Создание сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Заполнение структуры сервера
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 0; // Поиск свободного порта

    // Привязка сокета к адресу и порту
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Bind failed\n";
        close(sockfd);
        return 1;
    }

    // Получение номера порта
    socklen_t len = sizeof(servaddr);
    getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
    std::cout << "Server started on port: " << ntohs(servaddr.sin_port) << std::endl;

    char buffer[BUF_SIZE];
    socklen_t clilen = sizeof(cliaddr);

    while (true) {
        // Получение данных от клиента
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &clilen);
        if (n == -1) {
            std::cerr << "Error in recvfrom\n";
            close(sockfd);
            return 1;
        }

        // Отображение информации о клиенте
        std::cout << "Received from " << inet_ntoa(cliaddr.sin_addr) << ':' << ntohs(cliaddr.sin_port) << " : " << buffer << std::endl;

        // Отправка преобразованной информации обратно клиенту
        if (sendto(sockfd, buffer, n, 0, (struct sockaddr *)&cliaddr, clilen) == -1) {
            std::cerr << "Error in sendto\n";
            close(sockfd);
            return 1;
        }
    }
    close(sockfd);
    return 0;
}




