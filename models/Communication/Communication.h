/**
 * Alexandre Reis - 2018019414
 * Diogo Barbosa - 2018012425
 */

#ifndef TEN_IN_ROW_COMMUNICATION_H
#define TEN_IN_ROW_COMMUNICATION_H

#endif

enum COMMUNICATION_PROTOTYPE_ENUM {
    PROCESS_ID,
    MESSAGE_CODE,
    MESSAGE
};

enum MESSAGE_CODE_TYPES {
    INFO,
    GAME_MOVE,
    COMMAND,
    REQUEST_QUIT,
    INVALID_USERNAME,
    CONNECTION_REQUEST,
    CONNECTION_ACCEPTED,
    CONNECTION_REFUSED
};

char *initMessageModel(int PID, int messageCode, char *message);

void listeningResponse(int descriptor, char *buffer);
void sendMessage(int destDescriptor, char *message);