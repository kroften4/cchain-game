#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ts_queue.h"

#define BACKLOG 10

struct client_data {
    int connfd;
};

#define ROOM_SIZE 2
struct match_data {
    struct client_data *(players_fd[ROOM_SIZE]);
};

struct ts_queue *player_q;
pthread_cond_t q_has_match = PTHREAD_COND_INITIALIZER;

struct ts_queue *matches;

void print_queue(struct ts_queue *q);

int start_server(char *port);
int handle_connection(struct client_data *cl_data);
void matchmake(struct ts_queue *q);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *port = argv[1];

    int servfd = start_server(port);
    printf("Listening on %s\n", port);

    player_q = ts_queue_new();

    matches = ts_queue_new();


    // player queue consumer
    pthread_t matcher;
    pthread_create(&matcher, NULL, (void *)matchmake, player_q);

    while (1) {
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int connfd = accept(servfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd == -1) {
            perror("accept");
            continue;
        }

        puts("New connection");

        struct client_data *cl_data = malloc(sizeof(struct client_data));
        cl_data->connfd = connfd;

        // player queue producer
        pthread_t client;
        pthread_create(&client, NULL, (void *)&handle_connection, cl_data);
        pthread_detach(client);
    }

    close(servfd);
    return 0;
}

int start_server(char *port) {
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

int handle_connection(struct client_data *cl_data) {
    /*
    * Handle client communication during the game
    */
    char test_buf[] = "hey ya";
    ssize_t sent = send(cl_data->connfd, test_buf, sizeof(test_buf), 0);
    if (sent == -1) {
        perror("send");
        return EXIT_FAILURE; // TODO: change to pthread_exit
    } else {
        printf("sent %zu/%zu bytes\n", sent, sizeof(test_buf));
        ts_queue_enqueue(player_q, cl_data);
        puts("Put in da q");
        print_queue(player_q);
        pthread_cond_signal(&q_has_match);
        return EXIT_SUCCESS;
    }
}

void print_queue(struct ts_queue *q) {
    printf("[ ");
    for (struct ts_queue_node *node = q->head; node != NULL;
         node = node->next) {
        struct client_data *data = node->data;
        printf("%d ", data->connfd);
    }
    printf("]\n");
}

void matchmake(struct ts_queue *q) {
    while (1) {
        pthread_mutex_lock(&q->mutex);
        while (q->size < ROOM_SIZE) {
            pthread_cond_wait(&q_has_match, &q->mutex);
        }

        while (q->size >= ROOM_SIZE) {
            // if enough players in q to create a room, do so

            // create room
            struct match_data *match_data = malloc(sizeof(struct match_data));

            // fill it up
            for (int i = 0; i < ROOM_SIZE; i++) {
                match_data->players_fd[i] = q->head->data;
                __ts_queue_dequeue_nolock(q);
            }
            puts("Created a room:");
            print_queue(q);

            // add to room list
            __ts_queue_enqueue_nolock(matches, match_data);
        }

        pthread_mutex_unlock(&q->mutex);
    }
}

