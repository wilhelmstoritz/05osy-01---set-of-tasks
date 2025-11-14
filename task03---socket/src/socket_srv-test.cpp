//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// Handle client communication in child process

void handle_client( int t_client_socket )
{
    char l_buf[ 256 ];
    
    // Read name from client
    int l_len = read( t_client_socket, l_buf, sizeof( l_buf ) - 1 );
    if ( l_len <= 0 )
    {
        log_msg( LOG_ERROR, "Unable to read name from client." );
        close( t_client_socket );
        exit( 1 );
    }
    
    l_buf[ l_len ] = 0; // null terminate
    
    // Remove newline if present
    char *newline = strchr( l_buf, '\n' );
    if ( newline ) *newline = 0;
    
    log_msg( LOG_INFO, "Client sent name: %s", l_buf );
    
    // Redirect stdout to socket
    dup2( t_client_socket, STDOUT_FILENO );
    close( t_client_socket );
    
    // Build compilation command with name
    char l_define_arg[ 256 ];
    snprintf( l_define_arg, sizeof( l_define_arg ), "NAME=%s", l_buf );
    
    // Step 1: Compile pozdrav.cpp with -D NAME=name
    pid_t l_pid_compile = fork();
    if ( l_pid_compile < 0 )
    {
        log_msg( LOG_ERROR, "Fork failed for compilation." );
        exit( 1 );
    }
    
    if ( l_pid_compile == 0 )
    {
        // Child process - compile
        execlp( "g++", "g++", "-D", l_define_arg, "pozdrav.cpp", "-o", "out.bin", nullptr );
        log_msg( LOG_ERROR, "Exec g++ failed." );
        exit( 1 );
    }
    
    // Wait for compilation to finish
    int l_status;
    waitpid( l_pid_compile, &l_status, 0 );
    
    if ( !WIFEXITED( l_status ) || WEXITSTATUS( l_status ) != 0 )
    {
        log_msg( LOG_ERROR, "Compilation failed." );
        exit( 1 );
    }
    
    // Step 2: Compress with xz and send to stdout (which is now the socket)
    // No need to fork here - execlp replaces the current process with xz.
    // After xz finishes, the process exits automatically, which is what we want.
    execlp( "xz", "xz", "out.bin", "--stdout", nullptr );
    log_msg( LOG_ERROR, "Exec xz failed." );
    exit( 1 );
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port )
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // go!
    while ( 1 )
    {
        // Check for terminated child processes
        int l_status;
        while ( waitpid( -1, &l_status, WNOHANG ) > 0 )
        {
            log_msg( LOG_DEBUG, "Child process terminated." );
        }

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO;
        l_read_poll[ 0 ].events = POLLIN;
        l_read_poll[ 1 ].fd = l_sock_listen;
        l_read_poll[ 1 ].events = POLLIN;

        // select from fds
        int l_poll = poll( l_read_poll, 2, -1 );

        if ( l_poll < 0 )
        {
            log_msg( LOG_ERROR, "Function poll failed!" );
            exit( 1 );
        }

        if ( l_read_poll[ 0 ].revents & POLLIN )
        { // data on stdin
            char buf[ 128 ];
            
            int l_len = read( STDIN_FILENO, buf, sizeof( buf) );
            if ( l_len == 0 )
            {
                log_msg( LOG_DEBUG, "Stdin closed." );
                exit( 0 );
            }
            if ( l_len < 0 )
            {
                log_msg( LOG_DEBUG, "Unable to read from stdin!" );
                exit( 1 );
            }

            log_msg( LOG_DEBUG, "Read %d bytes from stdin", l_len );
            // request to quit?
            if ( !strncmp( buf, STR_QUIT, strlen( STR_QUIT ) ) )
            {
                log_msg( LOG_INFO, "Request to 'quit' entered.");
                close( l_sock_listen );
                exit( 0 );
            }
        }

        if ( l_read_poll[ 1 ].revents & POLLIN )
        { // new client connection
            sockaddr_in l_rsa;
            int l_rsa_size = sizeof( l_rsa );
            
            // Accept new connection
            int l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
            if ( l_sock_client == -1 )
            {
                log_msg( LOG_ERROR, "Unable to accept new client." );
                continue;
            }
            
            uint l_lsa = sizeof( l_srv_addr );
            // my IP
            getsockname( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
            log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                             inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
            // client IP
            getpeername( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
            log_msg( LOG_INFO, "Client IP: '%s'  port: %d",
                             inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );

            // Fork to handle client
            pid_t l_pid = fork();
            if ( l_pid < 0 )
            {
                log_msg( LOG_ERROR, "Fork failed for client handler." );
                close( l_sock_client );
                continue;
            }
            
            if ( l_pid == 0 )
            {
                // Child process - handle client
                close( l_sock_listen ); // Child doesn't need listening socket
                handle_client( l_sock_client );
                // handle_client will exit
            }
            else
            {
                // Parent process
                close( l_sock_client ); // Parent doesn't need client socket
                log_msg( LOG_INFO, "Created child process %d for client.", l_pid );
            }
        }
    } // while ( 1 )

    return 0;
}
