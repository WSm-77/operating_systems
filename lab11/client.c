#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_NAME_LEN 50
#define MAX_MSG_LEN 512
#define BUFFER_SIZE 1024

int clientSocket;
char clientName[MAX_NAME_LEN];
int isRunning = 1;

void cleanupClient() {
    if (clientSocket > 0) {
        send(clientSocket, "STOP", 4, 0);
        close(clientSocket);
    }

    printf("\n> Client finished work\n");
    exit(0);
}

void signalHandler(int sig) {
    isRunning = 0;
    cleanupClient();
}

void* receiveMessages(void* arg) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while (isRunning) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            if (isRunning) {
                printf("Connection to server was interrupted\n");
                isRunning = 0;
            }
            break;
        }

        buffer[bytesReceived] = '\0';

        if (strncmp(buffer, "ALIVE", 5) == 0) {
            // Respond to server ping
            send(clientSocket, "ALIVE_RESPONSE", 14, 0);
            continue;
        }
        else if (strncmp(buffer, "ERROR:", 6) == 0) {
            printf("Error: %s\n", buffer + 6);
        }
        else if (strncmp(buffer, "OK:", 3) == 0) {
            printf("%s\n", buffer + 3);
        }
        else {
            continue;
        }

        printf("> ");
        fflush(stdout);
    }

    return NULL;
}

void printHelp() {
    printf("\nAvailable commands:\n");
    printf("LIST - display list of active clients\n");
    printf("2ALL <message> - send message to all\n");
    printf("2ONE <client_name> <message> - send private message\n");
    printf("STOP - end connection\n");
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <client_name> <IP_address> <port>\n", argv[0]);
        return 1;
    }

    strncpy(clientName, argv[1], MAX_NAME_LEN - 1);
    clientName[MAX_NAME_LEN - 1] = '\0';

    if (strlen(clientName) == 0) {
        printf("Client name cannot be empty\n");
        return 1;
    }

    char* serverIp = argv[2];
    int serverPort = atoi(argv[3]);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Socket creation error");
        return 1;
    }

    // Configure server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        close(clientSocket);
        return 1;
    }

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error connecting to server");
        close(clientSocket);
        return 1;
    }

    // Send client name
    send(clientSocket, clientName, strlen(clientName), 0);

    // Wait for server response
    char response[BUFFER_SIZE];
    int responseLen = recv(clientSocket, response, sizeof(response) - 1, 0);
    if (responseLen <= 0) {
        printf("Communication error with server\n");
        close(clientSocket);
        return 1;
    }

    response[responseLen] = '\0';

    if (strncmp(response, "ERROR:", 6) == 0) {
        printf("Error: %s\n", response + 6);
        close(clientSocket);
        return 1;
    }

    printf("Connected to server as: %s\n", clientName);
    printHelp();

    // Start thread for receiving messages
    pthread_t receiveThread;
    pthread_create(&receiveThread, NULL, receiveMessages, NULL);

    // Main client loop
    char input[BUFFER_SIZE];
    while (isRunning) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            continue;
        }

        if (strcmp(input, "STOP") == 0) {
            isRunning = 0;
            break;
        }

        // Send command to server
        if (send(clientSocket, input, strlen(input), 0) < 0) {
            perror("Error sending message");
            break;
        }
    }

    cleanupClient();
    return 0;
}
