#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef enum {
    PRINT_CATCHED_SWITCH_MODE_CNT = 1,
    ACTIVATE_COUNTER = 2,
    IGNOER_CTRL_C = 3,
    PRINT_CTRL_C = 4,
    EXIT_CATCHER = 5,
} CatcherMode;

void activateCounter() {
    sigset_t pendingSignals;

    for (unsigned long counter = 0; /* empty */; counter++) {
        if (-1 == sigpending(&pendingSignals)) {
            perror("Error while reading sigpending\n");
            return;
        }

        // wait for switch mode signal
        if (sigismember(&pendingSignals, SIGUSR1)) {
            return;
        }

        printf("Counter = %ld\n", counter);
        sleep(1);
    }
}

void print_ctrl_c_pressed() {
    printf("Ctrl+C pressed\n");
}

int main(int argc, char** argv) {
    printf("Catcher PID = %d\n", getpid());

    unsigned long switchModeCnt = 0;
    sigset_t handableSignals;
    siginfo_t signalInfo;
    struct sigaction sigactionHandler;

    sigemptyset(&handableSignals);
    sigaddset(&handableSignals, SIGUSR1);

    if (-1 == sigprocmask(SIG_BLOCK, &handableSignals, NULL)) {
        perror("Error wihle processing mask!!!\n");

        return 1;
    }

    bool running = true;

    while (running) {
        printf("Waiting for signal SIGUSR1...\n");

        if (-1 == sigwaitinfo(&handableSignals, &signalInfo)) {
            perror("Error while waiting for signal!!!\n");

            return 1;
        }

        if (signalInfo.si_signo == SIGINT) {
            continue;
        }

        printf("Recived signal SIGUSR1\n");

        kill(signalInfo.si_pid, SIGUSR1);

        if (-1 == sigwaitinfo(&handableSignals, &signalInfo)) {
            perror("Error while waiting for signal");

            return 1;
        }

        switchModeCnt++;

        int catcherMode = signalInfo.si_value.sival_int;

        switch (catcherMode) {
            case PRINT_CATCHED_SWITCH_MODE_CNT:
                printf("Performed switch counter: %ld\n", switchModeCnt);

                break;

            case ACTIVATE_COUNTER:
                printf("Performed switch counter: %ld\n", switchModeCnt);
                activateCounter();

                break;

            case IGNOER_CTRL_C:
                sigactionHandler.sa_handler = SIG_IGN;
                sigaction(SIGINT, &sigactionHandler, NULL);
                // sigaddset(&handableSignals, SIGINT);

                break;

            case PRINT_CTRL_C:
                sigactionHandler.sa_handler = print_ctrl_c_pressed;
                sigaction(SIGINT, &sigactionHandler, NULL);
                // sigaddset(&handableSignals, SIGINT);

                break;

            case EXIT_CATCHER:
                printf("Exiting...\n");
                running = false;

                break;

            default:
                printf("Recived unknow signal: %d\n", catcherMode);

                break;
        };
    }

    return 0;
}
