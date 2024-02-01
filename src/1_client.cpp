#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

const int BUF_SIZE = 64;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>\n";
    return 1;
  }

  const char *server_ip = argv[1];
  int server_port = atoi(argv[2]);

  // Create a socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    std::cerr << "Failed to create socket\n";
    return 1;
  }

  // Fill socket address structure for server
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;          // IPv4
  servaddr.sin_port = htons(server_port); // Set port to server_port
  if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) != 1) {
    std::cerr << "Invalid address\n";
    close(sockfd);
    return 1;
  }

  for (int i = 0; i < 5; ++i) { // 5 packages
    // Create package message
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "Message %d from client", i);

    // Send data to server
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&servaddr,
               sizeof(servaddr)) == -1) {
      std::cerr << "Error in sendto\n";
      close(sockfd);
      return 1;
    }

    // Get response from server
    memset(buffer, 0, BUF_SIZE);
    socklen_t len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&servaddr,
                     &len);
    if (n == -1) {
      std::cerr << "Error in recvfrom\n";
      close(sockfd);
      return 1;
    }

    // Print received info
    std::cout << "Received from server: " << buffer << std::endl;
    sleep(i); // Delay ('i' seconds)
  }
  close(sockfd);
  return 0;
}