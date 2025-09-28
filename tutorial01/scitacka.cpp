#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int t_argc, char** t_argv) {
    int sum = 0;

    while(1) {
        int num;

        if (scanf("%d", &num) != 1) break;
        sum += num;
    }

    printf("suma %d\n", sum);

}
