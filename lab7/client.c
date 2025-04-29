#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

// global imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

// local imports
#include "common.h"

mqd_t clientQueue;
mqd_t serverQueue;

void initClient();
void stopClient();

void initClient() {
    char clientQueuePath[CLIENT_QUEUE_PATH_NAME_MAX_SIZE];
    getClientQueuePath(clientQueuePath, getpid());

    struct mq_attr queueAttr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MSGSIZE
    };

    mqd_t clientQueue = mq_open(clientQueuePath, O_CREAT | O_RDONLY, 0644, &queueAttr);

    if (clientQueue == -1) {
        perror("Couldn't open queue!!!\n");
        mq_unlink(clientQueuePath);
        exit(1);
    }

    // set client stopping with SIGINT
    struct sigaction sigactionStruct;
    sigactionStruct.sa_handler = stopClient;
    sigemptyset(&sigactionStruct.sa_mask);
    sigaction(SIGINT, &sigactionStruct, NULL);
}

void stopClient() {
    printf("Stopping client...\n");

    char clientQueuePath[CLIENT_QUEUE_PATH_NAME_MAX_SIZE];
    getClientQueuePath(clientQueuePath, getpid());
    mq_unlink(clientQueuePath);

    // close message queue
    mq_close(clientQueue);
    mq_close(serverQueue);
    exit(0);
}

void receiveMessages() {
    char message[MSGSIZE];

    while (true)
    {
        mq_receive(clientQueue, message, MSGSIZE, NULL);
        printf("%s\n", message);
    }
}

void sendMessages(int clientId) {
    while (true) {
        char *userMessage = NULL;
        size_t len = 0;
        ssize_t read;

        read = getline(&userMessage, &len, stdin);

        if (read != -1) {
            char toSendMessage[MSGSIZE];

            int toSendMessageLen = snprintf(toSendMessage, MSGSIZE, "%d#%s", clientId, userMessage);
            toSendMessage[toSendMessageLen] = '\0';

            mq_send(serverQueue, toSendMessage, toSendMessageLen + 1, 1);
        } else {
            printf("Error reading input\n");
        }

        free(userMessage);
    }
}

int main(int argc, char** argv) {
    // initialize communication

    char clientQueuePath[CLIENT_QUEUE_PATH_NAME_MAX_SIZE];
    getClientQueuePath(clientQueuePath, getpid());

    struct mq_attr queueAttr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MSGSIZE
    };

    clientQueue = mq_open(clientQueuePath, O_CREAT | O_RDONLY, 0644, &queueAttr);
    serverQueue = mq_open(SERVER_QUEUE_PATH, O_WRONLY);

    if (serverQueue == -1) {
        perror("Couldn't establish connection with server!!!\n");
        exit(1);
    }

    char initMessage[MSGSIZE];
    int initMessageLen = snprintf(initMessage, MSGSIZE, INIT_MESSAGE_TEMPLATE, getpid());
    initMessage[initMessageLen] = '\0';

    mq_send(serverQueue, initMessage, initMessageLen + 1, 1);

    char message[MSGSIZE];
    mq_receive(clientQueue, message, MSGSIZE, NULL);

    int clientId = atoi(message);

    // start chat

    pid_t childPid = fork();

    if (childPid == 0) {
        // create receiver in subprocess
        receiveMessages();
    } else {
        // sender
        sendMessages(clientId);
    }

    stopClient();

    return 0;
}
