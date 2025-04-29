#define _POSIX_C_SOURCE 199309L

// global imports
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>

// local imports
#include "common.h"
#include <sys/stat.h>

#define MAX_CLIENTS 10

mqd_t serverQueue;

int clientsCnt = 0;
mqd_t clientsQueues[MAX_CLIENTS] = {-1};
char clientQueuesPaths[MAX_CLIENTS][CLIENT_QUEUE_PATH_NAME_MAX_SIZE];

void initServer();
void stopServer();
void handleMessage(char* message);
bool isInitMessage(char* messageBody);

void initServer() {
    struct mq_attr queueAttr;
    queueAttr.mq_flags = 0;
    queueAttr.mq_maxmsg = 10;
    queueAttr.mq_msgsize = MSGSIZE;

    serverQueue = mq_open(SERVER_QUEUE_PATH, O_RDWR | O_CREAT, S_IRWXU, &queueAttr);

    // serverQueue = mq_open(SERVER_QUEUE_PATH, O_CREAT | O_RDWR, 0644, &queueAttr);
    // serverQueue = mq_open(SERVER_QUEUE_PATH, O_CREAT | O_RDWR, 0644, NULL);

    if (serverQueue == -1) {
        perror("Couldn't open queue!!!\n");
        mq_unlink(SERVER_QUEUE_PATH);
        exit(1);
    }

    // set server stopping with SIGINT
    struct sigaction sigactionStruct;
    sigactionStruct.sa_handler = stopServer;
    sigemptyset(&sigactionStruct.sa_mask);
    sigaction(SIGINT, &sigactionStruct, NULL);
}

void stopServer() {
    printf("Stopping server...\n");

    mq_unlink(SERVER_QUEUE_PATH);

    // close message queue
    mq_close(serverQueue);
    exit(0);
}

void handleMessage(char* message) {
    int clientId;
    char messageBody[MSGSIZE];

    // extract client ID and message body
    sscanf(message, "%d#%1024[^\n]", &clientId, messageBody);

    printf("clientID: %d\n", clientId);
    printf("message body: %s\n", messageBody);

    if (isInitMessage(messageBody)) {
        if (clientsCnt >= MAX_CLIENTS) {
            printf("Max number of clients reached!!!\n");
            printf("Can not register new client!!!\n");

            return;
        }

        int newClientID = clientsCnt;
        clientsCnt++;

        char* newClientQueuePath = clientQueuesPaths[newClientID];
        getClientQueuePath(newClientQueuePath, clientId);

        clientsQueues[newClientID] = mq_open(newClientQueuePath, O_WRONLY);

        if (clientsQueues[newClientID] == -1) {
            printf("Couldn't create message queue for client: %d\n", newClientID);
            exit(1);
        }

        char toSendMessage[MSGSIZE];
        int toSendMessageLen = snprintf(toSendMessage, MSGSIZE, "%d", newClientID);
        toSendMessage[toSendMessageLen] = '\0';

        mq_send(clientsQueues[newClientID], toSendMessage, MSGSIZE, 1);
    } else {
        for (int reciverId = 0; reciverId < MAX_CLIENTS; reciverId++) {
            if (reciverId == -1 || reciverId == clientId) {
                continue;
            }

            mq_send(clientsQueues[reciverId], messageBody, strlen(messageBody) + 1, 1);
        }
    }
}

bool isInitMessage(char* messageBody) {
    return strcmp("INIT", messageBody) == 0;
}

int main(int argc, char** argv) {
    // create message queue
    initServer();
    char message[MSGSIZE];

    while (true) {
        mq_receive(serverQueue, message, MSGSIZE, NULL);
        handleMessage(message);
    }

    stopServer();

    return 0;
}

