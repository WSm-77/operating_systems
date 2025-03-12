// system libraries
#include <stdio.h>

// local libraries
#include "test_collatz.h"
#define MAX_ITER 100

void print_steps(int test_number, int iterations, int *steps) {
    if (iterations == 0) {
        printf("Max itterations exceeded!!!\n");
        return;
    } else if (iterations > 1) {
        printf("%d -> ", test_number);
    }

    for (int i = 0; i < iterations - 1; i++) {
        printf("%d -> ", steps[i]);
    }

    printf("%d\n", steps[iterations - 1]);
}

int main(int argc, char** argv) {
    int steps[MAX_ITER];
    int test_number;
    int iterations;

    printf("// test 1\n");
    test_number = 4;
    iterations = test_collatz_convergence(test_number, MAX_ITER, steps);

    print_steps(test_number, iterations, steps);

    printf("// test 2\n");
    test_number = 7;
    iterations = test_collatz_convergence(test_number, MAX_ITER, steps);

    print_steps(test_number, iterations, steps);

    printf("// test 3\n");
    test_number = 37;
    iterations = test_collatz_convergence(test_number, MAX_ITER, steps);

    print_steps(test_number, iterations, steps);

    return 0;
}
