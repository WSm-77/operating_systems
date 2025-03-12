#include "collatz.h"
#include "test_collatz.h"

/**
 * @param input input number
 * @param max_iter maximal number of itterations to perform
 * @param steps array containing steps that reduce input to 1
 */
int test_collatz_convergence(int input, int max_iter, int *steps) {
    for (int iter = 0; iter < max_iter; iter++) {
        input = collatz_conjecture(input);
        steps[iter] = input;

        if (input == 1) {
            return iter + 1;
        }
    }

    return 0;
}
