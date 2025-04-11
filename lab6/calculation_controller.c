#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFOR_SIZE 1024
#define DEBUG 1

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        printf("%s: ", __FILE__); \
        printf(__VA_ARGS__); \
    }

#define PATH_TO_NAMED_PIPE "named_pipe"

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

bool parseArgs(char** argv, double* lowerLimitOfIntegration, double* upperLimitOfIntegration) {
    char* lowerLimitOfIntegrationStr = argv[1];
    char* upperLimitOfIntegrationStr = argv[2];

    if ( ! convertToDouble(lowerLimitOfIntegrationStr, lowerLimitOfIntegration)) {
        return false;
    }

    if ( ! convertToDouble(upperLimitOfIntegrationStr, upperLimitOfIntegration)) {
        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    // verify arguments
    if (3 != argc) {
        printf("Incorrect number of arguments: %d!!!\n", argc);
        printf("Please, provide lower and upper limit of integration!!!\n");

        return 1;
    }

    double lowerLimitOfIntegration = 0.0;
    double upperLimitOfIntegration = 0.0;

    if ( ! parseArgs(argv, &lowerLimitOfIntegration, &upperLimitOfIntegration)) {
        printf("Invalid arguments!!!\n");

        return 2;
    }

    PRINTF_DEBUG("Lower limit of integration: %lf\n", lowerLimitOfIntegration);
    PRINTF_DEBUG("Upper limit of integration: %lf\n", upperLimitOfIntegration);

    // delegate calculations
    mknod(PATH_TO_NAMED_PIPE, S_IFIFO | S_IRUSR | S_IWUSR, 0);

    FILE* namedPipe = fopen(PATH_TO_NAMED_PIPE, "w");

    if (namedPipe == NULL) {
        perror("Error opening named pipe");
        return 3;
    }

    fprintf(namedPipe, "%lf#%lf", lowerLimitOfIntegration, upperLimitOfIntegration);

    double integral = 0.0;

    fclose(namedPipe);

    // read data
    namedPipe = fopen(PATH_TO_NAMED_PIPE, "r");

    fscanf(namedPipe, "%lf", &integral);

    printf("Calculated integral: %lf\n", integral);

    fclose(namedPipe);

    return 0;
}
