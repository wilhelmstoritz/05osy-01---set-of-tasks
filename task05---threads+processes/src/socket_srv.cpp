////////////////////////////////////////////////////////////////////////////////
// producer-consumer socket server with POSIX IPC (processes)                 //
// each client works as either producer or consumer in separate process       //
// supports both shared memory queue (-shm) and POSIX message queue (-mq)     //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdarg>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <mqueue.h>

#define N 10                    // number of slots in the buffer
#define STR_QUIT "quit"
#define MAX_MSG_SIZE 256        // maximum message size

// IPC names
#define SEM_MUTEX_NAME  "/prodcons_mutex"
#define SEM_EMPTY_NAME  "/prodcons_empty"
#define SEM_FULL_NAME   "/prodcons_full"
#define SHM_NAME        "/prodcons_shm"
#define MQ_NAME         "/prodcons_mq"

// shared memory structure for circular buffer
struct shared_buffer {
    char items[N][MAX_MSG_SIZE];
    int in_index;
    int out_index;
};

// global variables
bool g_use_mq = false;          // true for message queue, false for shared memory
bool g_is_primary = false;      // true if this is the primary server (creates IPC)
sem_t *g_mutex_sem = nullptr;   // named semaphore for mutex
sem_t *g_empty_sem = nullptr;   // named semaphore for empty slots
sem_t *g_full_sem = nullptr;    // named semaphore for full slots
shared_buffer *g_shm_buffer = nullptr;  // pointer to shared memory
mqd_t g_mq = -1;                // message queue descriptor
int g_shm_fd = -1;              // shared memory file descriptor

// --- logging function --------------------------------------------------------
void log_msg(const char* t_prefix, const char* t_format, ...) {
    char l_buf[1024];
    va_list l_args;
    va_start(l_args, t_format);
    vsnprintf(l_buf, sizeof(l_buf), t_format, l_args);
    va_end(l_args);
    
    std::cout << "[" << getpid() << "] " << t_prefix << ": " << l_buf << std::endl;
}

// --- helper functions for buffer management ----------------------------------
void insert_item_shm(const char* t_item) {
    strncpy(g_shm_buffer->items[g_shm_buffer->in_index], t_item, MAX_MSG_SIZE - 1);
    g_shm_buffer->items[g_shm_buffer->in_index][MAX_MSG_SIZE - 1] = '\0';
    g_shm_buffer->in_index = (g_shm_buffer->in_index + 1) % N;
}

void remove_item_shm(char* t_item) {
    strncpy(t_item, g_shm_buffer->items[g_shm_buffer->out_index], MAX_MSG_SIZE - 1);
    t_item[MAX_MSG_SIZE - 1] = '\0';
    g_shm_buffer->out_index = (g_shm_buffer->out_index + 1) % N;
}

void insert_item_mq(const char* t_item) {
    if (mq_send(g_mq, t_item, strlen(t_item) + 1, 0) < 0) {
        log_msg("ERROR", "mq_send failed: %s", strerror(errno));
    }
}

void remove_item_mq(char* t_item) {
    ssize_t l_bytes = mq_receive(g_mq, t_item, MAX_MSG_SIZE, nullptr);
    if (l_bytes < 0) {
        log_msg("ERROR", "mq_receive failed: %s", strerror(errno));
        t_item[0] = '\0';
    } else {
        t_item[l_bytes] = '\0';
    }
}

// --- producer function - produces one item -----------------------------------
void producer(const char* t_item) {
    if (g_use_mq) {
        // message queue doesn't need empty semaphore (has built-in capacity)
        insert_item_mq(t_item);
        log_msg("BUFFER", "produced (mq): %s", t_item);
    } else {
        sem_wait(g_empty_sem);      // decrement empty count
        sem_wait(g_mutex_sem);      // enter critical region
        insert_item_shm(t_item);    // put new item in buffer
        log_msg("BUFFER", "produced (shm): %s", t_item);
        sem_post(g_mutex_sem);      // leave critical region
        sem_post(g_full_sem);       // increment count of full slots
    }
}

