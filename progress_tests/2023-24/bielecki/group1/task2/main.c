#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


void sighandler(int sig, siginfo_t *info, void *context) {
    printf("child process recived signal SIGUSR1 and value %d\n", info->si_value.sival_int);
}

int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    struct sigaction action;
    action.sa_sigaction = &sighandler;
    action.sa_flags = SA_SIGINFO;

    int child = fork();
    if(child == 0) {
        //zablokuj wszystkie sygnaly za wyjatkiem SIGUSR1
        sigset_t mask;
        sigfillset(&mask);
        sigdelset(&mask, SIGUSR1);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        //zdefiniuj obsluge SIGUSR1 w taki sposob zeby proces potomny wydrukowal
        //na konsole przekazana przez rodzica wraz z sygnalem SIGUSR1 wartosc
        sigaction(SIGUSR1, &action, NULL);

        //zaczekaj na sygnał
        pause();
    }
    else {
        //wyslij do procesu potomnego sygnal przekazany jako argv[2]
        //wraz z wartoscia przekazana jako argv[1]
        int signal = atoi(argv[2]);
        int value = atoi(argv[1]);
        printf("signal: %d, value: %d\n", signal, value);
        union sigval sigValue = {.sival_int = value};

        sleep(1);
        sigqueue(child, signal, sigValue);
    }

    return 0;
}
