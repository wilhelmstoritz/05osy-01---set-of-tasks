
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "gthr.h"

struct gt_context_t g_gttbl[ MaxGThreads ]; // statically allocated table for thread control
struct gt_context_t * g_gtcur;              // pointer to current thread
uint64_t g_start_time_ms = 0;               // scheduler start time in milliseconds

// --- get current time in milliseconds ----------------------------------------
static uint64_t gt_get_time_ms( void ) {
    struct timeval l_tv;
    gettimeofday( &l_tv, NULL );

    return (uint64_t)l_tv.tv_sec * 1000 + l_tv.tv_usec / 1000;
}

#if ( GT_PREEMPTIVE != 0 )

volatile int g_gt_yield_status = 1;         // yield status 

void gt_sig_handle( int t_sig );

// initialize and start SIGALRM
void gt_sig_start( void )
{
    struct sigaction l_sig_act;
    memset( &l_sig_act, 0, sizeof( l_sig_act ) );
    l_sig_act.sa_handler = gt_sig_handle;

    sigaction( SIGALRM, &l_sig_act, NULL );

    struct itimerval l_tv_alarm = { { 0, TimePeriod * 1000 }, { 0, TimePeriod * 1000 } };
    setitimer( ITIMER_REAL, & l_tv_alarm, NULL );
}

// deinitialize and stop SIGALRM
void gt_sig_stop( void )
{
    struct itimerval l_tv_alarm = { { 0, 0 }, { 0, 0 } };
    setitimer( ITIMER_REAL, & l_tv_alarm, NULL );

    struct sigaction l_sig_act;
    memset( &l_sig_act, 0, sizeof( l_sig_act ) );
    l_sig_act.sa_handler = SIG_DFL;

    sigaction( SIGALRM, &l_sig_act, NULL );
}

// unblock SIGALRM
void gt_sig_mask_reset( void ) 
{
    sigset_t l_set;                     // Create signal set
    sigemptyset( & l_set );             // Clear it
    sigaddset( & l_set, SIGALRM );      // Set signal (we use SIGALRM)

    sigprocmask( SIG_UNBLOCK, & l_set, NULL );  // Fetch and change the signal mask
}

// function triggered periodically by timer (SIGALRM)
void gt_sig_handle( int t_sig ) 
{
    gt_sig_mask_reset();                // enable SIGALRM again

    // --- manage timers - check all blocked tasks for delay expiration --------
    uint64_t l_now = gt_get_time_ms();
    for ( struct gt_context_t * p = &g_gttbl[ 0 ]; p < &g_gttbl[ MaxGThreads ]; p++ ) {
        if ( p->thread_state == Blocked && p->delay_until > 0 && l_now >= p->delay_until ) {
            p->delay_until = 0;
            p->thread_state = Ready;
        }
    }

    g_gt_yield_status = gt_yield();     // scheduler
}

#endif

// initialize first thread as current context (and start timer)
void gt_init( void ) 
{
    // --- clear all thread contexts -------------------------------------------
    memset( g_gttbl, 0, sizeof( g_gttbl ) );
    // -------------------------------------------------------------------------
    
    g_gtcur = & g_gttbl[ 0 ];           // initialize current thread with thread #0
    g_gtcur -> thread_state = Running;  // set current to running

    // --- set name of initial thread ------------------------------------------
    strncpy( g_gtcur->name, "main", MaxTaskName - 1 );
    g_gtcur->delay_until = 0;
    
    g_start_time_ms = gt_get_time_ms(); // remember start time
}


// exit thread
void gt_stop( void ) 
{
    if ( g_gtcur != & g_gttbl[ 0 ] )    // if not an initial thread,
    {
        g_gtcur -> thread_state = Unused;// set current thread as unused
        gt_yield();                     // yield and make possible to switch to another thread
        assert( !"reachable" );         // this code should never be reachable ... (if yes, returning function on stack was corrupted)
    }
}

void gt_start_scheduler( void )
{
    // --- start time measurement ----------------------------------------------
    g_start_time_ms = gt_get_time_ms(); // remember start time
    // -------------------------------------------------------------------------

#if ( GT_PREEMPTIVE != 0 )
    gt_sig_start();                     // gt_yield() will be called from gt_sig_handler()
    while ( g_gt_yield_status )
    {
        usleep( TimePeriod * 1000 + 1000 ); // idle, simulate CPU HW sleep
    }
    gt_sig_stop();
#else
    while ( gt_yield() ) {}             // if initial thread, wait for other to terminate
#endif
}


// switch from one thread to other
int gt_yield( void ) 
{
    struct gt_context_t * p;
    struct gt_regs * l_old, * l_new;
    int l_no_ready = 0;                 // not ready processes

    // --- check timers and unblock tasks if delay expired ---------------------
#if ( GT_PREEMPTIVE == 0 )
    // In cooperative mode, check timers here
    uint64_t l_now = gt_get_time_ms();
    for ( struct gt_context_t * t = &g_gttbl[ 0 ]; t < &g_gttbl[ MaxGThreads ]; t++ ) {
        if ( t->thread_state == Blocked && t->delay_until > 0 && l_now >= t->delay_until ) {
            t->delay_until = 0;
            t->thread_state = Ready;
        }
    }
#endif
    // -------------------------------------------------------------------------

    p = g_gtcur;
    while ( p -> thread_state != Ready )// iterate through g_gttbl[] until we find new thread in state Ready 
    {
        if ( p -> thread_state == Blocked || p -> thread_state == Suspended ) 
            l_no_ready++;               // number of Blocked and Suspend processes

        if ( ++p == & g_gttbl[ MaxGThreads ] )// at the end rotate to the beginning
            p = & g_gttbl[ 0 ];
        if ( p == g_gtcur )             // did not find any other Ready threads
        {
            return - l_no_ready;        // no task ready, or task table empty
        }
    }

    if ( g_gtcur -> thread_state == Running )// switch current to Ready and new thread found in previous loop to Running
        g_gtcur -> thread_state = Ready;
    p -> thread_state = Running;
    l_old = & g_gtcur -> regs;          // prepare pointers to context of current (will become old) 
    l_new = & p -> regs;                // and new to new thread found in previous loop
    g_gtcur = p;                        // switch current indicator to new thread
#if ( GT_PREEMPTIVE != 0 )
    gt_pree_swtch( l_old, l_new );      // perform context switch (assembly in gtswtch.S)
#else
    gt_swtch( l_old, l_new );           // perform context switch (assembly in gtswtch.S)
#endif
    return 1;
}


