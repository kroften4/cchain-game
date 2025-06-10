/*
 * CCHAIN Protocol
 * All messages must be null-terminated
 * Server messages:
 * QUEUE:AMOUNT - successfully joined the queue/someone else joined
 * START:OPPONENT_ID - game started
 * TURN:[LAST_CITY/NONE] - your turn. If this is the first turn of the game,
 *                         uses NONE
 * INVALID:INVALID_CODE - named city is invalid.
 *                        1: no such city
 *                        2: wrong starting letter
 *                        3: city already named
 * GAMEOVER:WINNER_ID - opponent gave up/quit the game
 *
 * Client messages:
 * CITY:CITY_NAME - send the city name in your turn
 * GIVEUP:LAST_WORDS - give up and say something nice to the opponent
 */

#include <string.h>

#define MAX_COMMAND_SIZE 10
#define MAX_DATA_SIZE 101
#define MAX_MSG_SIZE (MAX_COMMAND_SIZE + MAX_DATA_SIZE + 1)

enum cchain_command {
    CCHAIN_QUEUE,
    CCHAIN_START,
    CCHAIN_TURN,
    CCHAIN_INVALID,
    CCHAIN_GAMEOVER,
    CCHAIN_CITY,
    CCHAIN_GIVEUP
};

char *cchain_enum_to_str(char *buf, enum cchain_command command) {
    char *commandstr[] = {"QUEUE", "START", "TURN", "INVALID", "GAMEOVER",
                          "CITY", "GIVEUP"};
    strcpy(buf, commandstr[command]);
    return buf;
}

enum cchain_command cchain_str_to_enum(char *command) {
    char *command_strings[] = {"QUEUE", "START", "TURN", "INVALID", "GAMEOVER",
                          "CITY", "GIVEUP"};
    for (int i = 0; i < 7; i++) {
        if (strcmp(command_strings[i], command) == 0)
            return i;
    }
    return -1;
}

char *cchain_msg(char msgbuf[MAX_MSG_SIZE], enum cchain_command command,
                 char *data) {
    char commandstr[MAX_COMMAND_SIZE];
    cchain_enum_to_str(commandstr, command);
    strcat(msgbuf, commandstr);
    strcat(msgbuf, ":");
    strcat(msgbuf, data);
    return msgbuf;
}

enum cchain_command cchain_deserialize_msg(char *data_buf, char *msg) {
    char command[MAX_COMMAND_SIZE] = "";
    for (int i = 0 ; *msg != ':' && *msg != '\0'; msg++) {
        command[i] = *msg;
        i++;
    }
    if (*msg == '\0')
        return -1;
    msg++;
    strcpy(data_buf, msg);
    return cchain_str_to_enum(command);
}

