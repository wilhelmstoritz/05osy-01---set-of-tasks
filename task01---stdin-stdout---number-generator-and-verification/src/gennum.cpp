#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// number generator
void generate_numbers(long t_start, int t_count) {
    for (int i = 0; i < t_count; i++) {
        printf("%010ld\n", t_start + i);
    }
}

int main(int t_argc, char** t_argv) {
    // check number of arguments
    if (t_argc < 2 || t_argc > 3) {
        fprintf(stderr, "usage: %s s [n]\n", t_argv[0]);
        fprintf(stderr, "s - starting number (long)\n");
        fprintf(stderr, "n - count of numbers (default: 1000)\n");
        return 1;
    }
    
    // read starting number from command line; long by assignment
    long start_num = atol(t_argv[1]);

    // read the count of numbers N (default: 1000)
    int count = 1000;
    if (t_argc == 3)
        count = atoi(t_argv[2]);
    
    if (count <= 0) {
        fprintf(stderr, "error: count must be positive\n");
        return 1;
    }
    
    // generate and print numbers
    generate_numbers(start_num, count);
    
    return 0;
}