// --- consumer function - consumes one item -----------------------------------
void consumer(char* t_item) {
    if (g_use_mq) {
        // message queue handles synchronization internally
        remove_item_mq(t_item);
        log_msg("BUFFER", "consumed (mq): %s", t_item);
    } else {
        sem_wait(g_full_sem);       // decrement full count
        sem_wait(g_mutex_sem);      // enter critical region
        remove_item_shm(t_item);    // take item from buffer
        sem_post(g_mutex_sem);      // leave critical region
        sem_post(g_empty_sem);      // increment count of empty slots
        log_msg("BUFFER", "consumed (shm): %s", t_item);
    }
}

// --- read line from socket ---------------------------------------------------
ssize_t read_line(int t_sock, char* t_buf, size_t t_max_len) {
    size_t i = 0;
    while (i < t_max_len - 1) {
        char l_c;
        ssize_t l_n = read(t_sock, &l_c, 1);
        if (l_n <= 0) return l_n;
        
        t_buf[i++] = l_c;
        if (l_c == '\n') break;
    }
    t_buf[i] = '\0';
    return i;
}

// --- write string to socket --------------------------------------------------
ssize_t write_line(int t_sock, const char* t_str) {
    return write(t_sock, t_str, strlen(t_str));
}

// --- client thread data structure --------------------------------------------
struct client_data {
    int socket;
    sockaddr_in addr;
};

// --- handle producer client --------------------------------------------------
void handle_producer(int t_client_socket, const char* t_client_ip) {
    log_msg("PRODUCER", "client %s started as PRODUCER", t_client_ip);
    
    char l_buf[1024];
    while (true) {
        ssize_t l_len = read_line(t_client_socket, l_buf, sizeof(l_buf));
        
        if (l_len <= 0) {
            log_msg("PRODUCER", "client %s disconnected", t_client_ip);
            break;
        }
        
        // remove newline
        if (l_buf[l_len-1] == '\n') l_buf[l_len-1] = '\0';
        if (l_len > 1 && l_buf[l_len-2] == '\r') l_buf[l_len-2] = '\0';
        
        // check for quit command
        if (strcmp(l_buf, "quit") == 0 || strcmp(l_buf, "close") == 0) {
            log_msg("PRODUCER", "client %s requested quit", t_client_ip);
            break;
        }
        
        producer(l_buf);
        
        // send OK response
        write_line(t_client_socket, "OK\n");
    }
    
    close(t_client_socket);
    exit(EXIT_SUCCESS);
}

// --- handle consumer client --------------------------------------------------
void handle_consumer(int t_client_socket, const char* t_client_ip) {
    log_msg("CONSUMER", "client %s started as CONSUMER", t_client_ip);
    
    char l_buf[1024];
    char l_item[MAX_MSG_SIZE];
    
    while (true) {
        // get item from buffer
        consumer(l_item);
        
        // send item to client
        std::string l_msg = std::string(l_item) + "\n";
        ssize_t l_written = write_line(t_client_socket, l_msg.c_str());
        
        if (l_written <= 0) {
            log_msg("CONSUMER", "client %s disconnected", t_client_ip);
            break;
        }
        
        // wait for OK from client
        ssize_t l_len = read_line(t_client_socket, l_buf, sizeof(l_buf));
        
        if (l_len <= 0) {
            log_msg("CONSUMER", "client %s disconnected", t_client_ip);
            break;
        }
        
        // remove newline
        if (l_buf[l_len-1] == '\n') l_buf[l_len-1] = '\0';
        
        if (strcmp(l_buf, "OK") != 0) {
            log_msg("CONSUMER", "client %s sent unexpected response: %s", t_client_ip, l_buf);
        }
    }
    
    close(t_client_socket);
    exit(EXIT_SUCCESS);
}

