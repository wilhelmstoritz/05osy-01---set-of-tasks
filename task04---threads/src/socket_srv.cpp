////////////////////////////////////////////////////////////////////////////////
// producer-consumer socket server with POSIX threads                         //
// each client works as either producer or consumer                           //
////////////////////////////////////////////////////////////////////////////////

//#include "bits/stdc++.h"
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
std::string g_buffer[N];
int g_in_index = 0;     // index for producer to insert
int g_out_index = 0;    // index for consumer to remove

// POSIX semaphores - exactly 3 as required
sem_t g_mutex_sem;      // controls access to critical region
sem_t g_empty_sem;      // counts empty buffer slots
sem_t g_full_sem;       // counts full buffer slots

// mutex for logging to prevent mixed output
pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- logging function --------------------------------------------------------
void log_msg(const char* t_prefix, const char* t_format, ...) {
    pthread_mutex_lock(&g_log_mutex);
    
    char l_buf[1024];
    va_list l_args;
    va_start(l_args, t_format);
    vsnprintf(l_buf, sizeof(l_buf), t_format, l_args);
    va_end(l_args);
    
    std::cout << t_prefix << ": " << l_buf << std::endl;
    
    pthread_mutex_unlock(&g_log_mutex);
}

// --- helper functions for buffer management ----------------------------------
void insert_item(const std::string& t_item) {
    g_buffer[g_in_index] = t_item;
    g_in_index = (g_in_index + 1) % N;
}

std::string remove_item() {
    std::string l_item = g_buffer[g_out_index];
    g_out_index = (g_out_index + 1) % N;
    return l_item;
}

// --- producer function - produces one item -----------------------------------
void producer(const std::string& t_item) {
    sem_wait(&g_empty_sem);     // decrement empty count
    sem_wait(&g_mutex_sem);     // enter critical region
    insert_item(t_item);        // put new item in buffer
    log_msg("BUFFER", "produced: %s", t_item.c_str());
    sem_post(&g_mutex_sem);     // leave critical region
    sem_post(&g_full_sem);      // increment count of full slots
}

// --- consumer function - consumes one item -----------------------------------
std::string consumer() {
    std::string l_item;
    sem_wait(&g_full_sem);      // decrement full count
    sem_wait(&g_mutex_sem);     // enter critical region
    l_item = remove_item();     // take item from buffer
    sem_post(&g_mutex_sem);     // leave critical region
    sem_post(&g_empty_sem);     // increment count of empty slots
    log_msg("BUFFER", "consumed: %s", l_item.c_str());
    return l_item;
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
        
        std::string l_item(l_buf);
        producer(l_item);
        
        // send OK response
        write_line(t_client_socket, "OK\n");
    }
    
    close(t_client_socket);
}

// --- handle consumer client --------------------------------------------------
void handle_consumer(int t_client_socket, const char* t_client_ip) {
    log_msg("CONSUMER", "client %s started as CONSUMER", t_client_ip);
    
    char l_buf[1024];
    while (true) {
        // get item from buffer
        std::string l_item = consumer();
        
        // send item to client
        std::string l_msg = l_item + "\n";
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
}

// --- client handler thread ---------------------------------------------------
void* client_thread(void* t_arg) {
    client_data* l_data = (client_data*)t_arg;
    int l_client_socket = l_data->socket;
    char l_client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(l_data->addr.sin_addr), l_client_ip, INET_ADDRSTRLEN);
    
    delete l_data; // free the allocated structure
    
    // detach thread so it cleans up automatically
    pthread_detach(pthread_self());
    
    // ask client for role
    write_line(l_client_socket, "Task?\n");
    
    // read client's response
    char l_buf[256];
    ssize_t l_len = read_line(l_client_socket, l_buf, sizeof(l_buf));
    
    if (l_len <= 0) {
        log_msg("ERROR", "client %s disconnected before answering", l_client_ip);
        close(l_client_socket);
        return nullptr;
    }
    
    // remove newline
    if (l_buf[l_len-1] == '\n') l_buf[l_len-1] = '\0';
    if (l_len > 1 && l_buf[l_len-2] == '\r') l_buf[l_len-2] = '\0';
    
    // determine role
    if (strcmp(l_buf, "producer") == 0) {
        handle_producer(l_client_socket, l_client_ip);
    } else if (strcmp(l_buf, "consumer") == 0) {
        handle_consumer(l_client_socket, l_client_ip);
    } else {
        log_msg("ERROR", "client %s sent invalid task: %s", l_client_ip, l_buf);
        write_line(l_client_socket, "ERROR: invalid task; use 'producer' or 'consumer'\n");
        close(l_client_socket);
    }
    
    return nullptr;
}

