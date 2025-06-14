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
#include "matchmaking_server.h"

#define ROOM_SIZE 2

void play_cchain(struct client_data *cl_data[ROOM_SIZE]);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *port = argv[1];

    matchmaking_server(port, play_cchain);
    return 0;
}

/*
 * Play the city chain game.
 * TODO: fix SIGPIPE (sending to closed connection)
 * TODO: make room size variable
 */
void play_cchain(struct client_data *cl_data[ROOM_SIZE]) {
    puts("play_cchain: Starting game");
    struct server_msg {
        char *opponent_word;
    };
    int connfd_1 = cl_data[0]->connfd;
    int connfd_2 = cl_data[1]->connfd;

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

