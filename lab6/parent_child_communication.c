#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define BUFFOR_SIZE 1024
// #define DEBUG

double fun(double x) {
    return 4 / (x * x + 1);
}

bool convertToDobule(char* valueStr, double* value) {
    char* endptr = NULL;

    *value = strtod(valueStr, &endptr);

    if (*endptr != '\0') {
        printf("%s is not a double number!!!\n", valueStr);

        return false;
    }
    else if (ERANGE == errno) {
        printf("Size of double variable exceeded!!!\n");

        return false;
    }

    return true;
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

double calculate_integral(double lowerLimitOfIntegration, double upperLimitOfIntegration, double dx) {
    double integral = 0.0;
    double x_end = lowerLimitOfIntegration + dx;

    while (x_end <= upperLimitOfIntegration) {
        integral += dx * fun(x_end);
        x_end += dx;
    }

#ifdef DEBUG
    printf("integral value: %f\n", integral);
#endif

    return integral;
}

double delegate_calculations(double lowerLimitOfIntegration, double upperLimitOfIntegration, double dx) {
    int fdChildToParent[2];
    int* parentReadDescriptor = &fdChildToParent[0];
    int* childWriteDescriptor = &fdChildToParent[1];

    pipe(fdChildToParent);

    pid_t pid = fork();

    char buffor[BUFFOR_SIZE];
    buffor[0] = '\0';

    double integral = 0.0;

    if (0 == pid) {

        // child
#ifdef DEBUG
        printf("Child process\n");
#endif
        close(*parentReadDescriptor);

        double integral = calculate_integral(lowerLimitOfIntegration, upperLimitOfIntegration, dx);
        int bytesPrinted = sprintf(buffor, "%f", integral);
        buffor[bytesPrinted] = '\0';

        write(*childWriteDescriptor, buffor, bytesPrinted + 1);

        close(*childWriteDescriptor);

        // kill child process
        exit(0);
    } else {

        // parent
#ifdef DEBUG
        printf("Parent process\n");
#endif
        waitpid(pid, NULL, 0);


        close(*childWriteDescriptor);

        read(*parentReadDescriptor, &buffor, BUFFOR_SIZE);

#ifdef DEBUG
        printf("Parent buff content: %s\n", buffor);
#endif
        if ( ! convertToDobule(buffor, &integral)) {
            integral = 0.0;
        }

        close(*parentReadDescriptor);
    }

    return integral;
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

    for (int processesCount = 1; processesCount <= maxProcessesCount; processesCount++) {
        printf("\n######################\n");
        printf("Processes count: %d\n", processesCount);
        printf("######################\n\n");

        double integralVal = 0.0;
        double intervalSizePerProcess = (upperLimitOfIntegration - lowerLimitOfIntegration) / (double) processesCount;

        struct timespec startTime;
        struct timespec endTime;

        clock_gettime(CLOCK_MONOTONIC, &startTime);

        for (int process = 0; process < processesCount; process++) {
            double currProcessLowerLimit = lowerLimitOfIntegration + process * intervalSizePerProcess;
            double currProcessUpperLimit = currProcessLowerLimit + intervalSizePerProcess;

#ifdef DEBUG
            printf("\nProcess %d: lower limit: %f, upper limit: %f\n\n", process, currProcessLowerLimit, currProcessUpperLimit);
#endif

            integralVal += delegate_calculations(currProcessLowerLimit, currProcessUpperLimit, intervalSize);
        }

        clock_gettime(CLOCK_MONOTONIC, &endTime);

        double executionTime = (endTime.tv_sec - startTime.tv_sec) * 1e9;
        executionTime = (executionTime + (endTime.tv_nsec - startTime.tv_nsec)) * 1e-9;

        printf("Execution time: %f seconds\n", executionTime);

        printf("integral value: %f\n", integralVal);
    }

    return 0;
}
