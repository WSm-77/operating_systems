#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define PATH "/bin/ls"

int global = 0;

int main(int argc, char** argv) {
    if (2 != argc) {
        printf("Invalid arguments count: %d!!!\n", argc);
        printf("Please, provide directory path argument!!!");

        return 1;
    }

    int local = 0;

    pid_t childPid = 0;
    childPid = fork();

    if (0 == childPid) {
        // 1. display message "child process",
        printf("child process\n");

        // 2. increment the global and local variables,
        global++;
        local++;

        // 3. display message "child pid = %d, parent pid = %d"
        printf("child pid = %d, parent pid = %d\n", getpid(), getppid());

        // 4. display message "child's local = %d, child's global = %d"
        printf("child's local = %d, child's global = %d\n", local, global);

        // 5. execute the /bin/ls program with argv[1] argument using the execl() function, returning its error code
        return execl(PATH, "ls", "-l", argv[1], NULL);
    } else {
        // 1. Display the message "parent process"
        printf("parent process\n");

        // 2. Display the message "parent pid = %d, child pid = %d"
        printf("parent pid = %d, child pid = %d\n", getpid(), childPid);

        // 3. Display the message "Child exit code: %d"
        int status = 0;
        wait(&status);

        int childReturnedValue = WEXITSTATUS(status);
        printf("Child exit code: %d\n", childReturnedValue);

        // 4. Display the message "Parent's local = %d, parent's global = %d"
        printf("Parent's local = %d, parent's global = %d\n", local, global);

        // 5. Return an appropriate error code.
        return 0;
    }
}
