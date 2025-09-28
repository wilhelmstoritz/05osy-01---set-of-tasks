#include <stdio.h>
#include <unistd.h>

#include "../vercore.h"

// bank account verification function
int verify_number(long account_number) {
    // weights for CNB control algorithm
    int g_weights[] = { 1, 2, 4, 8, 5, 10, 9, 7, 3, 6 };

    int sum = 0;
    long tmp_number = account_number;

    // calculate checksum using CNB algorithm
    for (int i = 0; i < 10; i++) {
        sum += (tmp_number % 10) * g_weights[i];
        tmp_number /= 10;
    }

    // account is valid if sum is divisible by 11
    return (sum % 11) == 0;
}
