#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const int BUF_SIZE = 1024;

int main() {
  // Create a socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == -1) {
    std::cerr << "Failed to create socket\n";
    return 1;
  }

  // Fill server structure
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;                // Use IPv4
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any IP address
  servaddr.sin_port = 0;                        // OS will find a free port

  // Bind socket to address and port
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    std::cerr << "Bind failed\n";
    close(sockfd);
    return 1;
  }

  // Get & print port number
  socklen_t len = sizeof(servaddr);
  getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
  std::cout << "Server started on port: " << ntohs(servaddr.sin_port)
            << std::endl;

  struct sockaddr_in cliaddr;
  socklen_t clilen = sizeof(cliaddr);

  char buffer[BUF_SIZE];
  while (true) {
    // Get data from any client
    int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&cliaddr,
                     &clilen);
    if (n == -1) {
      std::cerr << "Error in recvfrom\n";
      close(sockfd);
      return 1;
    }

    // Print client info
    std::cout << "Received from " << inet_ntoa(cliaddr.sin_addr) << ':'
              << ntohs(cliaddr.sin_port) << " : " << buffer << std::endl;

    // Send client's message back
    if (sendto(sockfd, buffer, n, 0, (struct sockaddr *)&cliaddr, clilen) ==
        -1) {
      std::cerr << "Error in sendto\n";
      close(sockfd);
      return 1;
    }
  }
  close(sockfd);
  return 0;
}