// create new thread by providing pointer to function that will act like "run" method
int gt_go( void ( * t_run )( void ), const char *t_name, gt_handle_t *t_handle ) 
{
    char * l_stack;
    struct gt_context_t * p;
    
    for ( p = & g_gttbl[ 0 ];; p++ )            // find an empty slot
        if ( p == & g_gttbl[ MaxGThreads ] )    // if we have reached the end, gttbl is full and we cannot create a new thread
            return -1;
        else if ( p -> thread_state == Unused )
            break;                              // new slot was found

    l_stack = ( char * ) malloc( StackSize );   // allocate memory for stack of newly created thread
    if ( !l_stack )
        return -1;

    *( uint64_t * ) & l_stack[ StackSize - 8 ] = ( uint64_t ) gt_stop;  //  put into the stack returning function gt_stop in case function calls return
    *( uint64_t * ) & l_stack[ StackSize - 16 ] = ( uint64_t ) t_run;   //  put provided function as a main "run" function
    p -> regs.rsp = ( uint64_t ) & l_stack[ StackSize - 16 ];           //  set stack pointer
    p -> thread_state = Ready;                                          //  set state
    p -> delay_until = 0;                                               //  no delay
    
    // set task name
    if ( t_name )
        strncpy( p->name, t_name, MaxTaskName - 1 );
    else
        snprintf( p->name, MaxTaskName, "task%ld", p - g_gttbl );
    p->name[ MaxTaskName - 1 ] = '\0';
    
    // return handle if requested
    if ( t_handle )
        *t_handle = p;

    return 0;
}


// --- suspend a task (NULL = current task, like FreeRTOS vTaskSuspend) --------
void gt_suspend( gt_handle_t t_handle ) {
    struct gt_context_t * p = t_handle ? t_handle : g_gtcur;
    
    if ( p->thread_state == Running || p->thread_state == Ready || p->thread_state == Blocked ) {
        p->thread_state = Suspended;
        p->delay_until = 0;  // cancel any pending delay
        
        // if suspending current task, yield to another
        if ( p == g_gtcur )
            gt_yield();
    }
}


// --- resume a suspended task (like FreeRTOS vTaskResume) ---------------------
void gt_resume( gt_handle_t t_handle ) {
    if ( t_handle && t_handle->thread_state == Suspended ) {
        t_handle->thread_state = Ready;
    }
}


// --- delay current task for specified milliseconds (like FreeRTOS vTaskDelay)
void gt_delay( uint32_t t_ms ) {
    if ( g_gtcur == &g_gttbl[0] )
        return;  // don't block main/scheduler thread
    
    g_gtcur->delay_until = gt_get_time_ms() + t_ms;
    g_gtcur->thread_state = Blocked;
    gt_yield();
}


// --- print list of all tasks (like FreeRTOS vTaskList) -----------------------
void gt_task_list( void ) {
    static const char * l_state_names[] = { "Unused", "Running", "Ready", "Blocked", "Suspended" };
    
    printf( "\n%-16s %-10s %s\n", "task name", "state", "ID" );
    printf( "----------------------------------------\n" );
    
    for ( int i = 0; i < MaxGThreads; i++ ) {
        struct gt_context_t * p = &g_gttbl[i];
        if ( p->thread_state != Unused ) {
            printf( "%-16s %-10s %d\n", 
                    p->name, 
                    l_state_names[ p->thread_state ],
                    i );
        }
    }
    printf( "\n" );
}


int uninterruptibleNanoSleep( time_t t_sec, long t_nanosec ) 
{
#if 0
    struct timeval l_tv_cur, l_tv_limit;
    struct timeval l_tv_tmp = { t_sec, t_nanosec / 1000 };
    gettimeofday( &l_tv_cur, NULL );
    timeradd( &l_tv_cur, &l_tv_tmp, &l_tv_limit );
    while ( timercmp( &l_tv_cur, &l_tv_limit, <= ) )
    {
        timersub( &l_tv_limit, &l_tv_cur, &l_tv_tmp );
        struct timespec l_tout = { l_tv_tmp.tv_sec, l_tv_tmp.tv_usec * 1000 }, l_res;    
        if ( nanosleep( & l_tout, &l_res ) < 0 )
        {
            if ( errno != EINTR )
                return -1;
        }
        else
        {
            break;
        }
        gettimeofday( &l_tv_cur, NULL );
    }

#else
    struct timespec req;
    req.tv_sec = t_sec;
    req.tv_nsec = t_nanosec;
    
    do {
        if (0 != nanosleep( & req, & req)) {
        if (errno != EINTR)
            return -1;
        } else {
            break;
        }
    } while (req.tv_sec > 0 || req.tv_nsec > 0);
#endif
    return 0; /* Return success */
}

