#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define BUFFOR_SIZE 1024

double fun(double x) {
    return 4 / (x * x + 1);
}

bool parseArgs(char** argv, double* intervalSize, long* maxProcessesCount) {
    char* intervalSizeStr = argv[1];
    char* maxProcessesCountStr = argv[2];

    char* endptr = NULL;

    *intervalSize = strtod(intervalSizeStr, &endptr);

    if (*endptr != '\0') {
        printf("%s is not a double number!!!\n", intervalSizeStr);

        return false;
    }
    else if (ERANGE == errno) {
        printf("Size of double variable exceeded!!!\n");

        return false;
    }

    endptr = NULL;

    *maxProcessesCount = strtol(maxProcessesCountStr, &endptr, 10);

    if (*endptr != '\0') {
        printf("%s is not a long number!!!\n", intervalSizeStr);

        return false;
    }
    else if (ERANGE == errno) {
        printf("Size of long variable exceeded!!!\n");

        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    if (3 != argc) {
        printf("Incorrect number of arguments: %d!!!\n", argc);
        printf("Please, provide interval size and number of processes!!!\n");

        return 1;
    }

    double intervalSize = 0.0;
    long maxProcessesCount = 0;

    if ( ! parseArgs(argv, &intervalSize, &maxProcessesCount)) {
        printf("Invalid arguments!!!\n");

        return 2;
    }

    int lowerLimitOfIntegration = 0;
    int upperLimitOfIntegration = 1;

    int fd1[2];
    int* childReadDescriptor = &fd1[0];
    int* parentWriteDescriptor = &fd1[1];

    int fd2[2];
    int* parentReadDescriptor = &fd2[0];
    int* childWriteDescriptor = &fd2[1];

    pipe(fd1);
    pipe(fd2);

    write(*parentWriteDescriptor, "ala ma kota", 12);

    pid_t pid = fork();

    char buffor[BUFFOR_SIZE];
    buffor[0] = '\0';

    if (0 == pid) {

        // child

        printf("Child process\n");

        close(*parentReadDescriptor);
        close(*parentWriteDescriptor);

        ssize_t readSize = read(*childReadDescriptor, &buffor, BUFFOR_SIZE);

        printf("Child buff content: %s\n", buffor);

        write(*childWriteDescriptor, "adam ma emu", 12);

        close(*childReadDescriptor);
        close(*childWriteDescriptor);
    } else {

        // parent

        waitpid(pid, NULL, 0);

        printf("Parent process\n");

        close(*childWriteDescriptor);
        close(*childReadDescriptor);

        read(*parentReadDescriptor, &buffor, BUFFOR_SIZE);

        printf("Parent buff content: %s\n", buffor);

        close(*parentReadDescriptor);
        close(*parentWriteDescriptor);
    }

    return 0;
}
