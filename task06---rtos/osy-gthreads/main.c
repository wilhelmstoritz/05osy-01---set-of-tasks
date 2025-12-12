//***************************************************************************
//
// GThreads - preemptive threads in userspace. 
// Inspired by https://c9x.me/articles/gthreads/code0.html.
//
// Program created for subject OSMZ and OSY. 
//
// Michal Krumnikl, Dept. of Computer Sciente, michal.krumnikl@vsb.cz 2019
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// -----------------------------------------------------------------------------
// extended with FreeRTOS-like functions:
// gt_suspend, gt_resume, gt_delay, gt_task_list
// -----------------------------------------------------------------------------
//
//***************************************************************************

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

// === example 1: FreeRTOS-like example with three tasks (Hello, Sleep, Ring) ==
gt_handle_t g_sleep_task_handle = NULL;

// --- task "Hello" - periodically prints message ------------------------------
void task_hello( void ) {
    while ( 1 ) {
        printf( "Hello world!\n" );
        gt_delay( 1000 );  // delay 1000ms
    }
}

// --- task "Ring" - wakes up Sleep task periodically --------------------------
void task_ring( void ) {
    while ( 1 ) {
        gt_delay( 5000 );  // delay 5000ms
        printf( "Ring! Ring! Ring!\n" );
        gt_resume( g_sleep_task_handle );
    }
}

// --- task "Sleep" - suspends itself and waits to be resumed ------------------
void task_sleep( void ) {
    while ( 1 ) {
        gt_suspend( NULL );  // suspend self
        printf( "Please do not wake up!\n" );
    }
}

// === example 2: original test with counting threads ==========================
#define COUNT 100

// Dummy function to simulate some thread work
void f( void ) {
    static int x;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "F Thread id: %d count: %d\n", id, count );
        uninterruptibleNanoSleep( 0, 1000000 );

#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
}

// Dummy function to simulate some thread work
void g( void ) {
    static int x = 0;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "G Thread id: %d count: %d\n", id, count );
        uninterruptibleNanoSleep( 0, 1000000 );

#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
}

// === example 3: custom test for new functions ================================
gt_handle_t g_worker_handle = NULL;

void task_worker( void ) {
    int count = 0;
    while ( 1 ) {
        printf( "[worker] working... count=%d\n", count++ );
        gt_delay( 500 );  // work for 500ms intervals
    }
}

void task_controller( void ) {
    printf( "[controller] starting, will control worker task\n" );
    
    gt_delay( 2000 );  // let worker run for 2 seconds
    
    printf( "[controller] suspending worker...\n" );
    gt_suspend( g_worker_handle );
    gt_task_list();  // show task states
    
    gt_delay( 3000 );  // worker suspended for 3 seconds
    
    printf( "[controller] resuming worker...\n" );
    gt_resume( g_worker_handle );
    gt_task_list();  // show task states
    
    gt_delay( 2000 );  // let worker run for 2 more seconds
    
    printf( "[controller] test complete!\n" );
}


// === main - select which example to run ======================================
int main( int argc, char **argv ) {
    int example = 1;  // default to FreeRTOS-like example
    
    if ( argc > 1 )
        example = atoi( argv[1] );
    
    gt_init();      // initialize threads
    
    switch ( example ) {
    case 1:
        printf( "=== Running FreeRTOS-like example (Hello, Sleep, Ring) ===\n\n" );
        gt_go( task_hello, "Hello", NULL );
        gt_go( task_sleep, "Sleep", &g_sleep_task_handle );
        gt_go( task_ring, "Ring", NULL );
        break;
        
    case 2:
        printf( "=== Running original counting threads example ===\n\n" );
        gt_go( f, "F-thread1", NULL );
        gt_go( f, "F-thread2", NULL );
        gt_go( g, "G-thread1", NULL );
        gt_go( g, "G-thread2", NULL );
        break;
        
    case 3:
        printf( "=== Running custom suspend/resume test ===\n\n" );
        gt_go( task_worker, "worker", &g_worker_handle );
        gt_go( task_controller, "controller", NULL );
        break;
        
    default:
        printf( "usage: %s [1|2|3]\n", argv[0] );
        printf( "  1 - FreeRTOS-like example (Hello, Sleep, Ring)\n" );
        printf( "  2 - original counting threads\n" );
        printf( "  3 - custom suspend/resume test\n" );

        return 1;
    }
    
    gt_task_list();  // print initial task list
    
    gt_start_scheduler();  // wait until all threads terminate
    
    printf( "\nThreads finished\n" );
    return 0;
}
