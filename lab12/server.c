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
#define MAX_CONNECTIONS_QUEUE_SIZE 10
#define MAX_NAME_LEN 50
#define MAX_MSG_SIZE 1024
#define ERR_CLIENT_ALREADY_EXISTS -1
#define ERR_MAX_CLIENTS_REACHED -2

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    struct sockaddr_in addr;
    bool active;
    time_t lastAlive;
} client_t;

client_t clients[MAX_CLIENTS];
int clientCount = 0;
pthread_mutex_t clientsLock = PTHREAD_MUTEX_INITIALIZER;
int serverSocketFd;

void cleanupServer() {
    close(serverSocketFd);
    pthread_mutex_destroy(&clientsLock);
    printf("Exiting server\n");
    exit(0);
}

void exitOnFailure(char* exitMessage) {
    perror(exitMessage);
    cleanupServer();
    exit(1);
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

int addClient(const char* name, struct sockaddr_in addr) {
    pthread_mutex_lock(&clientsLock);

    // check if client already exists
    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        if (clients[clientId].active && strcmp(clients[clientId].name, name) == 0) {
            pthread_mutex_unlock(&clientsLock);
            return ERR_CLIENT_ALREADY_EXISTS;
        }
    }

    // find free space for client
    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        if (clients[clientId].active)
            continue;

        clients[clientId].id = clientId;
        clients[clientId].active = true;
        clients[clientId].addr = addr;
        clients[clientId].lastAlive = time(NULL);
        strncpy(clients[clientId].name, name, MAX_NAME_LEN - 1);

        pthread_mutex_unlock(&clientsLock);
        return clientId;
    }

    pthread_mutex_unlock(&clientsLock);

    return ERR_MAX_CLIENTS_REACHED;
}

void removeClient(int clientId) {
    pthread_mutex_lock(&clientsLock);

    client_t* currClient = &clients[clientId];
    if (clientId >= 0 && clientId < MAX_CLIENTS && currClient->active) {
        printf("[%s] Dispatchig client %s\n", getCurrentTime(), currClient->name);

        currClient->active = false;
        memset(currClient->name, 0, MAX_NAME_LEN);
    }

    pthread_mutex_unlock(&clientsLock);
}

void sendClientList(client_t* client) {
    char message[MAX_MSG_SIZE] = "OK:Alive users: ";
    char temp[MAX_NAME_LEN + 2];

    pthread_mutex_lock(&clientsLock);

    client_t* currClient;
    bool foundClients = false;

    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        currClient = &clients[clientId];

        if (!currClient->active)
            continue;

        snprintf(temp, sizeof(temp), "%s, ", currClient->name);
        strcat(message, temp);

        foundClients = true;
    }

    pthread_mutex_unlock(&clientsLock);

    // remove last comma
    int len = strlen(message);
    if (foundClients && message[len-2] == ',') {
        message[len-2] = '\0';
    } else {
        snprintf(temp, sizeof(temp), "NONE");
        strcat(message, temp);
    }

    sendto(serverSocketFd, message, MAX_MSG_SIZE, 0, (struct sockaddr*)&client->addr, sizeof(client->addr));
}

void broadcastMessage(client_t* sender, const char* message) {
    char messageBuffer[MAX_MSG_SIZE];

    snprintf(messageBuffer, sizeof(messageBuffer), "OK:[%s] %s: %s", getCurrentTime(), sender->name, message);

    pthread_mutex_lock(&clientsLock);

    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        if (clients[clientId].active && clientId != sender->id) {
            sendto(serverSocketFd, messageBuffer, MAX_MSG_SIZE, 0, (struct sockaddr*)&clients[clientId].addr, sizeof(clients[clientId].addr));
        }
    }

    pthread_mutex_unlock(&clientsLock);
}

void sendPrivateMessage(client_t* sender, const char* target, const char* message) {
    char buffer[MAX_MSG_SIZE];
    snprintf(buffer, sizeof(buffer), "OK:[%s] %s (private): %s", getCurrentTime(), sender->name, message);

    pthread_mutex_lock(&clientsLock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, target) == 0) {
            sendto(serverSocketFd, buffer, MAX_MSG_SIZE, 0, (struct sockaddr*)&clients[i].addr, sizeof(clients[i].addr));
            break;
        }
    }

    pthread_mutex_unlock(&clientsLock);
}

