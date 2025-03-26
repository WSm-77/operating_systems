#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

bool parseArgs(int argc, char** argv, long* procsNum) {
    if (2 != argc) {
        return false;
    }

    char* strtolPtr = NULL;
    char* strNum = argv[1];

    errno = 0;
    *procsNum = strtol(strNum, &strtolPtr, 10);

    if ('\0' != *strtolPtr) {
        printf("%s is not a number of type long!!!\n", strNum);

        return false;
    } else if (ERANGE == errno) {
        printf("%s is to big number for variable of type long!!!\n", strNum);

        return false;
    } else if (strNum == strtolPtr) {
        printf("Argument can not be an empty string!!!\n");

        return false;
    } else if (*procsNum < 0) {
        printf("Number of proccesses can not be less than 0!!!\n");

        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    long procsNum = 0;

    if (!parseArgs(argc, argv, &procsNum)) {
        printf("Invalid arguments!!!\n");
        printf("exiting...\n");

        return 1;
    }


    pid_t childPid = 0;

    for (int i = 0; i < procsNum; i++) {
        childPid = fork();

        if (0 == childPid) {
            printf("ppid = %d, pid = %d\n", getppid(), getpid());
            exit(0);
        } else {
            waitpid(childPid, NULL, 0);
        }
    }

    printf("num of procs = %ld\n", procsNum);

    return 0;
}
