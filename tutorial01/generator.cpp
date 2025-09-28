#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int t_argc, char** t_argv) {
    /*
    for(int i = 0; i < t_argc; i++)
        printf("%s\n", t_argv[i]);
    */

    if (t_argc < 2) {
        printf("malao oarametru!\n");
        exit(1);
    }

    int n = atoi(t_argv[1]);
    for(int i = 0; i < n; i++)
        printf("%d\n", rand() % 100000);

    //std::cout << "chci umrit!\n";
    return 0;
}
