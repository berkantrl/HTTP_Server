#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Usage: %s host port filepath\n", argv[0]);
        exit(1);
    }

    // Parse command line arguments
    char *host = argv[1];
    int port = atoi(argv[2]);
    char *filepath = argv[3];

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    // Look up the IP address of the host
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    // Set up the server address and port
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting to server");
    }

    // Send the HTTP request
    char request[BUFFER_SIZE];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", filepath, host);
    if (write(sockfd, request, strlen(request)) < 0) {
        error("ERROR writing to socket");
    }

    // Read the response
    char response[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = read(sockfd, response, BUFFER_SIZE - 1)) > 0) {
        response[bytes_read] = '\0';
        printf("%s", response);
    }

    // Close the socket
    close(sockfd);

    return 0;
}


// To compile and run this code, use the following commands:

// gcc http_client.c -o http_client
// ./http_client [host] [port number] [filepath]