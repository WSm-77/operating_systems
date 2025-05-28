#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#define MAX_CLIENTS 10
#define MAX_NAME_LEN 50
#define MAX_MSG_LEN 1024

typedef struct {
    int socket;
    char name[MAX_NAME_LEN];
    struct sockaddr_in addr;
    bool active;
    time_t lastAlive;
} client_t;

client_t clients[MAX_CLIENTS];
int clientCount = 0;
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
int serverSocket;

void cleanupServer() {
    close(serverSocket);
    pthread_mutex_destroy(&clientsMutex);
    printf("Exiting server\n");
    exit(0);
}

void signalHandler(int sig) {
    cleanupServer();
}

char* getCurrentTime() {
    static char timeStr[64];
    time_t now = time(NULL);
    struct tm *tmInfo = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", tmInfo);
    return timeStr;
}

int addClient(int socket, const char* name, struct sockaddr_in addr) {

}

void removeClient(int index) {

}

void sendClientList(int clientSocket) {

}

void broadcastMessage(const char* sender, const char* message, int senderIndex) {

}

void sendPrivateMessage(const char* sender, const char* target, const char* message) {

}

void* handleClient(void* arg) {
    int clientIndex = *(int*)arg;
    free(arg);

    removeClient(clientIndex);
    return NULL;
}

void* aliveChecker(void* arg) {


    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // clear clients data and make them inactive
    memset(clients, 0, sizeof(client_t) * MAX_CLIENTS);

    printf("Server runs on port: %d\n", port);

    // TODO: add clients handle loop

    cleanupServer();
    return 0;
}
