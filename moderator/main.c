/**
 * Alexandre Reis - 2018019414
 * Diogo Barbosa - 2018012425
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "Moderator.h"
#include "../helpers/helpers.h"
#include "../constants/constants.h"
#include "../models/Communication/Communication.h"

void setTempPaths() {
    mkdir(TEMP_ROOT_PATH, 0777);
    mkdir(TEMP_MODERATOR_PATH, 0777);
    mkdir(TEMP_CLIENTS_PATH, 0777);
}

void getArgsValues(int argc, char *argv[]) {
    int championship_duration, waiting_time;

    if (argc < 5) {
        printf("Incorrect set of arguments passed to the program. Must use: \n");
        printf("./moderator -d {championship duration} -w {waiting time}\n");
        exit(0);
    }

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) {
            i++;
            championship_duration = atoi(argv[i]);
        }

        if (!strcmp(argv[i], "-w")) {
            i++;
            waiting_time = atoi(argv[i]);
        }
    }

    system("clear");
    readEnvVariables();
    printInitialInformation(waiting_time, championship_duration);
}

// TODO
//  Validate the clients already connected (max 25)
//  If there is a free space, create a client connection (new node), add the descriptor and send the feedback message(success or not and why)
void handleClientRequest(char *message) {
    Array messageSplited = splitString(strdup(message));

    char clientPid[strlen(messageSplited.array[PROCESS_ID])];
    strcpy(clientPid, messageSplited.array[PROCESS_ID]);

    char clientsTempPath[strlen(TEMP_CLIENTS_PATH)];
    strcpy(clientsTempPath, TEMP_CLIENTS_PATH);

    char clientNamedPipe[strlen(clientPid) + strlen(clientsTempPath)];
    strcpy(clientNamedPipe, clientsTempPath);
    strcat(clientNamedPipe, clientPid);

    // TODO adapt. Follow the TODO above ^
    int fd = open(clientNamedPipe, O_WRONLY);
    sendMessage(fd, "ARBITRO: Pedido recebido");
    close(fd);
    
    freeTheArrayAllocatedMemory(&messageSplited);
}

// TODO DELETE
// ONLY FOR TEST PROPOSE
void connectionsTester(Moderator *Moderator) {
    Client client1, client2;
    Game game1, game2;

    client1 = initClient(123, "Diogo", "pipeClient1");
    client2 = initClient(1234, "Diogo2", "pipeClient2");

    game1 = initGame(321, 1, 2);
    game2 = initGame(4321, 3, 4);

    makeConnection(&Moderator->Connections, client1, game1);
    makeConnection(&Moderator->Connections, client2, game2);

    for (int i = 0; i < Moderator->Connections.length; ++i) {
        printf("Node\n");
        printf("PID: %i | User: %s\n", Moderator->Connections.RunningGames->Client.pid, Moderator->Connections.RunningGames->Client.userName);
        printf("------------\n");
        Moderator->Connections.RunningGames = Moderator->Connections.RunningGames->prox;
    }
}

/* TODO
 * Create a specific buffer to carry the messages info
 * Create the threads to:
 *      -> handle the CHAMPION duration and interrupt the games
 *      -> send info in a json format to the clients (data structure to be defined)
 *      -> to send the inputs received from a client to the related game
 *      -> ...
 *
 * On exit status(SIGTERM or SIGKILL), close the opened pipes and unlink(remove)
 */
int main(int argc, char *argv[]) {
    char responseBuffer[STRING_BUFFER] = "\0";
    int requestMessageSize = 0;

    //getArgsValues(argc, argv);
    setTempPaths();

    Moderator Moderator = initModerator();

    char *moderatorPipePath = createModeratorPipe(&Moderator);

    //Moderator.pipeDescriptor = open(moderatorPipePath, O_RDONLY);
    while (1) {
        Moderator.pipeDescriptor = open(moderatorPipePath, O_RDWR);

        if (read(Moderator.pipeDescriptor, &requestMessageSize, sizeof(int)) == 0) {
            puts("INFO: Client closed a connection.");
            continue;
        }

        if (read(Moderator.pipeDescriptor, responseBuffer, requestMessageSize) == 0) {
            puts("INFO: Client closed a connection.");
            continue;
        }
        printf("Reponse: %s %i\n", responseBuffer, requestMessageSize);

        close(Moderator.pipeDescriptor);

        handleClientRequest(responseBuffer);
        memset(responseBuffer, 0, sizeof(responseBuffer));
    }

    return 0;
}


