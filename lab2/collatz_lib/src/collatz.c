#include "collatz.h"

int collatz_conjecture(int input) {
    return input % 2 == 0 ? input / 2 : 3 * input + 1;
}
