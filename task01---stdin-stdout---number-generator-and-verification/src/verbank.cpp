#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// weights for CNB control algorithm
int g_weights[] = { 1, 2, 4, 8, 5, 10, 9, 7, 3, 6 };

// bank account verification function
int verify_bank_account(long account_number) {
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

// binary output function
void write_binary_result(long account_number, int is_valid, int valid_only) {
    if (valid_only && !is_valid)
        return; // skip invalid accounts in -v mode
    
    // write account number as binary
    write(STDOUT_FILENO, &account_number, sizeof(long));
    
    // write validity flag as binary
    char validity_flag = is_valid ? 1 : 0;
    write(STDOUT_FILENO, &validity_flag, sizeof(char));
}

// text output function
void write_text_result(long account_number, int is_valid, int valid_only) {
    if (valid_only && !is_valid)
        return; // skip invalid accounts in -v mode
    
    if (valid_only)
        printf("%010ld\n", account_number);
    else
        printf("%010ld %s\n", account_number, is_valid ? "VALID" : "INVALID");
}

int main(int t_argc, char** t_argv) {
    int valid_only = 0;  // -v flag
    int binary_mode = 0; // -b flag
    
    // parse command line arguments
    for (int i = 1; i < t_argc; i++) {
        if (strcmp(t_argv[i], "-v") == 0)
            valid_only = 1;
        else if (strcmp(t_argv[i], "-b") == 0)
            binary_mode = 1;
        else {
            fprintf(stderr, "unknown argument: %s\n", t_argv[i]);
            fprintf(stderr, "usage: %s [-v] [-b]\n", t_argv[0]);
            fprintf(stderr, "-v: show only valid accounts\n");
            fprintf(stderr, "-b: binary input/output mode\n");

            return 1;
        }
    }
    
    if (binary_mode) {
        // binary mode - read binary data
        long account_number;
        while (read(STDIN_FILENO, &account_number, sizeof(long)) == sizeof(long)) {
            int is_valid = verify_bank_account(account_number);
            write_binary_result(account_number, is_valid, valid_only);
        }
    } else {
        // text mode - read text numbers
        long account_number;
        while (scanf("%ld", &account_number) == 1) {
            int is_valid = verify_bank_account(account_number);
            write_text_result(account_number, is_valid, valid_only);
        }
    }
    
    return 0;
}
