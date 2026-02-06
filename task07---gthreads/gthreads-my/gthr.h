#ifndef __GTHR_H
#define __GTHR_H

#include <stdint.h>
#include <time.h>

#ifndef GT_PREEMPTIVE 

#define GT_PREEMPTIVE 1

#endif

enum {
    MaxGThreads = 5,                // Maximum number of threads, used as array size for gttbl
    StackSize = 0x400000,           // Size of stack of each thread
    TimePeriod = 10,                // Time period of Timer (ms)
    MaxTaskName = 16,               // maximum length of task name
};


// Thread state
typedef enum {
    Unused,
    Running,
    Ready,
    Blocked,
    Suspended,
} gt_thread_state_t;

// --- thread handle (pointer to context) --------------------------------------
typedef struct gt_context_t * gt_handle_t;

struct gt_context_t {
    struct gt_regs {                // Saved context, switched by gt_*swtch.S (see for detail)
        uint64_t rsp;
        uint64_t rbp;
#if ( GT_PREEMPTIVE == 0 )
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
#endif
    }
    regs;

    gt_thread_state_t thread_state; // process state

    char name[MaxTaskName];         // task name
    uint64_t delay_until;           // delay until this time (ms since start)
};


void gt_init( void );                       // initialize gttbl
int  gt_go( void ( * t_run )( void ), const char *t_name, gt_handle_t *t_handle );  // create new thread with name
void gt_stop( void );                       // terminate current thread
int  gt_yield( void );                      // yield and switch to another thread
void gt_start_scheduler( void );            // start scheduler, wait for all tasks

// --- new functions (FreeRTOS-like) -------------------------------------------
void gt_suspend( gt_handle_t t_handle );    // suspend task (NULL = current task)
void gt_resume( gt_handle_t t_handle );     // resume suspended task
void gt_delay( uint32_t t_ms );             // delay current task for t_ms milliseconds
void gt_task_list( void );                  // print list of all tasks with their states

void gt_swtch( struct gt_regs * t_old, struct gt_regs * t_new );        // declaration from gtswtch.S
void gt_pree_swtch( struct gt_regs * t_old, struct gt_regs * t_new );   // declaration from gtswtch.S

int uninterruptibleNanoSleep( time_t sec, long nanosec );   // uninterruptible sleep

#endif // __GTHR_H
