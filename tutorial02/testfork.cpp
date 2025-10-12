#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

int main(int t_argc, char** t_argv) {
    printf ("before fork\n");

    int roura[2];
    pipe(roura);
 
    if (fork() == 0) {
        close (roura[0]);
        // child process
        printf("child PID %d\n", getpid());

        for (int i = 0; i < 10; i++) {
            char buf[1313];
            sprintf(buf, "%d\n", rand() % 10000);
            write(roura[1], buf, strlen(buf));
            usleep(500000);
        }
        close(roura[1]);
        //getchar(); // wait for key press
    } else {
        close(roura[1]);
        while(1) {
            char buf[111];
            int r = read(roura[0], buf, sizeof(buf));
            if ( r==0) break;
            write (1, buf, r);
        }
        close(roura[0]);

        // parent process
        printf("parent PID %d\n", getpid());
        wait(NULL); // wait for child to finish
        //getchar(); // wait for key press
        printf("parent after child\n");
    }
    //getchar(); // wait for key press

    return 0;
}
