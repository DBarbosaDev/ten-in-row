/**
 * Alexandre Reis - 2018019414
 * Diogo Barbosa - 2018012425
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "Moderator.h"
#include "../helpers/helpers.h"
#include "../constants/constants.h"
#include "../models/Communication/Communication.h"

Moderator moderator;
int championship_duration = 0, waiting_time = 0;

void setTempPaths() {
    mkdir(TEMP_ROOT_PATH, 0777);
    mkdir(TEMP_MODERATOR_PATH, 0777);
    mkdir(TEMP_CLIENTS_PATH, 0777);
}

void getArgsValues(int argc, char *argv[]) {
    if (argc < 5) {
        perror("Incorrect set of arguments passed to the program. Must use: \n");
        perror("./moderator -d {championship duration} -w {waiting time}\n");
        exit(0);
    }

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) {
            i++;
            championship_duration = stringToNumber(argv[i]);
        }

        if (!strcmp(argv[i], "-w")) {
            i++;
            waiting_time = stringToNumber(argv[i]);
        }
    }

    system("clear");
    readEnvVariables();
    printInitialInformation(waiting_time, championship_duration);
}

// TODO close the games (SIGUSR1), wait the points, send to the client and close the connections
void signalHandler(int signal) {
    close(moderator.pipeDescriptor);
    unlink(TEMP_MODERATOR_NAMED_PIPE);

    while (moderator.connectedClients != NULL) {
        ConnectedClients *auxConnectedClients = moderator.connectedClients->prox;

        kill(moderator.connectedClients->client.pid, SIGUSR2);
        free(moderator.connectedClients);

        moderator.connectedClients = auxConnectedClients;
    }
    system(RM_TEMP_ROOT_PATH);

    moderator.connectedClients = NULL;
    exit(0);
}

// TODO commands with prints
void *commandReaderListener(void *pointerToData) {
    Moderator *Moderator = pointerToData;
    char command[INPUT_BUFFER];

    while (1) {
        scanf("%29s", command);
        int commandLength = strlen(command);

        if (!strcmp(command, "players")) {
            displayClients(Moderator);
        }
        else if (!strcmp(command, "games")) {
            displayGames(Moderator);
        }
        else if (command[0] == 'k') {
            char playerName[29] = "\0";

            strncpy(playerName, command + 1, commandLength-1);
            kickPlayer(Moderator, playerName);
        }
        else if (command[0] == 's') {
            char playerName[29] = "\0";

            strncpy(playerName, command + 1, commandLength-1);
            changeClientCommunicationStatus(Moderator, playerName, 1);
        }
        else if (command[0] == 'r') {
            char playerName[29] = "\0";

            strncpy(playerName, command + 1, commandLength-1);
            changeClientCommunicationStatus(Moderator, playerName, 0);
        }
        else if (!strcmp(command, "end")) {
            printf("Concluir o campeonato imediatamente\n");
        }
        else if (!strcmp(command, "exit")) {
            kill(moderator.pid, SIGTERM);
        }
        else {
            printf("Comando indisponivel\n");
        }
    }
}

void *championshipTimerThread(void *pointerToData) {
    Moderator *Moderator = pointerToData;

    pthread_join(Moderator->threads.championshipWaitingTimeThreadID, NULL);

    ConnectedClients *auxConnectedClient = Moderator->connectedClients;
    int gamePoints = 0, clientFd;
    char *gamePointsString;

    printf("\n## Campeonato iniciado!");fflush(stdout);
    sleep(championship_duration);
    printf("\n## O campeonato terminou!\n");fflush(stdout);

    Moderator->championStatus = FINISHED;

    while (Moderator->connectedClients != NULL) {
        kill(Moderator->connectedClients->client.gameChildProcess->pid, SIGUSR1);
        
        wait(&gamePoints);

        kill(Moderator->connectedClients->client.pid, SIGUSR1);

        clientFd = open(Moderator->connectedClients->client.pipeLocation, O_WRONLY);

        gamePoints = gamePoints/256;

        gamePointsString = strdup(getNumberInString(gamePoints));
        sendMessage(clientFd, initMessageModel(Moderator->pid, INFO, gamePointsString));
        close(clientFd);

        Moderator->connectedClients = Moderator->connectedClients->prox;
    }

    Moderator->connectedClients = auxConnectedClient;

    kill(Moderator->pid, SIGTERM);
}

void *championshipWaitingTimeThread(void *pointerToData) {
    Moderator *Moderator = pointerToData;

    printf("\n## Pelo menos 2 players foram conectados.\n");
    printf("## O campeonato inicia em %i segundos.\n", waiting_time);

    sleep(waiting_time);

    startChampionship(Moderator);

    pthread_create(&moderator.threads.championshipTimerThreadID, NULL, championshipTimerThread, &moderator);
    pthread_exit(NULL);
}

void buildGamesApps(Moderator *moderator, int numberOfGamesToBuild) {

    char command[100] = "\0";
    char gameName[20] = "\0";
    char gamePath[100] = "\0";

    for (int i = 0; i < numberOfGamesToBuild && i < 1000; i++)
    {
        strcat(command, "make jogo GAME_NUMBER=");
        strcat(command, getNumberInString(i+1));

        strcat(gameName, "g_");
        strcat(gameName, getNumberInString(i+1));
        strcat(gamePath, "./");
        strcat(gamePath, gameDir);
        strcat(gamePath, gameName);

        addGameApp(moderator, gameName, gamePath);

        system(command);

        memset(command, 0, sizeof(command));
        memset(gameName, 0, sizeof(gameName));
        memset(gamePath, 0, sizeof(gamePath));
    }
}

/* TODO
 * During the champion, on client request, get the client info by PID and redirect the info to the related game process. (WITH BUGS)
 * Create a function to send and get info from anonymous pipes (Moderator <-> Game)
 * (Client <-> Moderator) comm should follow the existing standard
 * 
 * Create the threads to:
 *      -> control the waiting time
 *      -> handle the CHAMPION duration and interrupt the games and clients when the counter fisnishes,sending the pontuation
 */
int main(int argc, char *argv[]) {
    char responseBuffer[STRING_BUFFER] = "\0";
    int numberOfGames = 4;
    
    initRandom();
    getArgsValues(argc, argv);
    setTempPaths();

    moderator = initModerator();
    moderator.pipeDescriptor = open(moderator.pipePath, O_RDWR);

    buildGamesApps(&moderator, numberOfGames);
    
    signal(SIGTERM, signalHandler);
    //signal(SIGINT, signalHandler);

    pthread_create(&moderator.threads.administratorCommandsReaderThreadID, NULL, commandReaderListener, &moderator);

    printf("-----------------------------------------------\n");
    printf("\t### A aguardar por clientes... ###\n");
    printf("-----------------------------------------------\n");

    while (moderator.championStatus != FINISHED) {
        listeningResponse(moderator.pipeDescriptor, responseBuffer);
        handleClientRequest(&moderator, responseBuffer);

        if (moderator.championStatus == WAITING_FOR_PLAYERS && moderator.connectedClientsLength == 2) {
            pthread_create(&moderator.threads.championshipWaitingTimeThreadID, NULL, championshipWaitingTimeThread, &moderator);
        }

        memset(responseBuffer, 0, sizeof(responseBuffer));
    }

    pthread_join(moderator.threads.championshipTimerThreadID, NULL);

    close(moderator.pipeDescriptor);
    return 0;
}


