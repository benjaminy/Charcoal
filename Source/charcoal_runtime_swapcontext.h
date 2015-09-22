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
#define __CHARCOAL_IMPL_THREAD 1
#define __CHARCOAL_IMPL_SWAPCONTEXT 2
#define __CHARCOAL_IMPL_GCC_SPLIT_STACK 3
#define __CHARCOAL_IMPL_COROUTINE 4

#define __CHARCOAL_ACTIVITY_IMPL CRCL(IMPL_COROUTINE)

/* when activities are implemented with threads, a charcoal thread is
 * just a unique id */

typedef void (*crcl(entry_t))( void *formals );

/* Thread flags */
// #define CRCL(THDF_ANY_RUNNING) 1

typedef struct thread thread, *thread_p

struct thread
{
    /* atomic */ size_t tick;
    pthread_t sys;
    crcl(atomic_int) unyield_depth, keep_going;
    uv_timer_t timer_req;
    activity_p activities, ready;
    pthread_mutex_t thd_management_mtx;
    pthread_cond_t thd_management_cond;
    unsigned flags;
    unsigned runnable_activities;
    /* Linked list of all threads */
    crcl(thread_t) *next, *prev;
};


#define __CHARCOAL_GENERIC_INIT(locals_size) \
    do { \
        size_t ls = locals_size; \
        __charcoal_frame *f = (__charcoal_frame *)malloc( \
            sizeof( f[0] ) + ls ); \
        f->fn = crcl(YYY_yielding); \
        f->return_addr = NULL; \
        f->caller = caller; \
        f->activity = NULL; \
        if( caller ) \
        { \
            caller->callee = f; \
            f->activity = caller->activity; \
        } \
    } while( 0 ) \

/*
  return x ->

  self->locals = x;
  return self->caller;
*/

void crcl(stack_monster)( crcl(frame_p) the_frame );

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

void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a,
    crcl(thread_t) *t, activity_p *qp );
activity_p crcl(pop_special_queue)(
    unsigned queue_flag, crcl(thread_t) *t, activity_p *qp );

int crcl(activity_blocked)( activity_p self );

/* join thread t.  Return True if t was the last application
 * thread. */
int crcl(join_thread)( crcl(thread_t) *t );
int crcl(activate)( activity_p act, crcl(entry_t) f, void *args );

activity_p crcl(get_self_activity)( void );
void crcl(set_self_activity)( activity_p a );
int crcl(activity_join)( activity_p , void * );
int crcl(activity_detach)( activity_p );

#endif /* __CHARCOAL_RUNTIME */
