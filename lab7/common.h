#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#define SERVER_QUEUE_PATH "/server_queue"
#define CLIENT_QUEUE_PATH_TEMPLATE "/client_queue%d"
#define INIT_MESSAGE_TEMPLATE "%d#INIT"
// #define INIT_MESSAGE_TEMPLATE "%s#INIT"
#define MSGSIZE 1024
#define CLIENT_QUEUE_PATH_NAME_MAX_SIZE 100

void getClientQueuePath(char* clientQueuePath) {
    snprintf(clientQueuePath, CLIENT_QUEUE_PATH_NAME_MAX_SIZE, CLIENT_QUEUE_PATH_TEMPLATE, getpid());
}
