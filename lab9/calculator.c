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
#include <pthread.h>

#define BUFFOR_SIZE 1024
#define DEBUG 0

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        printf(__VA_ARGS__); \
    }

typedef struct {
    double lowerLimitOfIntegration;
    double upperLimitOfIntegration;
    double dx;
    double integralValue;
} calculate_integral_args_t;

double fun(double x) {
    return 4 / (x * x + 1);
}

bool convertToDouble(char* valueStr, double* value) {
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

bool parseArgs(char** argv, double* intervalSize, long* maxThreadsCount) {
    char* intervalSizeStr = argv[1];
    char* maxThreadsCountStr = argv[2];

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

    *maxThreadsCount = strtol(maxThreadsCountStr, &endptr, 10);

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

void* calculate_integral(void* args) {
    PRINTF_DEBUG("Calculating integral...\n");

    calculate_integral_args_t* calculateIntegralArgsStruct = (calculate_integral_args_t*)args;
    double integral = 0.0;
    double x_end = calculateIntegralArgsStruct->lowerLimitOfIntegration + calculateIntegralArgsStruct->dx;

    while (x_end <= calculateIntegralArgsStruct->upperLimitOfIntegration) {
        integral += calculateIntegralArgsStruct->dx * fun(x_end);
        x_end += calculateIntegralArgsStruct->dx;
    }

    PRINTF_DEBUG("integral value: %f\n", integral);

    calculateIntegralArgsStruct->integralValue = integral;

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    if (3 != argc) {
        printf("Incorrect number of arguments: %d!!!\n", argc);
        printf("Please, provide interval size and number of threads!!!\n");

        return 1;
    }

    double intervalSize = 0.0;
    long maxThreadsCount = 0;

    if ( ! parseArgs(argv, &intervalSize, &maxThreadsCount)) {
        printf("Invalid arguments!!!\n");

        return 2;
    }

    int lowerLimitOfIntegration = 0;
    int upperLimitOfIntegration = 1;

    for (int threadsCount = 1; threadsCount <= maxThreadsCount; threadsCount++) {
        printf("\n######################\n");
        printf("threads count: %d\n", threadsCount);
        printf("######################\n\n");


        double intervalSizePerThread = (upperLimitOfIntegration - lowerLimitOfIntegration) / (double) threadsCount;

        pthread_t* threadsArr = malloc(sizeof(pthread_t) * threadsCount);
        calculate_integral_args_t* calculateIntegralArgsArr = malloc(sizeof(calculate_integral_args_t) * threadsCount);

        struct timespec startTime;
        struct timespec endTime;

        clock_gettime(CLOCK_MONOTONIC, &startTime);

        // create threads
        for (int thread = 0; thread < threadsCount; thread++) {
            double currThreadLowerLimit = lowerLimitOfIntegration + thread * intervalSizePerThread;
            double currThreadUpperLimit = currThreadLowerLimit + intervalSizePerThread;

            PRINTF_DEBUG("\nthread %d: lower limit: %f, upper limit: %f\n\n", thread, currThreadLowerLimit, currThreadUpperLimit);

            // Initialize the arguments for the thread
            calculateIntegralArgsArr[thread].lowerLimitOfIntegration = currThreadLowerLimit;
            calculateIntegralArgsArr[thread].upperLimitOfIntegration = currThreadUpperLimit;
            calculateIntegralArgsArr[thread].dx = intervalSize;
            calculateIntegralArgsArr[thread].integralValue = 0.0;

            pthread_create(&threadsArr[thread], NULL, calculate_integral, &calculateIntegralArgsArr[thread]);
        }

        double integralVal = 0.0;

        // wait for all child threades to finish
        for (int thread = 0; thread < threadsCount; thread++) {
            PRINTF_DEBUG("Waiting for integral result...\n");

            pthread_join(threadsArr[thread], NULL);
            integralVal += calculateIntegralArgsArr[thread].integralValue;
        }

        clock_gettime(CLOCK_MONOTONIC, &endTime);

        double executionTime = (endTime.tv_sec - startTime.tv_sec) * 1e9;
        executionTime = (executionTime + (endTime.tv_nsec - startTime.tv_nsec)) * 1e-9;

        printf("Execution time: %f seconds\n", executionTime);

        printf("integral value: %f\n", integralVal);

        free(threadsArr);
        free(calculateIntegralArgsArr);
    }

    return 0;
}
