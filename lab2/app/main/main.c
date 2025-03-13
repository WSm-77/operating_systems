#ifndef DYNAMIC_LIB_PATH
    #include "test_collatz.h"
#else
    #include <dlfcn.h>
#endif /* DYNAMIC_LIB_PATH */

#include <stdio.h>

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

#ifdef DYNAMIC_LIB_PATH
    void* handle = dlopen(DYNAMIC_LIB_PATH, RTLD_LAZY);

    if (!handle) {
        printf("%s\n", dlerror());
        return 1;
    }

    int (*test_collatz_convergence)(int, int, int*);
    test_collatz_convergence = dlsym(handle, "test_collatz_convergence");

    if (!test_collatz_convergence) {
        printf("%s\n", dlerror());
        return 2;
    }
#endif /* DYNAMIC_LIB_PATH */

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

#ifdef DYNAMIC_LIB_PATH
    dlclose(handle);
#endif /* DYNAMIC_LIB_PATH */

    return 0;
}
