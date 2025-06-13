#include <string.h>

#include "cchain_protocol.h"

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