client_t* get_client(struct sockaddr_in clientAddr) {
    pthread_mutex_lock(&clientsLock);

    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        if (clients[clientId].addr.sin_family == clientAddr.sin_family &&
            clients[clientId].addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
            clients[clientId].addr.sin_port == clientAddr.sin_port) {
            pthread_mutex_unlock(&clientsLock);

            return &clients[clientId];
        }
    }

    pthread_mutex_unlock(&clientsLock);

    return NULL;
}

void handleMessage(struct sockaddr_in clientAddr, char* message) {
    client_t* currClient = get_client(clientAddr);

    currClient->lastAlive = time(NULL);
    printf("from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    printf("[%s] USER(%s)> %s\n", getCurrentTime(), currClient->name, message);

    if (strncmp(message, "LIST", 4) == 0) {
        sendClientList(currClient);
    }
    else if (strncmp(message, "2ALL ", 5) == 0) {
        broadcastMessage(currClient, message + 5);
    }
    else if (strncmp(message, "2ONE ", 5) == 0) {
        char* space = strchr(message + 5, ' ');
        if (space) {
            *space = '\0';
            char* target = message + 5;
            char* message = space + 1;
            sendPrivateMessage(currClient, target, message);
        }
    }
    else if (strncmp(message, "STOP", 4) == 0) {
        removeClient(currClient->id);
    }
}

void* aliveChecker(void* arg) {
    while (true) {
        sleep(30);

        time_t now = time(NULL);
        pthread_mutex_lock(&clientsLock);

        for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
            client_t* currClient = &clients[clientId];

            if (currClient->active) {
                // check if user was active since last ping
                if (now - currClient->lastAlive > 30) {
                    printf("Client %s not responding\n", currClient->name);
                    currClient->active = 0;
                } else {
                    sendto(serverSocketFd, "ALIVE", 5, 0, (struct sockaddr*)&currClient->addr, sizeof(currClient->addr));
                }
            }
        }

        pthread_mutex_unlock(&clientsLock);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        return 1;
    }

    char* serverIp = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // set behavioru to signal
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // clear clients data and make them inactive
    memset(clients, 0, sizeof(client_t) * MAX_CLIENTS);

    // create socket
    serverSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocketFd < 0)
        exitOnFailure("Couldn't initialize server socket\n");

    // set server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0)
        exitOnFailure("Invalid IP address\n");

    // reuse port
    int reuseAddress = 1;
    setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
    if (bind(serverSocketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        exitOnFailure("Couldn't bind server socket\n");

    printf("Server runs on port: %d\n", port);

    pthread_t aliveCheckerThread;
    pthread_create(&aliveCheckerThread, NULL, aliveChecker, NULL);
    // pthread_detach(aliveCheckerThread);

    char messageBuffer[MAX_MSG_SIZE];
    char clientName[MAX_NAME_LEN];

    while (true) {
        ssize_t messageSize = recvfrom(serverSocketFd, messageBuffer, MAX_MSG_SIZE - 1, 0, (struct sockaddr*)&clientAddr, &clientLen);

        if (messageSize <= 0) {
            perror("Couldn't read message\n");
            continue;
        }

        messageBuffer[messageSize] = '\0';

        if (strncmp("REGISTER ", messageBuffer, 9) == 0) {
            strncpy(clientName, messageBuffer + 9, MAX_NAME_LEN);
            int clientId = addClient(clientName, clientAddr);

            switch (clientId){
                case ERR_CLIENT_ALREADY_EXISTS:
                    messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "ERROR:User %s alredy exists!!!\n", clientName);
                break;

                case ERR_MAX_CLIENTS_REACHED:
                    messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "ERROR:Max clients reached!!!\n");
                break;

                default:
                    messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "OK:User %s connected to server!!!\n", clientName);
                break;
            }

            sendto(serverSocketFd, messageBuffer, MAX_MSG_SIZE, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
        }
        else {
            handleMessage(clientAddr, messageBuffer);
        }
    }

    cleanupServer();
    return 0;
}
