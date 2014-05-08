#ifndef __CHARCOAL_RUNTIME
#define __CHARCOAL_RUNTIME

#include <unistd.h>
#include <uv.h>
#define _XOPEN_SOURCE
#undef _FORTIFY_SOURCE
#include <ucontext.h>
#include <setjmp.h>
#include <pthread.h>
#include <charcoal_semaphore.h>
#include <charcoal_runtime_io_commands.h>
#include <charcoal_runtime_atomics.h>

/* Exactly one of the following should be defined */
#define __CHARCOAL_ACTIVITY_IMPL_PTHREAD
#undef __CHARCOAL_ACTIVITY_IMPL_GCC_SPLIT_STACK
#undef __CHARCOAL_ACTIVITY_IMPL_GCC_SPLIT_STACK

/* when activities are implemented with threads, a charcoal thread is
 * just a unique id */

typedef void (*crcl(entry_t))( void *formals );

typedef struct crcl(thread_t) crcl(thread_t);
typedef struct activity_t activity_t;

/* Thread flags */
// #define CRCL(THDF_ANY_RUNNING) 1

struct crcl(thread_t)
{
    pthread_t sys;
    crcl(atomic_int) unyield_depth;
    crcl(atomic_int) timeout; //initially 0, signal handler sets to 1
    uv_timer_t timer_req;
    double start_time;
    double max_time;
    activity_t *activities, *ready, *to_be_reaped;
    pthread_mutex_t thd_management_mtx;
    pthread_cond_t thd_management_cond;
    unsigned flags;
    unsigned runnable_activities;
    /* Linked list of all threads */
    crcl(thread_t) *next, *prev;
};

/* XXX Need to refactor types some day */
typedef union crcl(io_response_t) crcl(io_response_t);

union crcl(io_response_t)
{
    struct
    {
        int rc;
        struct addrinfo *info;
    } addrinfo;
};

/* Activity flags */
#define __CHARCOAL_ACTF_DETACHED    (1 << 0)
#define __CHARCOAL_ACTF_BLOCKED     (1 << 1)
#define __CHARCOAL_ACTF_READY_QUEUE (1 << 2)
#define __CHARCOAL_ACTF_REAP_QUEUE  (1 << 3)
#define __CHARCOAL_ACTF_DONE        (1 << 4)

struct activity_t
{
    crcl(thread_t) *container;
    crcl(sem_t) can_run;
    unsigned flags;
    /* Every activity is is the (thread-local) list of all activities.
     * An activity may be in another "special" list, of which there
     * are currently two: Ready and To Be Reaped */
    activity_t *next, *prev, *snext, *sprev, *joining;
    crcl(io_response_t) io_response;
    ucontext_t ctx;
    jmp_buf    jmp;
    /* Debug and profiling stuff */
    int yield_attempts;
    /* Using the variable-sized last field of the struct hack */
    size_t ret_size;
    char return_value[ sizeof( int ) ];
};

void crcl(push_special_queue)(
    unsigned queue_flag, activity_t *a,
    crcl(thread_t) *t, activity_t **qp );
activity_t *crcl(pop_special_queue)(
    unsigned queue_flag, crcl(thread_t) *t, activity_t **qp );

int crcl(activity_blocked)( activity_t *self );

/* join thread t.  Return True if t was the last application
 * thread. */
int crcl(join_thread)( crcl(thread_t) *t );
int crcl(activate)( activity_t *act, crcl(entry_t) f, void *args );

activity_t *crcl(get_self_activity)( void );
void crcl(set_self_activity)( activity_t *a );
int crcl(activity_join)( activity_t *, void * );
int crcl(activity_detach)( activity_t * );


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
