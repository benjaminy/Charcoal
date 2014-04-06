#ifndef __CHARCOAL_RUNTIME
#define __CHARCOAL_RUNTIME

#include <unistd.h>
#include <uv.h>
#define _XOPEN_SOURCE
#include <ucontext.h>
#include <setjmp.h>
#include <pthread.h>
#include <charcoal_semaphore.h>
#include <charcoal_runtime_atomics.h>

/* Exactly one of the following should be defined */
#define __CHARCOAL_ACTIVITY_IMPL_PTHREAD
#undef __CHARCOAL_ACTIVITY_IMPL_GCC_SPLIT_STACK
#undef __CHARCOAL_ACTIVITY_IMPL_GCC_SPLIT_STACK

/* when activities are implemented with threads, a charcoal thread is
 * just a unique id */

typedef void (*CRCL(entry_t))( void *formals );

typedef struct CRCL(thread_t) CRCL(thread_t);
typedef struct CRCL(activity_t) CRCL(activity_t);

/* Thread flags */
// #define __CRCL_THDF_ANY_RUNNING 1

struct CRCL(thread_t)
{
    pthread_t self;
    CRCL(atomic_int) unyielding;
    CRCL(atomic_int) timeout; //initially 0, signal handler sets to 1
    uv_timer_t timer_req;
    double start_time;
    double max_time;
    CRCL(activity_t) *activities, *ready;
    pthread_mutex_t thd_management_mtx;
    pthread_cond_t thd_management_cond;
    unsigned flags;
    unsigned runnable_activities;
    /* Linked list of all threads */
    CRCL(thread_t) *next, *prev;
};

/* Activity flags */
#define __CRCL_ACTF_DETACHED 1
#define __CRCL_ACTF_BLOCKED 2

struct CRCL(activity_t)
{
    int yield_attempts;
    CRCL(thread_t) *container;
    CRCL(sem_t) can_run;
    double stupid_buffer[100];
    unsigned flags;
    /* Linked list of all activities in a given thread */
    CRCL(activity_t) *next, *prev, *ready_next, *ready_prev;
    ucontext_t ctx;
    jmp_buf    jmp; 
    /* Using the variable-sized last field of the struct hack */
    size_t ret_size;
    char return_value[ sizeof( int ) ];
};

int CRCL(activate)( CRCL(activity_t) *act, CRCL(entry_t) f, void *args );

CRCL(activity_t) *CRCL(get_self_activity)( void );
void CRCL(set_self_activity)( CRCL(activity_t) *a );
int CRCL(activity_join)( CRCL(activity_t) * );
int CRCL(activity_detach)( CRCL(activity_t) * );


/* Ignore everything from here to the end */


/*
 * TetraStak Concurrency Library
 */

typedef struct _TET_MACHINE _TET_MACHINE, *TET_MACHINE;
typedef struct _TET_PROCESS _TET_PROCESS, *TET_PROCESS;
typedef struct _TET_THREAD  _TET_THREAD,  *TET_THREAD;
typedef struct _TET_TASK    _TET_TASK,    *TET_TASK;
/* uni- and bi-directional channels.  All channels must be attached
 * to a task in order to be used. */
typedef struct _TET_CHANNEL _TET_CHANNEL, *TET_CHANNEL;
/* A channel port ("ch_port") is one end of a channel.  All sending
 * and receiving actually happens on ports, not channels. */
typedef struct _TET_CH_PORT _TET_CH_PORT, *TET_CH_PORT;
/* Channel types are an interesting area for further exploration */

typedef enum
{
    TETRC_SUCCESS,
} TET_RESULT_CODE;

TET_RESULT_CODE tet_spawn( void *(*f)(void *), void *param );
TET_RESULT_CODE tet_spawn_in_thread(
    void *(*f)(void *), void *param, TET_THREAD thread );
TET_RESULT_CODE tet_spawn_in_process(
    void *(*f)(void *), void *param, TET_PROCESS process );
TET_RESULT_CODE tet_spawn_in_machine(
    void *(*f)(void *), void *param, TET_MACHINE machine );

#if 0
/* coroutine and channel stuff not implemented yet */

TET_RESULT_CODE tet_coroutine_init(
    void *(*f)(void *),
    void *param,
    TET_BCHANNEL channel );

TET_RESULT_CODE tet_coroutine_call(
    TET_BCHANNEL channel, ... );

TET_RESULT_CODE tet_coroutine_call_async(
    TET_BCHANNEL channel, TET_EVENT event, ... );

/* The simplest send receive primitives. */
TET_RESULT_CODE tet_send( TET_CH_PORT port, void *data, size_t sz );
TET_RESULT_CODE tet_recv( TET_CH_PORT port, void *data, size_t *sz );
TET_RESULT_CODE tet_send_fancy(
    TET_CH_PORT port, ... );
TET_RESULT_CODE tet_recv_fancy(
    TET_CH_PORT port, ... );



TET_RESULT_CODE tet_yield( void );
TET_RESULT_CODE tet_yield_value( void *val );
TET_RESULT_CODE tet_yield_value_to_chan( void *val, TET_CHANNEL channel );
TET_RESULT_CODE tet_set_coroutine_channel(
    TET_CHANNEL channel, TET_CHANNEL *prev_channel );

/* task, thread, process, machine? */
TET_RESULT_CODE tet_async_call( void *(*f)( void *), promise? );

/* synchronization primitives */

/* While executing inside a task atomic block, a task will never
 * switch control to another task.  This provides no synchronization
 * with respect to other threads, processes or machines. */
TET_RESULT_CODE tet_task_atomic_enter();
TET_RESULT_CODE tet_task_atomic_leave();
#define TET_TASK_ATOMIC( STMT ) \
    do { \
        tet_task_atomic_enter(); \
        STMT; \
        tet_task_atomic_leave(); \
    } while( 0 )

typedef struct _TET_MUTEX_MACHINE _TET_MUTEX_MACHINE, *TET_MUTEX_MACHINE;
typedef struct _TET_MUTEX_PROCESS _TET_MUTEX_PROCESS, *TET_MUTEX_PROCESS;
typedef struct _TET_MUTEX_THREAD  _TET_MUTEX_THREAD,  *TET_MUTEX_THREAD;
typedef struct _TET_MUTEX_TASK    _TET_MUTEX_TASK,    *TET_MUTEX_TASK;

typedef struct
{
    channel
} semaphore;

#endif

#endif /* __CHARCOAL_RUNTIME */
