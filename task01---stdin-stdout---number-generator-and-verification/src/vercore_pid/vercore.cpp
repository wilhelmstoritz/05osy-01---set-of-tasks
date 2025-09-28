#include <stdio.h>
#include <unistd.h>

#include "../vercore.h"

// czech personal identification number (rodné číslo) verification function
// according to MV ČR: https://mv.gov.cz/clanek/rady-a-sluzby-dokumenty-rodne-cislo.aspx
int verify_number(long personal_id) {
    // convert to string to work with individual digits
    char pid_str[12];
    snprintf(pid_str, sizeof(pid_str), "%010ld", personal_id);
    
    // count actual digits (without leading zeros)
    int digit_count = 0;
    long tmp = personal_id;
    if (tmp == 0)
        digit_count = 1;
    else {
        while (tmp > 0) {
            digit_count++;
            tmp /= 10;
        }
    }
    
    // PID must have 9 or 10 digits
    if (digit_count != 9 && digit_count != 10)
        return 0; // invalid
    
    // extract date components from the format YYMMDDXXXX
    int year  = (pid_str[0] - '0') * 10 + (pid_str[1] - '0');
    int month = (pid_str[2] - '0') * 10 + (pid_str[3] - '0');
    int day   = (pid_str[4] - '0') * 10 + (pid_str[5] - '0');
    
    // determine actual month and gender
    int actual_month = month;
    if (month > 50)
        actual_month = month - 50; // women have month + 50
    else if (month > 20)
        actual_month = month - 20; // special cases (foreigners, etc.)
    
    // basic date validation
    if (actual_month < 1 || actual_month > 12)
        return 0; // invalid month
    
    if (day < 1 || day > 31)
        return 0; // invalid day
    
    // more specific day validation per month
    int days_in_month[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (day > days_in_month[actual_month - 1])
        return 0; // invalid day for given month
    
    // for 10-digit numbers (issued after 1.1.1954), check divisibility by 11
    if (digit_count == 10) {
        // according to MV ČR: "Rodné číslo je desetimístné číslo, které je dělitelné jedenácti beze zbytku"
        if (personal_id % 11 != 0)
            return 0; // not divisible by 11
    }
    
    // for 9-digit numbers (issued before 1.1.1954), we just validate the date structure
    // according to MV ČR: they "nesplňují podmínku dělitelnosti jedenácti"
    
    return 1; // valid
}
