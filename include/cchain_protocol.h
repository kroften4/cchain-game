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

char *cchain_enum_to_str(char *buf, enum cchain_command command);

enum cchain_command cchain_str_to_enum(char *command);

char *cchain_msg(char msgbuf[MAX_MSG_SIZE], enum cchain_command command,
                 char *data);

enum cchain_command cchain_deserialize_msg(char *data_buf, char *msg);

