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
mqd_t clientsQueues[MAX_CLIENTS];

void initServer();
void stopServer();
void handleMessage(char* message);

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
    // char clientId[MSGSIZE];
    char messageBody[MSGSIZE];

    printf("%s\n", message);

    // extract client ID and message body
    sscanf(message, "%d#%s", &clientId, messageBody);
    // sscanf(message, "%d#%s", clientId, messageBody);

    printf("clientID: %d\n", clientId);
    printf("message body: %s\n", messageBody);
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

