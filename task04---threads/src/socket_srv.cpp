////////////////////////////////////////////////////////////////////////////////
// producer-consumer socket server with POSIX threads                         //
// each client works as either producer or consumer                           //
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <cstring>
#include <cstdarg>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>

#define N 10            // number of slots in the buffer
#define STR_QUIT "quit"

// global buffer implemented as circular buffer
std::string buffer[N];
int in_index = 0;       // index for producer to insert
int out_index = 0;      // index for consumer to remove

// POSIX semaphores - exactly 3 as required
sem_t mutex_sem;        // controls access to critical region
sem_t empty_sem;        // counts empty buffer slots
sem_t full_sem;         // counts full buffer slots

// mutex for logging to prevent mixed output
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- logging function --------------------------------------------------------
void log_msg(const char* prefix, const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    std::cout << prefix << ": " << buf << std::endl;
    
    pthread_mutex_unlock(&log_mutex);
}

// --- helper functions for buffer management ----------------------------------
void insert_item(const std::string& item) {
    buffer[in_index] = item;
    in_index = (in_index + 1) % N;
}

std::string remove_item() {
    std::string item = buffer[out_index];
    out_index = (out_index + 1) % N;
    return item;
}

// --- producer function - produces one item -----------------------------------
void producer(const std::string& item) {
    sem_wait(&empty_sem);           // decrement empty count
    sem_wait(&mutex_sem);           // enter critical region
    insert_item(item);              // put new item in buffer
    log_msg("BUFFER", "Produced: %s", item.c_str());
    sem_post(&mutex_sem);           // leave critical region
    sem_post(&full_sem);            // increment count of full slots
}

// --- consumer function - consumes one item -----------------------------------
std::string consumer() {
    std::string item;
    sem_wait(&full_sem);            // decrement full count
    sem_wait(&mutex_sem);           // enter critical region
    item = remove_item();           // take item from buffer
    sem_post(&mutex_sem);           // leave critical region
    sem_post(&empty_sem);           // increment count of empty slots
    log_msg("BUFFER", "Consumed: %s", item.c_str());
    return item;
}

// --- read line from socket ---------------------------------------------------
ssize_t read_line(int sock, char* buf, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        char c;
        ssize_t n = read(sock, &c, 1);
        if (n <= 0) return n;
        
        buf[i++] = c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return i;
}

// --- write string to socket --------------------------------------------------
ssize_t write_line(int sock, const char* str) {
    return write(sock, str, strlen(str));
}

// --- client thread data structure --------------------------------------------
struct client_data {
    int socket;
    sockaddr_in addr;
};

// --- handle producer client --------------------------------------------------
void handle_producer(int client_socket, const char* client_ip) {
    log_msg("PRODUCER", "client %s started as PRODUCER", client_ip);
    
    char buf[1024];
    while (true) {
        ssize_t len = read_line(client_socket, buf, sizeof(buf));
        
        if (len <= 0) {
            log_msg("PRODUCER", "client %s disconnected", client_ip);
            break;
        }
        
        // remove newline
        if (buf[len-1] == '\n') buf[len-1] = '\0';
        if (len > 1 && buf[len-2] == '\r') buf[len-2] = '\0';
        
        // check for quit command
        if (strcmp(buf, "quit") == 0 || strcmp(buf, "close") == 0) {
            log_msg("PRODUCER", "client %s requested quit", client_ip);
            break;
        }
        
        std::string item(buf);
        producer(item);
        
        // send OK response
        write_line(client_socket, "OK\n");
    }
    
    close(client_socket);
}

// --- handle consumer client --------------------------------------------------
void handle_consumer(int client_socket, const char* client_ip) {
    log_msg("CONSUMER", "client %s started as CONSUMER", client_ip);
    
    char buf[1024];
    while (true) {
        // get item from buffer
        std::string item = consumer();
        
        // send item to client
        std::string msg = item + "\n";
        ssize_t written = write_line(client_socket, msg.c_str());
        
        if (written <= 0) {
            log_msg("CONSUMER", "client %s disconnected", client_ip);
            break;
        }
        
        // wait for OK from client
        ssize_t len = read_line(client_socket, buf, sizeof(buf));
        
        if (len <= 0) {
            log_msg("CONSUMER", "client %s disconnected", client_ip);
            break;
        }
        
        // remove newline
        if (buf[len-1] == '\n') buf[len-1] = '\0';
        
        if (strcmp(buf, "OK") != 0) {
            log_msg("CONSUMER", "client %s sent unexpected response: %s", client_ip, buf);
        }
    }
    
    close(client_socket);
}

