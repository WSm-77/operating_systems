#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
    pid_t childPid = 0;

    childPid = fork();

    if (0 != childPid) {
        wait(NULL);

        printf("parent pid: current pid: %d\n", getpid());
        printf("parent pid: child pid: %d\n", childPid);
        printf("parent pid: parent pid: %d\n", getppid());
    } else {
        printf("child pid: current pid: %d\n", getpid());
        printf("child pid: child pid: %d\n", childPid);
        printf("child pid: parent pid: %d\n", getppid());
    }


    return 0;
}
