#include <stdio.h>
#include <unistd.h>

#include "gennumcore.h"

// number generator - text mode
void generate_numbers(long t_start, int t_count) {
    for (int i = 0; i < t_count; i++)
        printf("%010ld\n", t_start + i);
}

// number generator - binary mode
void generate_numbers_binary(long t_start, int t_count) {
    for (int i = 0; i < t_count; i++) {
        long number = t_start + i;
        write(STDOUT_FILENO, &number, sizeof(long));
    }
}