// --- client handler thread ---------------------------------------------------
void* client_thread(void* arg) {
    client_data* data = (client_data*)arg;
    int client_socket = data->socket;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    delete data; // free the allocated structure
    
    // detach thread so it cleans up automatically
    pthread_detach(pthread_self());
    
    // ask client for role
    write_line(client_socket, "Task?\n");
    
    // read client's response
    char buf[256];
    ssize_t len = read_line(client_socket, buf, sizeof(buf));
    
    if (len <= 0) {
        log_msg("ERROR", "client %s disconnected before answering", client_ip);
        close(client_socket);
        return nullptr;
    }
    
    // remove newline
    if (buf[len-1] == '\n') buf[len-1] = '\0';
    if (len > 1 && buf[len-2] == '\r') buf[len-2] = '\0';
    
    // determine role
    if (strcmp(buf, "producer") == 0) {
        handle_producer(client_socket, client_ip);
    } else if (strcmp(buf, "consumer") == 0) {
        handle_consumer(client_socket, client_ip);
    } else {
        log_msg("ERROR", "client %s sent invalid task: %s", client_ip, buf);
        write_line(client_socket, "ERROR: invalid task; use 'producer' or 'consumer'\n");
        close(client_socket);
    }
    
    return nullptr;
}

// --- main --------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }
    
    int port = atoi(argv[1]);
    if (port <= 0) {
        std::cerr << "invalid port number" << std::endl;
        return EXIT_FAILURE;
    }
    
    // initialize semaphores
    sem_init(&mutex_sem, 0, 1);     // mutex starts at 1
    sem_init(&empty_sem, 0, N);     // initially all slots are empty
    sem_init(&full_sem, 0, 0);      // initially no slots are full

    std::cout << "producer-consumer socket server" << std::endl;
    std::cout << "buffer size: " << N << std::endl;
    std::cout << "listening on port: " << port << std::endl;
    std::cout << "enter 'quit' to stop server" << std::endl;
    
    // create listening socket
    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    // enable port reuse
    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }
    
    // bind to port
    sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(port);
    
    if (bind(listen_socket, (sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("bind");
        close(listen_socket);
        return EXIT_FAILURE;
    }
    
    // listen for connections
    if (listen(listen_socket, 5) < 0) {
        perror("listen");
        close(listen_socket);
        return EXIT_FAILURE;
    }
    
    // main server loop
    while (true) {
        pollfd polls[2];
        polls[0].fd = STDIN_FILENO;
        polls[0].events = POLLIN;
        polls[1].fd = listen_socket;
        polls[1].events = POLLIN;
        
        int ret = poll(polls, 2, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }
        
        // check for stdin input (quit command)
        if (polls[0].revents & POLLIN) {
            char buf[128];
            int len = read(STDIN_FILENO, buf, sizeof(buf));
            if (len > 0) {
                buf[len] = '\0';
                if (strncmp(buf, STR_QUIT, strlen(STR_QUIT)) == 0) {
                    std::cout << "shutting down server..." << std::endl;
                    break;
                }
            }
        }
        
        // check for new client connection
        if (polls[1].revents & POLLIN) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(listen_socket, (sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("accept");
                continue;
            }
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            log_msg("INFO", "new connection from %s:%d", 
                    client_ip, ntohs(client_addr.sin_port));
            
            // create thread for client
            client_data* data = new client_data;
            data->socket = client_socket;
            data->addr = client_addr;
            
            pthread_t thread;
            if (pthread_create(&thread, nullptr, client_thread, data) != 0) {
                perror("pthread_create");
                close(client_socket);
                delete data;
            }
        }
    }
    
    // cleanup
    close(listen_socket);
    sem_destroy(&mutex_sem);
    sem_destroy(&empty_sem);
    sem_destroy(&full_sem);
    
    std::cout << "server stopped" << std::endl;
    return EXIT_SUCCESS;
}
