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
    printf("\n[child %d] === '%s' file info ===================\n", pid, t_filepath);
    
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

    printf("[child %d] === end of '%s' file info ============\n\n", pid, t_filepath);

    // update last size
    if (last_size) {
        *last_size = file_stat.st_size;
    }
}

// print file information header (called by child process)
void print_file_header(const char* t_filepath, off_t old_size, off_t new_size) {
    struct stat file_stat;
    
    if (stat(t_filepath, &file_stat) != 0) {
        fprintf(stderr, "[child] error | cannot stat file '%s'\n", t_filepath);
        return;
    }
    
    pid_t pid = getpid();
    printf("\n[child %d] === '%s' change detected ================\n", pid, t_filepath);
    printf("[child %d] file. . . . . . : %s\n", pid, t_filepath);
    printf("[child %d] process ID . . . : %d\n", pid, pid);
    printf("[child %d] old size. . . . : %ld bytes\n", pid, old_size);
    printf("[child %d] new size. . . . : %ld bytes\n", pid, new_size);
    
    // modification time
    struct tm* timeinfo = localtime(&file_stat.st_mtime);
    char time_str[80];
    asctime_r(timeinfo, time_str);
    time_str[strcspn(time_str, "\n")] = 0;
    printf("[child %d] last modified. . : %s\n", pid, time_str);
    printf("[child %d] ================================================\n\n", pid);
}

// logger child process - reads from all monitoring children and logs to file
void logger_process(int read_fd, const char* logfile) {
    pid_t pid = getpid();
    fprintf(stderr, "[logger %d] info | logger process started, logging to '%s'\n", pid, logfile);
    
    // open log file for writing (or use tty device)
    FILE* log_fp = fopen(logfile, "w");
    if (!log_fp) {
        fprintf(stderr, "[logger %d] error | cannot open logfile '%s'\n", pid, logfile);
        close(read_fd);
        exit(EXIT_FAILURE);
    }
    
    char buffer[4096];
    while (true) {
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            // pipe closed or error
            fprintf(stderr, "[logger %d] info | pipe closed, exiting...\n", pid);
            break;
        }
        
        buffer[bytes_read] = '\0';
        
        // check for 'nolog' file before each output
        if (access("nolog", F_OK) != 0) {
            // 'nolog' file does NOT exist, so we can write to output
            fprintf(log_fp, "%s", buffer);
            fflush(log_fp);
        }
        // if 'nolog' exists, we still read from pipe but don't write to output
    }
    
    fclose(log_fp);
    close(read_fd);
}

// child process monitoring function
void monitor_file(const char* t_filepath, int pipe_fd, int logger_fd) {
    pid_t pid = getpid();
    off_t last_size = -1; // initialize to -1 to force reading initial size

    // print initial file information
    //print_file_info(t_filepath, &last_size);
    
    // redirect stdout to logger pipe
    dup2(logger_fd, STDOUT_FILENO);
    close(logger_fd);
    
    // get initial file size
    struct stat file_stat;
    if (stat(t_filepath, &file_stat) == 0) {
        last_size = file_stat.st_size;
    }
    
    //printf("[child %d] info | waiting for check commands...\n", pid);
    printf("[child %d] info | monitoring file '%s' (initial size: %ld bytes)\n", 
           pid, t_filepath, last_size);
    fflush(stdout);
    
    char buffer[256];
    
    // monitor loop - wait for "check\n" commands from parent
    while (true) {
        ssize_t bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            // pipe closed or error
            printf("[child %d] info | pipe closed, exiting...\n", pid);
            fflush(stdout);
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
                printf("[child %d] info | file '%s' has grown from %ld to %ld bytes\n",
                    pid, t_filepath, last_size, file_stat.st_size);
                
                // read and display new content
                FILE* fp = fopen(t_filepath, "r");
                if (fp) {
                    // seek to previous end position (or start if first check)
                    off_t seek_pos = (last_size > 0) ? last_size : 0;
                    fseek(fp, seek_pos, SEEK_SET);
                    
                    printf("\n[child %d] === '%s' new content: ================\n", pid, t_filepath);
                    char line[1024];
                    while (fgets(line, sizeof(line), fp)) {
                        printf("[child %d] %s", pid, line);
                    }
                    printf("[child %d] === end of '%s' new content ==========\n", pid, t_filepath);

                    fclose(fp);
                }
                
                // print updated file information
                print_file_info(t_filepath, &last_size);
            } else {
                printf("[child %d] info | no changes in file '%s'\n", pid, t_filepath);
            }
            fflush(stdout);
        }
    }
    
    close(pipe_fd);
}

