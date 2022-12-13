#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 2048  // max number of bytes we can get at once
#define BUFFER_SIZE 4096 // size of each UDP packet
#define DONE "DONE" // signal to stop receiving
#define DELIM "/?key=" // delimiter to split search string
#define DBADDR "127.0.0.1"

void get_file_from_database(const char* search_string, const char* database_ip, int database_port, int client_socket);

int main(int argc, char* argv[]) {
  int listenfd, connfd;  // listen on listenfd, new connection on connfd
  struct sockaddr_in servaddr;  // server address
  // check command line arguments
  if (argc != 3) {
    fprintf(stderr, "usage: %s <port> <DBport>\n", argv[0]);
    exit(1);
  }

  // get the port and DBport from the command line arguments
  int port = atoi(argv[1]);
  int DBport = atoi(argv[2]);
  char* web_root = "Webpage";

  // create a socket to listen on
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket");
    exit(1);
  }

  // set socket options
  int optval = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) == -1) {
    perror("Error setting socket options");
    exit(1);
  }

  // set up the server address
  bzero((char *) &servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((unsigned short)port);

  // bind the socket to the server address
  if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
    perror("Error binding socket to address");
    exit(1);
  }

  // listen for incoming connections
  if (listen(listenfd, 10) == -1) {
    perror("Error listening for connections");
    exit(1);
  }

  printf("Listening on port %d...\n", port);

  // loop forever, accepting client connections one at a time
  while (1) {
    // accept a new client connection
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    if ((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_addr_len)) == -1) {
      perror("Error accepting connection");
      exit(1);
    }

        // read the client's request
        char request[1024]; // Create an array for the request message

        // Get the request message using recv() requests
        int bytes_received = recv(connfd, request, sizeof(request), 0);
        if (bytes_received < 0) {
            perror("Error reading from socket");
            exit(1);
        }

        char* token = strtok(request, " ");
        if (strcmp(token, "GET") == 0)
        {
            char cli_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &cli_addr.sin_addr, cli_ip_str, INET_ADDRSTRLEN);
            printf("%s ", cli_ip_str);
            printf("%s",token);
            token = strtok(NULL, " ");
            char* path = token;
            printf(" %s ", token);
            char first_char = token[0];
            token = strtok(NULL, " ");
            printf("%s",token);
            // Get the first character to check if its first letter is '/'
            if (first_char != '/' && first_char != ' ') {
                // send a response
                char* response = "HTTP/1.1 400 Bad Request\r\n\r\n";
                bytes_received = write(connfd, response, strlen(response));
                printf(" 400 Bad Request\n");
            }
            if (path[1] == '?'){
                char* term = strtok(path, DELIM);
                char search_string[100] = "";
                strcpy(search_string, term);
                while ((term = strtok(NULL, "+")) != NULL) {
                    strncat(search_string, term, strlen(term));
                }
                get_file_from_database(term,DBADDR,DBport,connfd);
            }
            
            char file_path[1024];
            sprintf(file_path, "%s%s", web_root, path);
            FILE* file = fopen(file_path, "r");
            if (file != NULL) {
                // send a response
                char buffer[1024];
                char html[1024] = "";
                while (fgets(buffer, sizeof(buffer), file) != NULL) {
                    // Add the read data to the html variable
                    strcat(html, buffer);
                }
                fclose(file);
                char response[4096];
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n %s", html);
                bytes_received = write(connfd, response, strlen(response));
                printf(" 200 OK\n");
            }
            else{
              char* response = "HTTP/1.1 404 Not Found\r\n\r\n";
              bytes_received = write(connfd, response, strlen(response));
              printf(" 404 Not Found\n");
            }
        }
        else
        {
            // Send a 501 error to the client
            char* response = "HTTP/1.1 501 Not Implemented\r\n\r\n";
            bytes_received = write(connfd, response, strlen(response));
            printf(" 501 Not Implemented\n");
            if (bytes_received < 0) {
                perror("Error writing to socket");
                exit(1);
            }
            
        }

        // close the client socket
        close(connfd);
    }

    // close the listen socket
    close(listenfd);

}


void get_file_from_database(const char* search_string, const char* database_ip, int database_port, int client_socket) {
  // create UDP socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("ERROR opening socket");
    return;
  }

  // set up address for database server
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(database_ip);
  servaddr.sin_port = htons(database_port);

// set timeout for receiving response from database server
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
    perror("ERROR setting socket timeout");
    return;
  }

  // receive response from database server
  char buffer[BUFFER_SIZE];
  int n;
  while ((n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL)) > 0) {
    if (strcmp(buffer, "File Not Found") == 0) {
      // file not found in database, send "404 Not Found" to client
      write(client_socket, "HTTP/1.1 404 Not Found", 23);
      break;
    }
    else if (strcmp(buffer, DONE) == 0) {
      // received final packet, stop receiving
      break;
    }
    else {
      // send received data to client
      char response[4140];
      sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: jpg/html\r\n\r\n %s", buffer);
      write(client_socket, buffer, n);
    }
  }

  if (n < 0) {
    // timed out waiting for response from database server, send "408 Request Timeout" to client
    write(client_socket, "HTTP/1.1 408 Request Timeout", 19);
  }

  // close UDP socket
  close(sockfd);
}