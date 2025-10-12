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
void print_file_info(const char* t_filepath) {
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

    printf("[child %d] done...\n", pid);
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
    // step 2: create child process for each valid file
    // ------------------------------------------------------------------------
    printf("\n--- processing files with child processes ----------\n");
    printf("parent process ID: %d\n", getpid());
    
    std::vector<pid_t> child_pids;
    
    for (size_t i = 0; i < valid_files.size(); i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            // fork failed
            fprintf(stderr, "[parent] error | fork failed for file '%s'\n", valid_files[i].c_str());
            continue;
        }
        else if (pid == 0) {
            // child process
            print_file_info(valid_files[i].c_str());
            exit(EXIT_SUCCESS);
        }
        else {
            // parent process
            child_pids.push_back(pid);
            printf("[parent] created child process %d for file '%s'\n", pid, valid_files[i].c_str());
        }
    }
    
    // parent waits for all children to complete
    printf("[parent] waiting for child processes to complete\n");
    int status;
    int completed = 0;
    
    for (size_t i = 0; i < child_pids.size(); i++) {
        pid_t finished_pid = wait(&status);
        
        if (finished_pid > 0) {
            completed++;
            if (WIFEXITED(status)) {
                printf("[parent] child process %d finished with exit code %d\n", finished_pid, WEXITSTATUS(status));
            } else {
                printf("[parent] child process %d terminated abnormally\n", finished_pid);
            }
        }
    }
    
    printf("\n--- all child processes completed (%d/%lu) ---------\n", completed, child_pids.size());
    
    return EXIT_SUCCESS;
}