int main(int argc, char** argv) {
    // ------------------------------------------------------------------------
    // parse arguments for -l logfile
    // ------------------------------------------------------------------------
    const char* logfile = nullptr;
    int first_file_arg = 1;
    
    if (argc >= 3 && strcmp(argv[1], "-l") == 0) {
        logfile = argv[2];
        first_file_arg = 3;
    }
    
    if (first_file_arg >= argc) {
        fprintf(stderr, "usage: %s [-l logfile] <file1> [file2] ...\n", argv[0]);
        fprintf(stderr, "example: %s -l output.log *.txt\n", argv[0]);
        fprintf(stderr, "example: %s -l /dev/pts/0 *.txt\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // if no logfile specified, use stdout (via /dev/stdout)
    if (!logfile) {
        logfile = "/dev/stdout";
    }
    
    // ------------------------------------------------------------------------
    // task 4: check for 'stop' file before starting
    // ------------------------------------------------------------------------
    if (access("stop", F_OK) == 0) {
        fprintf(stderr, "[error] 'stop' file exists in current directory\n");
        fprintf(stderr, "[hint] remove 'stop' file before running the program\n");
        return EXIT_FAILURE;
    }
    
    // ------------------------------------------------------------------------
    // task 1: create list of valid files
    // ------------------------------------------------------------------------
    std::vector<std::string> valid_files;

    // arguments
    fprintf(stderr, "--- processing argument(s) -------------------------\n");
    for (int i = first_file_arg; i < argc; i++) {
        fprintf(stderr, "checking: '%s'\n", argv[i]);
        
        if (is_valid_file(argv[i])) {
            valid_files.push_back(argv[i]);
        }
    }
    
    // summary
    fprintf(stderr, "\n--- summary ----------------------------------------\n");
    fprintf(stderr, "total arguments: %d\n", argc - first_file_arg);
    fprintf(stderr, "valid files. . : %lu\n", valid_files.size());
    fprintf(stderr, "skipped. . . . : %d\n", argc - first_file_arg - (int)valid_files.size());
    fprintf(stderr, "logfile. . . . : %s\n", logfile);
    
    if (valid_files.empty()) {
        fprintf(stderr, "[error] no valid files to process\n");
        return EXIT_FAILURE;
    }
    
    fprintf(stderr, "\n--- valid files for processing ---------------------\n");
    for (size_t i = 0; i < valid_files.size(); i++) {
        fprintf(stderr, "%lu. %s\n", i + 1, valid_files[i].c_str());
    }
    
    // ------------------------------------------------------------------------
    // task 2 & 3: create pipes and child processes for each valid file
    // ------------------------------------------------------------------------
    fprintf(stderr, "\n--- processing files with child processes ----------\n");
    fprintf(stderr, "[parent] info | process ID: %d\n", getpid());
    
    // create logger pipe
    int logger_pipe[2];
    if (pipe(logger_pipe) == -1) {
        fprintf(stderr, "[parent] error | logger pipe creation failed\n");
        return EXIT_FAILURE;
    }
    
    // create logger child process
    pid_t logger_pid = fork();
    if (logger_pid < 0) {
        fprintf(stderr, "[parent] error | fork failed for logger process\n");
        close(logger_pipe[0]);
        close(logger_pipe[1]);
        return EXIT_FAILURE;
    } else if (logger_pid == 0) {
        // logger child process
        close(logger_pipe[1]); // close write end
        logger_process(logger_pipe[0], logfile);
        exit(EXIT_SUCCESS);
    }
    
    // parent process - close read end, keep write end for monitoring children
    close(logger_pipe[0]);
    fprintf(stderr, "[parent] info | created logger process %d\n", logger_pid);
    
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
            usleep(30000); // 30 ms; gives parent time to print info; keeps console output organized and readable
            monitor_file(valid_files[i].c_str(), pipefd[0], logger_pipe[1]);
            
            exit(EXIT_SUCCESS);
        } else {
            // parent process
            
            // close read end of pipe (parent only writes)
            close(pipefd[0]);
            
            child_pids.push_back(pid);
            pipe_write_fds.push_back(pipefd[1]);
            
            fprintf(stderr, "[parent] info | created child process %d with pipe for file '%s'\n",pid, valid_files[i].c_str());
        }

        // small delay to avoid overwhelming output; keeps console output organized and readable
        usleep(100000); // 100 ms
    }
    
    // ------------------------------------------------------------------------
    // task 3 & 4: parent sends "check\n" commands and monitors for 'stop' file
    // ------------------------------------------------------------------------
    fprintf(stderr, "[parent] info | starting monitoring loop...\n");
    fprintf(stderr, "[parent] hint | to stop monitoring, create 'stop' file\n");
    
    // give children time to initialize
    sleep(1);
    
    int check_count = 0;
    bool stop_requested = false;
    
    while (!stop_requested) {
        sleep(1);
        check_count++;
        
        // check for 'stop' file
        if (access("stop", F_OK) == 0) {
            fprintf(stderr, "[parent] info | 'stop' file detected, initiating shutdown...\n");
            stop_requested = true;
            break;
        }

        fprintf(stderr, "[parent] info | sending check command #%d to all children...\n", check_count);
        
        // send "check\n" to all children
        for (size_t i = 0; i < pipe_write_fds.size(); i++) {
            const char* cmd = "check\n";
            ssize_t written = write(pipe_write_fds[i], cmd, strlen(cmd));
            
            if (written < 0) {
                fprintf(stderr, "[parent] error | failed to write to pipe for child %lu\n", i);
            }

            // small delay to give children time to process; keeps console output organized and readable
            usleep(30000); // 30 ms
        }
    }
    
    // cleanup - close all pipes to signal children to exit
    fprintf(stderr, "[parent] info | closing all pipes...\n");
    for (size_t i = 0; i < pipe_write_fds.size(); i++) {
        close(pipe_write_fds[i]);
    }
    
    // close logger pipe write end to signal logger to exit
    close(logger_pipe[1]);
    
    // wait for all children to complete
    fprintf(stderr, "[parent] info | waiting for all child processes to complete...\n");
    int completed = 0;
    for (size_t i = 0; i < child_pids.size(); i++) {
        int status;
        pid_t finished_pid = waitpid(child_pids[i], &status, 0);
        
        if (finished_pid > 0) {
            completed++;
            if (WIFEXITED(status)) {
                fprintf(stderr, "[parent] info | child process %d finished with exit code %d\n", 
                       finished_pid, WEXITSTATUS(status));
            } else {
                fprintf(stderr, "[parent] info | child process %d terminated abnormally\n", finished_pid);
            }
        }
    }
    
    // wait for logger process
    int status;
    waitpid(logger_pid, &status, 0);
    if (WIFEXITED(status)) {
        fprintf(stderr, "[parent] info | logger process %d finished with exit code %d\n", 
               logger_pid, WEXITSTATUS(status));
    }
    
    fprintf(stderr, "[parent] info | all child processes completed (%d/%lu)\n", completed, child_pids.size());
    fprintf(stderr, "[parent] info | monitoring stopped\n");
    fprintf(stderr, "[parent] hint | remember to remove 'stop' file before next run\n");
    
    return EXIT_SUCCESS;
}
