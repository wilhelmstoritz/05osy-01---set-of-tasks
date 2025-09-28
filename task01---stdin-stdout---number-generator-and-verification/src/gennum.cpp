#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

int main(int t_argc, char** t_argv) {
    int binary_mode = 0;
    int arg_start = 1;  // index where S argument starts
    
    // check for -b flag
    if (t_argc > 1 && strcmp(t_argv[1], "-b") == 0) {
        binary_mode = 1;
        arg_start = 2;
    }
    
    // check number of arguments (accounting for possible -b flag)
    int remaining_args = t_argc - arg_start;
    if (remaining_args < 1 || remaining_args > 2) {
        fprintf(stderr, "usage: %s [-b] s [n]\n", t_argv[0]);
        fprintf(stderr, "-b: binary output mode\n");
        fprintf(stderr, "s - starting number (long)\n");
        fprintf(stderr, "n - count of numbers (default: 1000)\n");

        return 1;
    }
    
    // read starting number from command line; long by assignment
    long start_num = atol(t_argv[arg_start]);

    // read the count of numbers N (default: 1000)
    int count = 1000;
    if (remaining_args == 2)
        count = atoi(t_argv[arg_start + 1]);
    
    if (count <= 0) {
        fprintf(stderr, "error: count must be positive\n");

        return 1;
    }
    
    // generate and print numbers
    if (binary_mode)
        generate_numbers_binary(start_num, count);
    else
        generate_numbers(start_num, count);
    
    return 0;
}
