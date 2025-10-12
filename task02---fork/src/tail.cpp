#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>

// check if a file is valid for reading
bool is_valid_file(const char* t_filepath) {
    struct stat file_stat;
    
    // ... file exists and get stats
    if (stat(t_filepath, &file_stat) != 0) {
        fprintf(stderr, "error | cannot stat file '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's a directory
    if (S_ISDIR(file_stat.st_mode)) {
        fprintf(stderr, "info | skipping directory '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's executable (we want to skip executables)
    if (file_stat.st_mode & S_IXUSR) {
        fprintf(stderr, "info | skipping executable '%s'\n", t_filepath);
        return false;
    }
    
    // ... it's readable
    if (access(t_filepath, R_OK) != 0) {
        fprintf(stderr, "error | cannot read file '%s'\n", t_filepath);
        return false;
    }
    
    return true;
}

// print file information (called by child process)
void print_file_info(const char* t_filepath, off_t* last_size) {
    struct stat file_stat;
    
    if (stat(t_filepath, &file_stat) != 0) {
        fprintf(stderr, "[child] error | cannot stat file '%s'\n", t_filepath);
        return;
    }
    
    pid_t pid = getpid();
    printf("\n[child %d] === file: %s ==========\n", pid, t_filepath);
    
    // mode (permissions)
    printf("[child %d] mode . . . . . . : %o (octal)\n", pid, file_stat.st_mode & 0777);
    printf("[child %d] permissions. . . : ", pid);
    printf((S_ISDIR(file_stat.st_mode))  ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
    
    // modification time
    struct tm* timeinfo = localtime(&file_stat.st_mtime);
    char time_str[80];

    asctime_r(timeinfo, time_str);
    time_str[strcspn(time_str, "\n")] = 0; // remove newline from asctime output
    printf("[child %d] last modified. . : %s\n", pid, time_str);
    
    // file size
    printf("[child %d] size . . . . . . : %ld bytes\n", pid, file_stat.st_size);
    
    printf("[child %d] process ID . . . : %d\n", pid, pid);
    printf("[child %d] parent process ID: %d\n", pid, getppid());

    // update last size
    if (last_size) {
        *last_size = file_stat.st_size;
    }
}

// child process monitoring function
void monitor_file(const char* t_filepath, int pipe_fd) {
    pid_t pid = getpid();
    off_t last_size = -1; // initialize to -1 to force reading initial size
    
    // print initial file information
    print_file_info(t_filepath, &last_size);
    printf("[child %d] waiting for check commands...\n", pid);
    
    char buffer[256];
    
    // monitor loop - wait for "check\n" commands from parent
    while (true) {
        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            // pipe closed or error
            printf("[child %d] pipe closed, exiting...\n", pid);
            break;
        }
        
        buffer[bytes_read] = '\0';
        
        // check if we received "check\n"
        if (strstr(buffer, "check\n") != NULL) {
            struct stat file_stat;
            
            if (stat(t_filepath, &file_stat) != 0) {
                fprintf(stderr, "[child %d] error | cannot stat file '%s'\n", pid, t_filepath);
                continue;
            }
            
            // check if file has grown
            if (file_stat.st_size > last_size) {
                printf("\n[child %d] file '%s' has grown from %ld to %ld bytes\n", 
                       pid, t_filepath, last_size, file_stat.st_size);
                
                // read and display new content
                FILE* fp = fopen(t_filepath, "r");
                if (fp) {
                    // seek to previous end position (or start if first check)
                    off_t seek_pos = (last_size > 0) ? last_size : 0;
                    fseek(fp, seek_pos, SEEK_SET);
                    
                    printf("[child %d] === new content: ==========\n", pid);
                    char line[1024];
                    while (fgets(line, sizeof(line), fp)) {
                        printf("[child %d] %s", pid, line);
                    }
                    printf("[child %d] === end of new content ==========\n", pid);
                    
                    fclose(fp);
                }
                
                // print updated file information
                print_file_info(t_filepath, &last_size);
            } else {
                printf("[child %d] no changes in file '%s'\n", pid, t_filepath);
            }
        }
    }
    
    close(pipe_fd);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file1> [file2] ...\n", argv[0]);
        fprintf(stderr, "example: %s *.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // ------------------------------------------------------------------------
    // step 1: create list of valid files
    // ------------------------------------------------------------------------
    std::vector<std::string> valid_files;

    // arguments
    printf("--- processing %d argument(s) ----------------------\n", argc - 1);
    for (int i = 1; i < argc; i++) {
        //printf("checking: '%s'\n", argv[i]);
        
        if (is_valid_file(argv[i])) {
            valid_files.push_back(argv[i]);
            //printf(" > VALID: added to processing list\n");
        } else {
            //printf(" > SKIPPED\n");
        }
        //printf("\n");
    }
    
    // summary
    printf("\n--- summary ----------------------------------------\n");
    printf("total arguments: %d\n", argc - 1);
    printf("valid files. . : %lu\n", valid_files.size());
    printf("skipped. . . . : %d\n", argc - 1 - (int)valid_files.size());
    
    if (valid_files.empty()) {
        fprintf(stderr, "[error] no valid files to process\n");
        return EXIT_FAILURE;
    }
    
    printf("\n--- valid files for processing ---------------------\n");
    for (size_t i = 0; i < valid_files.size(); i++) {
        printf("%lu. %s\n", i + 1, valid_files[i].c_str());
    }
    
    // ------------------------------------------------------------------------
    // step 2 & 3: create pipes and child processes for each valid file
    // ------------------------------------------------------------------------
    printf("\n--- processing files with child processes ----------\n");
    printf("parent process ID: %d\n", getpid());
    
    std::vector<pid_t> child_pids;
    std::vector<int> pipe_write_fds; // parent writes to these
    
    for (size_t i = 0; i < valid_files.size(); i++) {
        int pipefd[2];
        
        // create pipe for this child
        if (pipe(pipefd) == -1) {
            fprintf(stderr, "[parent] error | pipe creation failed for file '%s'\n", valid_files[i].c_str());
            continue;
        }
        
        pid_t pid = fork();
        
        if (pid < 0) {
            // fork failed
            fprintf(stderr, "[parent] error | fork failed for file '%s'\n", valid_files[i].c_str());
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        } else if (pid == 0) {
            // child process
            
            // close write end of pipe (child only reads)
            close(pipefd[1]);
            
            // close all other pipes that don't belong to this child
            for (size_t j = 0; j < pipe_write_fds.size(); j++) {
                close(pipe_write_fds[j]);
            }
            
            // monitor the file
            monitor_file(valid_files[i].c_str(), pipefd[0]);
            
            exit(EXIT_SUCCESS);
        } else {
            // parent process
            
            // close read end of pipe (parent only writes)
            close(pipefd[0]);
            
            child_pids.push_back(pid);
            pipe_write_fds.push_back(pipefd[1]);
            
            printf("[parent] created child process %d with pipe for file '%s'\n",pid, valid_files[i].c_str());
        }
    }
    
    // ------------------------------------------------------------------------
    // step 3: parent sends "check\n" commands every second
    // ------------------------------------------------------------------------
    printf("\n[parent] starting monitoring loop...\n");
    printf("[parent] press ctrl+c to stop\n\n");
    
    // give children time to initialize
    sleep(1);
    
    int check_count = 0;
    while (true) {
        sleep(1);
        check_count++;
        
        printf("[parent] sending check command #%d to all children...\n", check_count);
        
        // send "check\n" to all children
        for (size_t i = 0; i < pipe_write_fds.size(); i++) {
            const char* cmd = "check\n";
            ssize_t written = write(pipe_write_fds[i], cmd, strlen(cmd));
            
            if (written < 0) {
                fprintf(stderr, "[parent] error | failed to write to pipe for child %lu\n", i);
            }
        }
    }
    
    // cleanup (this code won't be reached unless we add signal handling)
    for (size_t i = 0; i < pipe_write_fds.size(); i++) {
        close(pipe_write_fds[i]);
    }
    
    // wait for all children
    for (size_t i = 0; i < child_pids.size(); i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
    }
    
    return EXIT_SUCCESS;
}
