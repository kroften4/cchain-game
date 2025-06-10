#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "cchain_protocol.h"

#define BUF_LEN 1000

int connect_to_server(char *name, char *port);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("usage: krftn_test_client <ip/name> <port>");
        return EXIT_FAILURE;
    }

    int serverfd = connect_to_server(argv[1], argv[2]);
    if (serverfd == -1) {
        fprintf(stderr, "Couldn't connect to the server\n");
        return -1;
    }

    char buf[BUF_LEN];
    if (recv(serverfd, buf, sizeof buf, 0) == -1) {
        perror("fail in recv()");
        close(serverfd);
        return -1;
    }
    printf("recieved %s\n", buf);
    while (1) {
        char server_msg[BUF_LEN];
        if (recv(serverfd, server_msg, sizeof server_msg, 0) == -1) {
            perror("fail in recv()");
            close(serverfd);
            return -1;
        }
        printf("recieved %s\n", server_msg);

        char data[MAX_DATA_SIZE] = "";
        enum cchain_command command = cchain_deserialize_msg(data, server_msg);
        switch (command) {
        case CCHAIN_TURN:
        {
            char city[MAX_DATA_SIZE];
            fgets(city, MAX_DATA_SIZE - 1, stdin);
            city[MAX_DATA_SIZE - 1] = '\0';

            char city_msg[MAX_MSG_SIZE] = "";
            cchain_msg(city_msg, CCHAIN_CITY, city);
            send(serverfd, city_msg, sizeof(city_msg), 0);
            break;
        }
        default:
            break;
        }
    }

    return 0;
}

int connect_to_server(char *name, char *port) {
    int errnosave = 0, status, serverfd;
    struct addrinfo hints;
    struct addrinfo *res;

    // get addrinfo for remote
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(name, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    // select the appropriate addrinfo linked list member
    if (res == NULL) {
        perror("couldn't find host addrinfo (list is empty)");
        return -1;
    }
    struct addrinfo server_ai = *res;
    freeaddrinfo(res);

    // make a socket and connect
    if ((serverfd = socket(server_ai.ai_family, server_ai.ai_socktype, server_ai.ai_protocol)) == -1) {
        errnosave = errno;
        perror("failed to create a socket");
        return -1;
    }

    if (connect(serverfd, server_ai.ai_addr, server_ai.ai_addrlen) == -1) {
        errnosave = errno;
        perror("failed to connect");
        return -1;
    }
    printf("connected to server\n");
    return serverfd;
}