// --- main --------------------------------------------------------------------
int main(int t_argc, char** t_argv) {
    if (t_argc < 2) {
        std::cerr << "usage: " << t_argv[0] << " <port>" << std::endl;
        return EXIT_FAILURE;
    }
    
    int l_port = atoi(t_argv[1]);
    if (l_port <= 0) {
        std::cerr << "invalid port number" << std::endl;
        return EXIT_FAILURE;
    }
    
    // initialize semaphores
    sem_init(&g_mutex_sem, 0, 1);   // mutex starts at 1
    sem_init(&g_empty_sem, 0, N);   // initially all slots are empty
    sem_init(&g_full_sem, 0, 0);    // initially no slots are full

    std::cout << "producer-consumer socket server" << std::endl;
    std::cout << "buffer size: " << N << std::endl;
    std::cout << "listening on port: " << l_port << std::endl;
    std::cout << "enter 'quit' to stop server" << std::endl;
    
    // create listening socket
    int l_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (l_listen_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }
    
    // enable port reuse
    int l_opt = 1;
    if (setsockopt(l_listen_socket, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0) {
        perror("setsockopt");
    }
    
    // bind to port
    sockaddr_in l_srv_addr;
    memset(&l_srv_addr, 0, sizeof(l_srv_addr));
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_addr.s_addr = INADDR_ANY;
    l_srv_addr.sin_port = htons(l_port);
    
    if (bind(l_listen_socket, (sockaddr*)&l_srv_addr, sizeof(l_srv_addr)) < 0) {
        perror("bind");
        close(l_listen_socket);
        return EXIT_FAILURE;
    }
    
    // listen for connections
    if (listen(l_listen_socket, 5) < 0) {
        perror("listen");
        close(l_listen_socket);
        return EXIT_FAILURE;
    }
    
    // main server loop
    while (true) {
        pollfd l_polls[2];
        l_polls[0].fd = STDIN_FILENO;
        l_polls[0].events = POLLIN;
        l_polls[1].fd = l_listen_socket;
        l_polls[1].events = POLLIN;
        
        int l_ret = poll(l_polls, 2, -1);
        if (l_ret < 0) {
            perror("poll");
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
                perror("accept");
                continue;
            }
            
            char l_client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(l_client_addr.sin_addr), l_client_ip, INET_ADDRSTRLEN);
            log_msg("INFO", "new connection from %s:%d", 
                    l_client_ip, ntohs(l_client_addr.sin_port));
            
            // create thread for client
            client_data* l_data = new client_data;
            l_data->socket = l_client_socket;
            l_data->addr = l_client_addr;
            
            pthread_t l_thread;
            if (pthread_create(&l_thread, nullptr, client_thread, l_data) != 0) {
                perror("pthread_create");
                close(l_client_socket);
                delete l_data;
            }
        }
    }
    
    // cleanup
    close(l_listen_socket);
    sem_destroy(&g_mutex_sem);
    sem_destroy(&g_empty_sem);
    sem_destroy(&g_full_sem);
    
    std::cout << "server stopped" << std::endl;
    return EXIT_SUCCESS;
}
