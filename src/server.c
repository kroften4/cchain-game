#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#include "ts_queue.h"
#include "cchain_protocol.h"
#include "server.h"

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

void enqueue_new_player(int connfd);
void *matchmake(void *q_p);

void *play_cchain(void *room_data_p);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *port = argv[1];


    player_q = ts_queue_new();
    matches = ts_queue_new();

    // player queue consumer
    pthread_t matcher;
    pthread_create(&matcher, NULL, matchmake, player_q);

    server(port, enqueue_new_player);
    return 0;
}

void enqueue_new_player(int connfd) {
    /*
    * Handle client communication during the game
    */
    char queue_msg[] = "QUEUE:1";
    ssize_t sent = send(connfd, queue_msg, sizeof(queue_msg), 0);
    if (sent == -1) {
        perror("enqueue_new_player: send");
        // pthread_exit
    } else {
        printf("enqueue_new_player: sent %zu/%zu bytes\n", sent, sizeof(queue_msg));
        struct client_data *cl_data = malloc(sizeof(struct client_data));
        if (cl_data == NULL) {
            perror("enqueue_new_player: malloc");
            return;
            //pthread_exit
        }
        cl_data->connfd = connfd;
        ts_queue_enqueue(player_q, cl_data);
        printf("enqueue_new_player: Put %d in the queue ", connfd);
        print_queue(player_q);
        pthread_cond_signal(&q_has_match);
        // pthread_exit
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

void *matchmake(void *q_p) {
    struct ts_queue *q = (struct ts_queue *) q_p;
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
            printf("matchmake: Created a room ");
            print_queue(q);

            // add to room list
            __ts_queue_enqueue_nolock(matches, match_data);

            pthread_t game_thread;
            pthread_create(&game_thread, NULL, play_cchain,
                           match_data);
            pthread_detach(game_thread);
        }

        pthread_mutex_unlock(&q->mutex);
    }
}

/*
 * Play the city chain game. `room_data` contains 2 `connfd`s for the 2 players
 * TODO: fix SIGPIPE (sending to closed connection)
 * TODO: make room size variable
 */
void *play_cchain(void *room_data_p) {
    struct match_data *room_data = (struct match_data *) room_data_p;
    puts("play_cchain: Starting game");
    struct server_msg {
        char *opponent_word;
    };
    int connfd_1 = room_data->players_fd[0]->connfd;
    int connfd_2 = room_data->players_fd[1]->connfd;

    char start_msg_1[MAX_MSG_SIZE] = "";
    cchain_msg(start_msg_1, CCHAIN_START, "1");
    send(connfd_1, start_msg_1, sizeof(start_msg_1), 0);
    printf("play_cchain: Sent %s\n", start_msg_1);

    char start_msg_2[MAX_MSG_SIZE] = "";
    cchain_msg(start_msg_2, CCHAIN_START, "0");
    send(connfd_2, start_msg_2, sizeof(start_msg_2), 0);
    printf("play_cchain: Sent %s\n", start_msg_1);

    int player_fds[2] = {connfd_1, connfd_2};
    srand(time(NULL));
    int current_player = rand() % 2;

    char first_turn_msg[MAX_MSG_SIZE] = "";
    cchain_msg(first_turn_msg, CCHAIN_TURN, "NONE");
    send(player_fds[current_player], first_turn_msg,
         sizeof(first_turn_msg), 0);
    printf("play_cchain: Sent %s\n", first_turn_msg);

    while (1) {
        char player_msg[MAX_MSG_SIZE];
        recv(player_fds[current_player], player_msg, MAX_MSG_SIZE, 0);
        player_msg[MAX_MSG_SIZE - 1] = '\0';
        printf("play_cchain: Player %d says %s\n", current_player, player_msg);

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
            printf("play_cchain: Unexpected command: %s", cchain_enum_to_str(buf, player_cmd));
            break;
        }
        default:
            printf("play_cchain: No such command: %d\n", player_cmd);
            break;
        }
        send(player_fds[!current_player], server_msg,
             sizeof(server_msg), 0);
        printf("play_cchain: Sent %s", server_msg);

        current_player = !current_player;
    }
}

