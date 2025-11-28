////////////////////////////////////////////////////////////////////////////////
// socket client for producer-consumer with processes                        //
// client can work as producer or consumer based on server's Task? prompt     //
////////////////////////////////////////////////////////////////////////////////

//#include "bits/stdc++.h"
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <cstdarg>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

// --- configuration -----------------------------------------------------------
#define NAMES_FILE              "jmena.txt"
#define DEFAULT_NAMES_PER_MIN   60

// --- log messages ------------------------------------------------------------
#define LOG_ERROR               0   // errors
#define LOG_INFO                1   // information and notifications
#define LOG_DEBUG               2   // debug messages

int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...) {
    if (t_log_level && t_log_level > g_debug) return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsnprintf(l_buf, sizeof(l_buf), t_form, l_arg);
    va_end(l_arg);

    switch (t_log_level) {
    case LOG_INFO:
        std::cout << "INF: " << l_buf << std::endl;
        break;
    case LOG_DEBUG:
        std::cout << "DEB: " << l_buf << std::endl;
        break;
    case LOG_ERROR:
        std::cerr << "ERR: (" << errno << "-" << strerror(errno) << ") " 
                  << l_buf << std::endl;
        break;
    }
}

// --- global variables --------------------------------------------------------
int g_socket = -1;
volatile sig_atomic_t g_running = 1;
int g_names_per_minute = DEFAULT_NAMES_PER_MIN;
pid_t g_child_pid = -1;

// --- helper functions --------------------------------------------------------
// read line from socket
bool read_line(int t_sock, std::string& t_line) {
    char l_ch;
    t_line.clear();
    while (true) {
        int l_n = read(t_sock, &l_ch, 1);
        if (l_n <= 0) return false;
        if (l_ch == '\n') break;
        t_line += l_ch;
    }
    return true;
}

// send line to socket
bool send_line(int t_sock, const std::string& t_line) {
    std::string l_msg = t_line + "\n";
    int l_len = write(t_sock, l_msg.c_str(), l_msg.length());
    return l_len == (int)l_msg.length();
}

// load names from file
std::vector<std::string> load_names(const char* t_filename) {
    std::vector<std::string> l_names;
    std::ifstream l_file(t_filename);
    
    if (!l_file.is_open()) {
        log_msg(LOG_ERROR, "unable to open file: %s", t_filename);
        return l_names;
    }
    
    std::string l_line;
    while (std::getline(l_file, l_line)) {
        if (!l_line.empty()) {
            l_names.push_back(l_line);
        }
    }
    
    l_file.close();
    log_msg(LOG_INFO, "loaded %zu names from file.", l_names.size());
    return l_names;
}

// --- signal handler ----------------------------------------------------------
void signal_handler(int t_sig) {
    (void)t_sig;
    g_running = 0;
}

// --- producer process - sends names to server --------------------------------
void producer_process() {
    log_msg(LOG_INFO, "producer process started (pid: %d)", getpid());
    
    // load names from file
    std::vector<std::string> l_names = load_names(NAMES_FILE);
    if (l_names.empty()) {
        log_msg(LOG_ERROR, "no names loaded, producer process exiting");
        exit(EXIT_FAILURE);
    }
    
    size_t l_name_idx = 0;
    
    while (g_running) {
        // calculate sleep time based on names per minute
        int l_sleep_us = (60 * 1000000) / g_names_per_minute;  // microseconds
        
        // send name to server
        std::string l_name = l_names[l_name_idx];
        if (!send_line(g_socket, l_name)) {
            log_msg(LOG_ERROR, "failed to send name to server");
            break;
        }
        
        log_msg(LOG_INFO, "sent: %s", l_name.c_str());
        
        // wait for OK response
        std::string l_response;
        if (!read_line(g_socket, l_response)) {
            log_msg(LOG_ERROR, "failed to read response from server");
            break;
        }
        
        if (l_response != "OK") {
            log_msg(LOG_INFO, "server response: %s", l_response.c_str());
        }
        
        // move to next name (circular)
        l_name_idx = (l_name_idx + 1) % l_names.size();
        
        // sleep
        usleep(l_sleep_us);
    }
    
    log_msg(LOG_INFO, "producer process exiting");
    close(g_socket);
    exit(EXIT_SUCCESS);
}

// --- consumer process - receives names from server ---------------------------
void consumer_process() {
    log_msg(LOG_INFO, "consumer process started (pid: %d)", getpid());
    
    while (g_running) {
        // read name from server
        std::string l_name;
        if (!read_line(g_socket, l_name)) {
            log_msg(LOG_ERROR, "failed to read from server");
            break;
        }
        
        // display received name
        log_msg(LOG_INFO, "received: %s", l_name.c_str());
        
        // send OK response
        if (!send_line(g_socket, "OK")) {
            log_msg(LOG_ERROR, "failed to send OK to server");
            break;
        }
    }
    
    log_msg(LOG_INFO, "consumer process exiting");
    close(g_socket);
    exit(EXIT_SUCCESS);
}

// --- help --------------------------------------------------------------------
void help(int t_narg, char **t_args) {
    if (t_narg <= 1 || !strcmp(t_args[1], "-h")) {
        std::cout << "\n"
                  << "  socket client for producer-consumer\n"
                  << "\n"
                  << "  usage: " << t_args[0] << " [-h -d] ip_or_name port_number\n"
                  << "\n"
                  << "    -d  debug mode \n"
                  << "    -h  this help\n"
                  << "\n"
                  << "  server will ask 'Task?' - client responds 'producer' or 'consumer'\n"
                  << "\n";

        exit(EXIT_SUCCESS);
    }
}

