#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <string.h>

#define ARG_CNT 2

typedef enum {
    NONE,
    IGNORE,
    HANDLER,
    MASK,
    INVALID
} ActionType;

void handler(int sigNo) {
    printf("handler called after reciving signal: %d\n", sigNo);
}

ActionType parseArgs(char* arg) {
    ActionType result = INVALID;

    if (0 == strcmp(arg, "none")) {
        result = NONE;
    }
    else if (0 == strcmp(arg, "ignore")) {
        result = IGNORE;
    }
    else if (0 == strcmp(arg, "handler")) {
        result = HANDLER;
    }
    else if (0 == strcmp(arg, "mask")) {
        result = MASK;
    }

    return result;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Invalid number of arguments: %d!!!\n", argc);
        printf("Provide %d arguments\n", ARG_CNT);

        return 1;
    }

    ActionType actionEnum = parseArgs(argv[1]);

    struct sigaction actionStruct;
    sigset_t blockMask;

    sigemptyset(&actionStruct.sa_mask);
    sigemptyset(&blockMask);

    switch (actionEnum) {
        case NONE:
            actionStruct.sa_handler = SIG_DFL;
            break;

        case IGNORE:
            actionStruct.sa_handler = SIG_IGN;
            break;

        case HANDLER:
            actionStruct.sa_handler = handler;
            break;

        case MASK:
            sigaddset(&blockMask, SIGUSR1);
            sigprocmask(SIG_SETMASK, &blockMask, NULL);

            // check if mask prevents from reciving error
            actionStruct.sa_handler = SIG_ERR;
            break;

        default:
            break;
    }

    sigaction(SIGUSR1, &actionStruct, NULL);

    printf("actionEnum = %d\n", actionEnum);

    raise(SIGUSR1);

    if (MASK == actionEnum) {
        sigset_t pendingSignals;
        sigpending(&pendingSignals);

        printf("is SIGUSR1 a pending signal? %s\n", sigismember(&pendingSignals, SIGUSR1) != 0 ? "yes" : "no");
    }

    printf("Text displayed after rise\n");

    return 0;
}