// --- client handler process --------------------------------------------------
void handle_client_process(int t_client_socket, sockaddr_in t_client_addr) {
    char l_client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(t_client_addr.sin_addr), l_client_ip, INET_ADDRSTRLEN);
    
    // ask client for role
    write_line(t_client_socket, "Task?\n");
    
    // read client's response
    char l_buf[256];
    ssize_t l_len = read_line(t_client_socket, l_buf, sizeof(l_buf));
    
    if (l_len <= 0) {
        log_msg("ERROR", "client %s disconnected before answering", l_client_ip);
        close(t_client_socket);
        exit(EXIT_FAILURE);
    }
    
    // remove newline
    if (l_buf[l_len-1] == '\n') l_buf[l_len-1] = '\0';
    if (l_len > 1 && l_buf[l_len-2] == '\r') l_buf[l_len-2] = '\0';
    
    // determine role
    if (strcmp(l_buf, "producer") == 0) {
        handle_producer(t_client_socket, l_client_ip);
    } else if (strcmp(l_buf, "consumer") == 0) {
        handle_consumer(t_client_socket, l_client_ip);
    } else {
        log_msg("ERROR", "client %s sent invalid task: %s", l_client_ip, l_buf);
        write_line(t_client_socket, "ERROR: invalid task; use 'producer' or 'consumer'\n");
        close(t_client_socket);
        exit(EXIT_FAILURE);
    }
}

// --- IPC initialization ------------------------------------------------------
bool init_ipc(bool t_use_mq) {
    // try to create IPC resources exclusively (primary server)
    // if that fails, try to open existing ones (secondary server)
    
    // try to create mutex semaphore exclusively
    g_mutex_sem = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (g_mutex_sem == SEM_FAILED) {
        if (errno == EEXIST) {
            // IPC already exists, open as secondary server
            g_is_primary = false;
            std::cout << "connecting to existing IPC resources (secondary server)" << std::endl;
            
            g_mutex_sem = sem_open(SEM_MUTEX_NAME, 0);
            if (g_mutex_sem == SEM_FAILED) {
                std::cerr << "sem_open (mutex) failed: " << strerror(errno) << std::endl;
                return false;
            }
        } else {
            std::cerr << "sem_open (mutex) failed: " << strerror(errno) << std::endl;
            return false;
        }
    } else {
        g_is_primary = true;
        std::cout << "creating new IPC resources (primary server)" << std::endl;
    }
    
    if (t_use_mq) {
        // message queue mode
        if (g_is_primary) {
            struct mq_attr l_attr;
            l_attr.mq_flags = 0;
            l_attr.mq_maxmsg = N;
            l_attr.mq_msgsize = MAX_MSG_SIZE;
            l_attr.mq_curmsgs = 0;
            
            g_mq = mq_open(MQ_NAME, O_CREAT | O_EXCL | O_RDWR, 0644, &l_attr);
            if (g_mq == (mqd_t)-1) {
                std::cerr << "mq_open failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_unlink(SEM_MUTEX_NAME);
                return false;
            }
        } else {
            g_mq = mq_open(MQ_NAME, O_RDWR);
            if (g_mq == (mqd_t)-1) {
                std::cerr << "mq_open failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                return false;
            }
        }
        
        std::cout << "using POSIX message queue" << std::endl;
    } else {
        // shared memory mode - need all three semaphores
        if (g_is_primary) {
            g_empty_sem = sem_open(SEM_EMPTY_NAME, O_CREAT | O_EXCL, 0644, N);
            if (g_empty_sem == SEM_FAILED) {
                std::cerr << "sem_open (empty) failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_unlink(SEM_MUTEX_NAME);
                return false;
            }
            
            g_full_sem = sem_open(SEM_FULL_NAME, O_CREAT | O_EXCL, 0644, 0);
            if (g_full_sem == SEM_FAILED) {
                std::cerr << "sem_open (full) failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_close(g_empty_sem);
                sem_unlink(SEM_MUTEX_NAME);
                sem_unlink(SEM_EMPTY_NAME);
                return false;
            }
            
            // create shared memory
            g_shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0644);
            if (g_shm_fd < 0) {
                std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_close(g_empty_sem);
                sem_close(g_full_sem);
                sem_unlink(SEM_MUTEX_NAME);
                sem_unlink(SEM_EMPTY_NAME);
                sem_unlink(SEM_FULL_NAME);
                return false;
            }
            
            // set size of shared memory
            if (ftruncate(g_shm_fd, sizeof(shared_buffer)) < 0) {
                std::cerr << "ftruncate failed: " << strerror(errno) << std::endl;
                close(g_shm_fd);
                shm_unlink(SHM_NAME);
                sem_close(g_mutex_sem);
                sem_close(g_empty_sem);
                sem_close(g_full_sem);
                sem_unlink(SEM_MUTEX_NAME);
                sem_unlink(SEM_EMPTY_NAME);
                sem_unlink(SEM_FULL_NAME);
                return false;
            }
        } else {
            // secondary server - open existing semaphores and shared memory
            g_empty_sem = sem_open(SEM_EMPTY_NAME, 0);
            if (g_empty_sem == SEM_FAILED) {
                std::cerr << "sem_open (empty) failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                return false;
            }
            
            g_full_sem = sem_open(SEM_FULL_NAME, 0);
            if (g_full_sem == SEM_FAILED) {
                std::cerr << "sem_open (full) failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_close(g_empty_sem);
                return false;
            }
            
            g_shm_fd = shm_open(SHM_NAME, O_RDWR, 0644);
            if (g_shm_fd < 0) {
                std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
                sem_close(g_mutex_sem);
                sem_close(g_empty_sem);
                sem_close(g_full_sem);
                return false;
            }
        }
        
        // map shared memory (both primary and secondary)
        g_shm_buffer = (shared_buffer*)mmap(nullptr, sizeof(shared_buffer),
                                             PROT_READ | PROT_WRITE, MAP_SHARED,
                                             g_shm_fd, 0);
        if (g_shm_buffer == MAP_FAILED) {
            std::cerr << "mmap failed: " << strerror(errno) << std::endl;
            close(g_shm_fd);
            if (g_is_primary) shm_unlink(SHM_NAME);
            sem_close(g_mutex_sem);
            sem_close(g_empty_sem);
            sem_close(g_full_sem);
            if (g_is_primary) {
                sem_unlink(SEM_MUTEX_NAME);
                sem_unlink(SEM_EMPTY_NAME);
                sem_unlink(SEM_FULL_NAME);
            }
            return false;
        }
        
        // initialize buffer indices (only primary)
        if (g_is_primary) {
            g_shm_buffer->in_index = 0;
            g_shm_buffer->out_index = 0;
            memset(g_shm_buffer->items, 0, sizeof(g_shm_buffer->items));
        }
        
        std::cout << "using shared memory circular buffer" << std::endl;
    }
    
    return true;
}