// --- main --------------------------------------------------------------------
int main(int t_narg, char **t_args) {
    if (t_narg <= 2) help(t_narg, t_args);

    int l_port = 0;
    char *l_host = nullptr;

    // parse arguments
    for (int i = 1; i < t_narg; i++) {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;
        else if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);
        else if (*t_args[i] != '-') {
            if (!l_host)
                l_host = t_args[i];
            else if (!l_port)
                l_port = atoi(t_args[i]);
        }
    }

    if (!l_host || !l_port) {
        log_msg(LOG_INFO, "host or port is missing!");
        help(t_narg, t_args);
        exit(EXIT_FAILURE);
    }

    log_msg(LOG_INFO, "connecting to '%s':%d.", l_host, l_port);

    // resolve hostname
    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req));
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans);
    if (l_get_ai) {
        log_msg(LOG_ERROR, "unknown host name!");
        exit(EXIT_FAILURE);
    }

    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port);
    freeaddrinfo(l_ai_ans);

    // create socket
    g_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socket == -1) {
        log_msg(LOG_ERROR, "unable to create socket");
        exit(EXIT_FAILURE);
    }

    // connect to server
    if (connect(g_socket, (sockaddr *)&l_cl_addr, sizeof(l_cl_addr)) < 0) {
        log_msg(LOG_ERROR, "unable to connect to server");
        exit(EXIT_FAILURE);
    }

    log_msg(LOG_INFO, "connected to server");

    // read Task? prompt from server
    std::string l_task_prompt;
    if (!read_line(g_socket, l_task_prompt)) {
        log_msg(LOG_ERROR, "failed to read task prompt from server");
        close(g_socket);
        exit(EXIT_FAILURE);
    }

    log_msg(LOG_INFO, "server asks: %s", l_task_prompt.c_str());

    // ask user for role
    std::cout << "enter 'producer' or 'consumer': " << std::flush;
    
    char l_role[128];
    if (!fgets(l_role, sizeof(l_role), stdin)) {
        log_msg(LOG_ERROR, "failed to read role from stdin");
        close(g_socket);
        exit(EXIT_FAILURE);
    }
    
    // remove newline
    l_role[strcspn(l_role, "\n")] = 0;

    // Send role to server
    if (!send_line(g_socket, l_role)) {
        log_msg(LOG_ERROR, "failed to send role to server");
        close(g_socket);
        exit(EXIT_FAILURE);
    }

    log_msg(LOG_INFO, "role selected: %s", l_role);

    // setup signal handler for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGCHLD, SIG_IGN);  // auto-reap children

    // start appropriate process based on role
    if (!strcasecmp(l_role, "producer")) {
        // fork child process for producer work
        g_child_pid = fork();
        if (g_child_pid < 0) {
            log_msg(LOG_ERROR, "fork failed");
            close(g_socket);
            exit(EXIT_FAILURE);
        }
        
        if (g_child_pid == 0) {
            // child process - do producer work
            producer_process();
            // producer_process calls exit()
        }
        
        // parent process - wait for quit command or child exit
        log_msg(LOG_INFO, "producer child started (pid: %d)", g_child_pid);
        log_msg(LOG_INFO, "enter 'quit' to stop client");
        
        char l_buffer[128];
        while (g_running && fgets(l_buffer, sizeof(l_buffer), stdin)) {
            l_buffer[strcspn(l_buffer, "\n")] = 0;
            if (!strcmp(l_buffer, "quit") || !strcmp(l_buffer, "close")) {
                break;
            }
        }
        
        // terminate child process
        if (g_child_pid > 0) {
            kill(g_child_pid, SIGTERM);
            waitpid(g_child_pid, nullptr, 0);
        }
        
    } else if (!strcasecmp(l_role, "consumer")) {
        // fork child process for consumer work
        g_child_pid = fork();
        if (g_child_pid < 0) {
            log_msg(LOG_ERROR, "fork failed");
            close(g_socket);
            exit(EXIT_FAILURE);
        }
        
        if (g_child_pid == 0) {
            // child process - do consumer work
            consumer_process();
            // consumer_process calls exit()
        }
        
        // parent process - wait for quit command or child exit
        log_msg(LOG_INFO, "consumer child started (pid: %d)", g_child_pid);
        log_msg(LOG_INFO, "enter 'quit' to stop client");
        
        char l_buffer[128];
        while (g_running && fgets(l_buffer, sizeof(l_buffer), stdin)) {
            l_buffer[strcspn(l_buffer, "\n")] = 0;
            if (!strcmp(l_buffer, "quit") || !strcmp(l_buffer, "close")) {
                break;
            }
        }
        
        // terminate child process
        if (g_child_pid > 0) {
            kill(g_child_pid, SIGTERM);
            waitpid(g_child_pid, nullptr, 0);
        }
        
    } else {
        log_msg(LOG_ERROR, "invalid role: %s", l_role);
        close(g_socket);
        exit(EXIT_FAILURE);
    }

    // cleanup
    close(g_socket);
    log_msg(LOG_INFO, "client finished");

    return EXIT_SUCCESS;
}
