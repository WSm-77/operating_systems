#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define ARGS_CNT 3

bool parseLong(char* arg, long* longPtr) {
    char* endPtr = NULL;

    *longPtr = strtol(arg, &endPtr, 10);

    if (ERANGE == errno) {
        printf("Number is to big to long variable type!!!\n");
        return false;
    }
    else if ('\0' != *endPtr) {
        printf("Provided argument is not of type long!!!\n");
        return false;
    }

    return true;
}

bool parseArgs(int argc, char** argv, long* catcherPid, long* catcherMode) {
    if (ARGS_CNT!= argc) {
        printf("Wrong number of arguments!!!\n");
        return false;
    }

    return parseLong(argv[1], catcherPid) && parseLong(argv[2], catcherMode);
}

int main(int argc, char** argv) {
    long catcherPid = -1;
    long catcherMode = -1;


    if ( ! parseArgs(argc, argv, &catcherPid, &catcherMode) ) {
        printf("Parsing argument failed!!!\n");
        printf("Exiting...\n");

        return 1;
    }

    sigset_t handableSignals;

    sigemptyset(&handableSignals);
    sigaddset(&handableSignals, SIGUSR1);
    sigprocmask(SIG_BLOCK, &handableSignals, NULL);

    kill((int)catcherPid, SIGUSR1);

    int signalNumber;

    if (-1 == sigwait(&handableSignals, &signalNumber)) {
        perror("Error while waiting for sigwait");

        return 1;
    }

    printf("Recived signal: %d\n", signalNumber);

    union sigval mode = {
        .sival_int = catcherMode
    };

    sigqueue(catcherPid, SIGUSR1, mode);

    return 0;
}