// --- IPC cleanup -------------------------------------------------------------
void cleanup_ipc(bool t_use_mq) {
    if (t_use_mq) {
        if (g_mq != (mqd_t)-1) {
            mq_close(g_mq);
            if (g_is_primary) mq_unlink(MQ_NAME);
        }
    } else {
        if (g_shm_buffer != nullptr && g_shm_buffer != MAP_FAILED) {
            munmap(g_shm_buffer, sizeof(shared_buffer));
        }
        if (g_shm_fd >= 0) {
            close(g_shm_fd);
            if (g_is_primary) shm_unlink(SHM_NAME);
        }
        if (g_empty_sem != nullptr) {
            sem_close(g_empty_sem);
            if (g_is_primary) sem_unlink(SEM_EMPTY_NAME);
        }
        if (g_full_sem != nullptr) {
            sem_close(g_full_sem);
            if (g_is_primary) sem_unlink(SEM_FULL_NAME);
        }
    }
    
    if (g_mutex_sem != nullptr) {
        sem_close(g_mutex_sem);
        if (g_is_primary) sem_unlink(SEM_MUTEX_NAME);
    }
}

// --- main --------------------------------------------------------------------
int main(int t_argc, char** t_argv) {
    if (t_argc < 3) {
        std::cerr << "usage: " << t_argv[0] << " <-shm|-mq> <port>" << std::endl;
        std::cerr << "  -shm  use shared memory circular buffer" << std::endl;
        std::cerr << "  -mq   use POSIX message queue" << std::endl;
        return EXIT_FAILURE;
    }
    
    // parse IPC mode
    if (strcmp(t_argv[1], "-shm") == 0) {
        g_use_mq = false;
    } else if (strcmp(t_argv[1], "-mq") == 0) {
        g_use_mq = true;
    } else {
        std::cerr << "invalid mode: " << t_argv[1] << std::endl;
        std::cerr << "use -shm or -mq" << std::endl;
        return EXIT_FAILURE;
    }
    
    int l_port = atoi(t_argv[2]);
    if (l_port <= 0) {
        std::cerr << "invalid port number" << std::endl;
        return EXIT_FAILURE;
    }
    
    // initialize IPC resources
    if (!init_ipc(g_use_mq)) {
        std::cerr << "failed to initialize IPC resources" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "producer-consumer socket server (process-based)" << std::endl;
    std::cout << "buffer size: " << N << std::endl;
    std::cout << "listening on port: " << l_port << std::endl;
    std::cout << "enter 'quit' to stop server" << std::endl;
    
    // create listening socket
    int l_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (l_listen_socket < 0) {
        std::cerr << "socket error: " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
    
    // enable port reuse
    int l_opt = 1;
    if (setsockopt(l_listen_socket, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0) {
        std::cerr << "setsockopt error: " << strerror(errno) << std::endl;
    }
    
    // bind to port
    sockaddr_in l_srv_addr;
    memset(&l_srv_addr, 0, sizeof(l_srv_addr));
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_addr.s_addr = INADDR_ANY;
    l_srv_addr.sin_port = htons(l_port);
    
    if (bind(l_listen_socket, (sockaddr*)&l_srv_addr, sizeof(l_srv_addr)) < 0) {
        std::cerr << "bind error: " << strerror(errno) << std::endl;
        close(l_listen_socket);
        return EXIT_FAILURE;
    }
    
    // listen for connections
    if (listen(l_listen_socket, 5) < 0) {
        std::cerr << "listen error: " << strerror(errno) << std::endl;
        close(l_listen_socket);
        return EXIT_FAILURE;
    }
    
    // main server loop
    while (true) {
        // reap zombie processes
        while (waitpid(-1, nullptr, WNOHANG) > 0);
        
        pollfd l_polls[2];
        l_polls[0].fd = STDIN_FILENO;
        l_polls[0].events = POLLIN;
        l_polls[1].fd = l_listen_socket;
        l_polls[1].events = POLLIN;
        
        int l_ret = poll(l_polls, 2, -1);
        if (l_ret < 0) {
            if (errno == EINTR) continue;
            std::cerr << "poll error: " << strerror(errno) << std::endl;
            break;
        }
        
        // check for stdin input (quit command)
        if (l_polls[0].revents & POLLIN) {
            char l_buf[128];
            int l_len = read(STDIN_FILENO, l_buf, sizeof(l_buf));
            if (l_len > 0) {
                l_buf[l_len] = '\0';
                if (strncmp(l_buf, STR_QUIT, strlen(STR_QUIT)) == 0) {
                    std::cout << "shutting down server..." << std::endl;
                    break;
                }
            }
        }
        
        // check for new client connection
        if (l_polls[1].revents & POLLIN) {
            sockaddr_in l_client_addr;
            socklen_t l_client_len = sizeof(l_client_addr);
            
            int l_client_socket = accept(l_listen_socket, (sockaddr*)&l_client_addr, &l_client_len);
            if (l_client_socket < 0) {
                std::cerr << "accept error: " << strerror(errno) << std::endl;
                continue;
            }
            
            char l_client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(l_client_addr.sin_addr), l_client_ip, INET_ADDRSTRLEN);
            log_msg("INFO", "new connection from %s:%d", 
                    l_client_ip, ntohs(l_client_addr.sin_port));
            
            // fork process for client
            pid_t l_pid = fork();
            if (l_pid < 0) {
                std::cerr << "fork error: " << strerror(errno) << std::endl;
                close(l_client_socket);
                continue;
            }
            
            if (l_pid == 0) {
                // child process
                close(l_listen_socket);  // child doesn't need listening socket
                handle_client_process(l_client_socket, l_client_addr);
                // handle_client_process will call exit()
            } else {
                // parent process
                close(l_client_socket);  // parent doesn't need client socket
                log_msg("INFO", "forked process %d for client", l_pid);
            }
        }
    }
    
    // cleanup
    close(l_listen_socket);
    cleanup_ipc(g_use_mq);
    
    // wait for any remaining child processes
    while (waitpid(-1, nullptr, WNOHANG) > 0);
    
    std::cout << "server stopped" << std::endl;
    return EXIT_SUCCESS;
}
