//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
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
#include <netdb.h>
#include <sys/wait.h>

#define STR_CLOSE               "close"

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
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number name\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "    name: name to send to server\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{

    if ( t_narg <= 3 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = nullptr;
    char *l_name = nullptr;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' )
        {
            if ( !l_host )
                l_host = t_args[ i ];
            else if ( !l_port )
                l_port = atoi( t_args[ i ] );
            else if ( !l_name )
                l_name = t_args[ i ];
        }
    }

    if ( !l_host || !l_port || !l_name )
    {
        log_msg( LOG_INFO, "Host, port or name is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Connection to '%s':%d.", l_host, l_port );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( l_host, nullptr, &l_ai_req, &l_ai_ans );
    if ( l_get_ai )
    {
        log_msg( LOG_ERROR, "Unknown host name!" );
        exit( 1 );
    }

    sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( l_port );
    freeaddrinfo( l_ai_ans );

    // socket creation
    int l_sock_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_server == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    // connect to server
    if ( connect( l_sock_server, ( sockaddr * ) &l_cl_addr, sizeof( l_cl_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to connect server." );
        exit( 1 );
    }

    uint l_lsa = sizeof( l_cl_addr );
    // my IP
    getsockname( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
    // server IP
    getpeername( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "Server IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

    // Send name to server
    char l_name_msg[ 128 ];
    snprintf( l_name_msg, sizeof( l_name_msg ), "%s\n", l_name );
    int l_len = write( l_sock_server, l_name_msg, strlen( l_name_msg ) );
    if ( l_len < 0 )
    {
        log_msg( LOG_ERROR, "Unable to send name to server." );
        close( l_sock_server );
        exit( 1 );
    }
    log_msg( LOG_INFO, "Sent name: %s", l_name );

    // Open file for writing received data
    int l_fd = open( "runme.xz", O_WRONLY | O_CREAT | O_TRUNC, 0644 );
    if ( l_fd < 0 )
    {
        log_msg( LOG_ERROR, "Unable to create runme.xz file." );
        close( l_sock_server );
        exit( 1 );
    }

    // Receive data from server and write to file
    char l_buf[ 4096 ];
    int l_total_bytes = 0;
    
    while ( 1 )
    {
        l_len = read( l_sock_server, l_buf, sizeof( l_buf ) );
        if ( l_len == 0 )
        {
            log_msg( LOG_INFO, "Server closed connection. Transfer complete." );
            break;
        }
        else if ( l_len < 0 )
        {
            log_msg( LOG_ERROR, "Unable to read data from server." );
            close( l_fd );
            close( l_sock_server );
            exit( 1 );
        }
        
        // Write to file
        int l_written = write( l_fd, l_buf, l_len );
        if ( l_written < 0 )
        {
            log_msg( LOG_ERROR, "Unable to write to file." );
            close( l_fd );
            close( l_sock_server );
            exit( 1 );
        }
        
        l_total_bytes += l_len;
        log_msg( LOG_DEBUG, "Received %d bytes (total: %d)", l_len, l_total_bytes );
    }

    close( l_fd );
    close( l_sock_server );
    
    log_msg( LOG_INFO, "Received %d bytes total. Saved to runme.xz", l_total_bytes );

    // Fork process to decompress, set permissions, and execute
    pid_t l_pid = fork();
    if ( l_pid < 0 )
    {
        log_msg( LOG_ERROR, "Fork failed." );
        exit( 1 );
    }
    
    if ( l_pid == 0 )
    {
        // Child process - sequential execution
        
        // Step 1: Decompress runme.xz
        pid_t l_pid_xz = fork();
        if ( l_pid_xz < 0 )
        {
            log_msg( LOG_ERROR, "Fork failed for xz." );
            exit( 1 );
        }
        
        if ( l_pid_xz == 0 )
        {
            // Decompress
            execlp( "xz", "xz", "-d", "runme.xz", nullptr );
            log_msg( LOG_ERROR, "Exec xz failed." );
            exit( 1 );
        }
        
        // Wait for decompression
        int l_status;
        waitpid( l_pid_xz, &l_status, 0 );
        
        if ( !WIFEXITED( l_status ) || WEXITSTATUS( l_status ) != 0 )
        {
            log_msg( LOG_ERROR, "Decompression failed." );
            exit( 1 );
        }
        
        // Step 2: Set execute permissions
        pid_t l_pid_chmod = fork();
        if ( l_pid_chmod < 0 )
        {
            log_msg( LOG_ERROR, "Fork failed for chmod." );
            exit( 1 );
        }
        
        if ( l_pid_chmod == 0 )
        {
            // Set permissions
            execlp( "chmod", "chmod", "+x", "runme", nullptr );
            log_msg( LOG_ERROR, "Exec chmod failed." );
            exit( 1 );
        }
        
        // Wait for chmod
        waitpid( l_pid_chmod, &l_status, 0 );
        
        if ( !WIFEXITED( l_status ) || WEXITSTATUS( l_status ) != 0 )
        {
            log_msg( LOG_ERROR, "chmod failed." );
            exit( 1 );
        }
        
        // Step 3: Execute runme
        log_msg( LOG_INFO, "Executing runme..." );
        execlp( "./runme", "./runme", nullptr );
        log_msg( LOG_ERROR, "Exec runme failed." );
        exit( 1 );
    }
    
    // Parent waits for child
    int l_status;
    waitpid( l_pid, &l_status, 0 );
    
    log_msg( LOG_INFO, "Execution completed." );

    return 0;
  }
