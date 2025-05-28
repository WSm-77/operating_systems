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
#define MAX_CONNECTIONS_QUEUE_SIZE 5
#define MAX_NAME_LEN 50
#define MAX_MSG_SIZE 1024
#define ERR_CLIENT_ALREADY_EXISTS -1
#define ERR_MAX_CLIENTS_REACHED -2

typedef struct {
    int socket;
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

int addClient(int socket, const char* name, struct sockaddr_in addr) {
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

        clients[clientId].active = true;
        clients[clientId].addr = addr;
        clients[clientId].socket = socket;
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
        close(currClient->socket);
        memset(currClient->name, 0, MAX_NAME_LEN);
    }

    pthread_mutex_unlock(&clientsLock);
}

void sendClientList(int clientSocket) {
    char message[MAX_MSG_SIZE] = "LIST_RESPONSE:";
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

    send(clientSocket, message, sizeof(message), 0);
}

void broadcastMessage(const char* sender, const char* message, int senderId) {
    char messageBuffer[MAX_MSG_SIZE];

    snprintf(messageBuffer, sizeof(messageBuffer), "MSG_ALL:[%s] %s: %s", getCurrentTime(), sender, message);

    pthread_mutex_lock(&clientsLock);

    for (int clientId = 0; clientId < MAX_CLIENTS; clientId++) {
        if (clients[clientId].active && clientId != senderId) {
            send(clients[clientId].socket, messageBuffer, strlen(messageBuffer), 0);
        }
    }

    pthread_mutex_unlock(&clientsLock);
}

void sendPrivateMessage(const char* sender, const char* target, const char* message) {
    char buffer[MAX_MSG_SIZE];
    snprintf(buffer, sizeof(buffer), "MSG_PRIVATE:[%s] %s (private): %s", getCurrentTime(), sender, message);

    pthread_mutex_lock(&clientsLock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, target) == 0) {
            send(clients[i].socket, buffer, strlen(buffer), 0);
            break;
        }
    }

    pthread_mutex_unlock(&clientsLock);
}

bool handleMessage(int clientId, char* message) {
    client_t* currClient = &clients[clientId];

    if (strncmp(message, "LIST", 4) == 0) {
        sendClientList(currClient->socket);
    }
    else if (strncmp(message, "2ALL ", 5) == 0) {
        broadcastMessage(currClient->name, message + 5, clientId);
    }
    else if (strncmp(message, "2ONE ", 5) == 0) {
        char* space = strchr(message + 5, ' ');
        if (space) {
            *space = '\0';
            char* target = message + 5;
            char* message = space + 1;
            sendPrivateMessage(currClient->name, target, message);
        }
    }
    else if (strncmp(message, "STOP", 4) == 0) {
        return false;
    }

    return true;
}

void* handleClient(void* arg) {
    int clientId = *(int*)arg;
    free(arg);

    client_t* currClient = &clients[clientId];

    char messageBuffer[MAX_MSG_SIZE];

    while (true) {
        ssize_t messageSize = recv(currClient->socket, messageBuffer, sizeof(messageBuffer) - 1, 0);
        if (messageSize < 0) {
            perror("Failed to read message\n");
            break;
        }

        messageBuffer[messageSize] = '\0';
        currClient->lastAlive = time(NULL);

        printf("[%s] USER(%s) > %s\n", getCurrentTime(), currClient->name, messageBuffer);

        if (!handleMessage(clientId, messageBuffer)) {
            break;
        }
    }

    removeClient(clientId);
    return NULL;
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
                    close(currClient->socket);
                    currClient->active = 0;
                } else {
                    send(currClient->socket, "ALIVE", 5, MSG_DONTWAIT);
                }
            }
        }

        pthread_mutex_unlock(&clientsLock);
    }

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

    // set behavioru to signal
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // clear clients data and make them inactive
    memset(clients, 0, sizeof(client_t) * MAX_CLIENTS);

    // create socket
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd < 0)
        exitOnFailure("Couldn't initialize server socket\n");

    // set server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // in this program we can also use INADDR_LOOPBACK
    serverAddr.sin_port = htons(port);

    if (bind(serverSocketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        exitOnFailure("Couldn't bind server socket\n");

    if (listen(serverSocketFd, MAX_CONNECTIONS_QUEUE_SIZE) < 0)
        exitOnFailure("Couldn't start listening\n");

    printf("Server runs on port: %d\n", port);

    pthread_t aliveCheckerThread;
    pthread_create(&aliveCheckerThread, NULL, aliveChecker, NULL);
    // pthread_detach(aliveCheckerThread);

    char clientName[MAX_NAME_LEN];
    char messageBuffer[MAX_MSG_SIZE];

    while (true) {
        // get file descriptor to client socket
        int clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientSocketFd < 0) {
            perror("Couldn't accept connection\n");
            continue;
        }

        ssize_t nameSize = recv(clientSocketFd, clientName, sizeof(clientName) - 1, 0);
        if (nameSize < 0) {
            perror("Couldn't get client name\n");
            close(clientSocketFd);
            continue;
        }

        clientName[nameSize] = '\0';

        // try to create new client
        int clientIdx = addClient(clientSocketFd, clientName, clientAddr);
        bool error = true;
        int messageSize = 0;

        switch (clientIdx){
            case ERR_CLIENT_ALREADY_EXISTS:
                messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "ERROR:User %s alredy exists!!!\n", clientName);
            break;

            case ERR_MAX_CLIENTS_REACHED:
                messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "ERROR:Max clients reached!!!\n");
            break;

            default:
                messageSize = snprintf(messageBuffer, sizeof(messageBuffer) - 1, "OK:User %s connected to server!!!\n", clientName);
                error = false;
            break;
        }

        messageBuffer[messageSize] = '\0';

        send(clientSocketFd, messageBuffer, messageSize + 1, 0);

        if (error) {
            close(clientSocketFd);
            continue;
        }

        // handle client messages in deteached thread
        printf("New client \"%s\" connected from %s\n", clientName, inet_ntoa(clientAddr.sin_addr));

        pthread_t clientThread;
        int* clientIdArg = malloc(sizeof(int));
        *clientIdArg = clientIdx;
        pthread_create(&clientThread, NULL, handleClient, clientIdArg);
        pthread_detach(clientThread);

    }

    cleanupServer();
    return 0;
}
