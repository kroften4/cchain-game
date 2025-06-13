#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#include "ts_queue.h"
#include "cchain_protocol.h"

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

void play_cchain(struct match_data *room_data);

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
    char queue_msg[] = "QUEUE:1";
    ssize_t sent = send(cl_data->connfd, queue_msg, strlen(queue_msg), 0);
    if (sent == -1) {
        perror("send");
        return EXIT_FAILURE; // TODO: change to pthread_exit
    } else {
        printf("sent %zu/%zu bytes\n", sent, sizeof(queue_msg));
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

            pthread_t game_thread;
            pthread_create(&game_thread, NULL, (void *(*)(void *))play_cchain,
                           match_data);
            pthread_detach(game_thread);
        }

        pthread_mutex_unlock(&q->mutex);
    }
}

/*
 * Play the city chain game. `room_data` contains 2 `connfd`s for the 2 players
 * TODO: make room size variable
 */
void play_cchain(struct match_data *room_data) {
    puts("Starting game");
    struct server_msg {
        char *opponent_word;
    };
    int connfd_1 = room_data->players_fd[0]->connfd;
    int connfd_2 = room_data->players_fd[1]->connfd;

    char start_msg_1[MAX_MSG_SIZE] = "";
    cchain_msg(start_msg_1, CCHAIN_START, "1");
    send(connfd_1, start_msg_1, sizeof(start_msg_1), 0);
    printf("Sent %s\n", start_msg_1);

    char start_msg_2[MAX_MSG_SIZE] = "";
    cchain_msg(start_msg_2, CCHAIN_START, "0");
    send(connfd_2, start_msg_2, sizeof(start_msg_2), 0);
    printf("Sent %s\n", start_msg_1);

    int player_fds[2] = {connfd_1, connfd_2};
    srand(time(NULL));
    int current_player = rand() % 2;

    char first_turn_msg[MAX_MSG_SIZE] = "";
    cchain_msg(first_turn_msg, CCHAIN_TURN, "NONE");
    send(player_fds[current_player], first_turn_msg,
         sizeof(first_turn_msg), 0);
    printf("Sent %s\n", first_turn_msg);

    while (1) {
        char player_msg[MAX_MSG_SIZE];
        recv(player_fds[current_player], player_msg, MAX_MSG_SIZE, 0);
        player_msg[MAX_MSG_SIZE - 1] = '\0';
        printf("Player %d says %s\n", current_player, player_msg);

        char data[MAX_DATA_SIZE] = "";
        enum cchain_command player_cmd = cchain_deserialize_msg(data, player_msg);

        char server_msg[MAX_MSG_SIZE] = "";
        switch (player_cmd) {
        case CCHAIN_CITY:
            cchain_msg(server_msg, CCHAIN_TURN, data);
            break;
        case CCHAIN_START:
        case CCHAIN_TURN:
        case CCHAIN_INVALID:
        case CCHAIN_GAMEOVER:
        case CCHAIN_GIVEUP:
        {
            char buf[MAX_COMMAND_SIZE] = "";
            printf("Unexpected command: %s", cchain_enum_to_str(buf, player_cmd));
            break;
        }
        default:
            printf("No such command: %d\n", player_cmd);
            break;
        }
        send(player_fds[!current_player], server_msg,
             sizeof(server_msg), 0);
        printf("Sent %s", server_msg);

        current_player = !current_player;
    }
}

