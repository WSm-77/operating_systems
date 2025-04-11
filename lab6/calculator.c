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
#define INTERVALS_CNT 10000.0

#define PATH_TO_NAMED_PIPE "named_pipe"

#define PRINTF_DEBUG(...) \
    if (DEBUG) { \
        printf("%s: ", __FILE__); \
        printf(__VA_ARGS__); \
    }

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

double calculate_integral(double lowerLimitOfIntegration, double upperLimitOfIntegration, double dx) {
    double integral = 0.0;
    double x_end = lowerLimitOfIntegration + dx;

    while (x_end <= upperLimitOfIntegration) {
        integral += dx * fun(x_end);
        x_end += dx;
    }

    PRINTF_DEBUG("integral value: %f\n", integral);

    return integral;
}

int main(int argc, char** argv) {
    // read from pipe
    mknod(PATH_TO_NAMED_PIPE, S_IFIFO | S_IRUSR | S_IWUSR, 0);

    FILE* namedPipe = fopen(PATH_TO_NAMED_PIPE, "r");

    if (namedPipe == NULL) {
        perror("Error opening named pipe");
        return 3;
    }

    double lowerLimitOfIntegration = 0.0;
    double upperLimitOfIntegration = 0.0;

    fscanf(namedPipe, "%lf#%lf", &lowerLimitOfIntegration, &upperLimitOfIntegration);

    fclose(namedPipe);

    PRINTF_DEBUG("Lower limit of integration: %f\n", lowerLimitOfIntegration);
    PRINTF_DEBUG("Upper limit of integration: %f\n", upperLimitOfIntegration);

    // calculate integral
    double dx = (upperLimitOfIntegration - lowerLimitOfIntegration) / INTERVALS_CNT;

    double integral = calculate_integral(lowerLimitOfIntegration, upperLimitOfIntegration, dx);

    PRINTF_DEBUG("Calculated integral: %lf\n", integral);

    // send calculations back
    namedPipe = fopen(PATH_TO_NAMED_PIPE, "w");

    fprintf(namedPipe, "%lf", integral);

    fclose(namedPipe);

    return 0;
}

