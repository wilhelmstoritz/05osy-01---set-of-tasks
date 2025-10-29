#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
//#include <string.h>

int main(int t_argc, char** t_argv) {
    int roura1[2];
    pipe(roura1);

    if (fork() == 0) {
        // potomek
        dup2(roura1[1], 1); // presmerovani stdout do pipe
        close(roura1[0]);
        close(roura1[1]);

        execlp("ls", "ls", "/var/", "/etc/", nullptr);
        printf("exec failed\n"); // this line should not be reached if exec is successful
        exit(1);
    }

    if (fork() == 0) {
        // potomek
        int fd = open("vystup.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
        dup2(fd, 1); // presmerovani stdout do souboru
        close(fd);



        dup2(roura1[0], 0); // presmerovani stdin z pipe
        close(roura1[0]);
        close(roura1[1]);

        execlp("tr", "tr", "a-z", "A-Z", nullptr);
        printf("exec failed\n"); // this line should not be reached if exec is successful
        exit(1);
    }

    close (roura1[0]);
    close (roura1[1]);

    wait(NULL);
    wait(NULL);

    // rodic
    //while (1)
    // {
    //    char buffer[128];
    //    ssize_t count = read(roura1[0], buffer, sizeof(buffer));
    //    if (count <= 0) {
    //        break; // konec dat nebo chyba
    //    }
        
    //  write(1, buffer, count); // vypis na stdout
    //}
    

    
    
    

    return EXIT_SUCCESS;
}
