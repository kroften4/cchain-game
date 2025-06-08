#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define BACKLOG 10

int start_server(char *port);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *port = argv[1];

    int servfd = start_server(port);
    printf("Listening on %s\n", port);

    while (1) {
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int connfd = accept(servfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd == -1) {
            perror("accept");
            continue;
        }

        puts("New connection");

        char *test_buf;

        pid_t pid = fork();
        switch(pid) {
        case -1:
            perror("fork");
            continue;
        case 0:
            test_buf = "hey ya";
            ssize_t sent = send(connfd, test_buf, strlen(test_buf), 0);
            if (sent == -1) {
                perror("send");
                exit(EXIT_FAILURE);
            } else {
                printf("sent %zu/%zu bytes\n", sent, strlen(test_buf));
                exit(EXIT_SUCCESS);
            }
        default:
            wait(&pid);
            close(connfd);
            puts("Closed connection");
        }
    }

    close(servfd);
    return 0;
}

int start_server (char *port) {
    /*
    * Start a TCP server
    *
    * Returns server socket fd on success, and -1 on error
    *
    * Also spams errors if any occur into console
    */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *server_ai;
    int ai_status = getaddrinfo(NULL, port, &hints, &server_ai);
    if (ai_status != 0) {
        freeaddrinfo(server_ai);
        fprintf(stderr, "server: addrinfo: %s\n", gai_strerror(ai_status));
        return -1;
    }

    int sockfd;
    for ( ; server_ai != NULL; server_ai = server_ai->ai_next) {
        sockfd = socket(server_ai->ai_family, server_ai->ai_socktype, 0);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, server_ai->ai_addr, server_ai->ai_addrlen) == -1) {
            perror("server: bind");
            close(sockfd);
            continue;
        }

        break;
    }

    if (server_ai == NULL) {
        fprintf(stderr, "server: Failed to bind\n");
        return -1;
    }

    freeaddrinfo(server_ai);

    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
