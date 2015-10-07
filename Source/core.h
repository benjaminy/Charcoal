#ifndef __CHARCOAL_CORE
#define __CHARCOAL_CORE

/* Should be included before absolutely anything else in Charcoal
 * runtime source files.  Charcoal application compilation flows should
 * implicitly include this file. */

#ifdef __CHARCOAL_CIL
/* Example: #pragma cilnoremove("func1", "var2", "type foo", "struct bar") */
#pragma cilnoremove( "struct __charcoal_frame_t" )
#pragma cilnoremove( "struct activity_t" )
#pragma cilnoremove( "MYSTERIOUS_CIL_BUG" )
#pragma cilnoremove( "__charcoal_activate" )
#pragma cilnoremove( "__charcoal_yield" )
#pragma cilnoremove( "__charcoal_yield_impl" )
#pragma cilnoremove( "__charcoal_activity_waiting_or_done" )
#pragma cilnoremove( "__charcoal_add_to_waiters" )
#endif

#include <uv.h>

#define crcl(n) __charcoal_ ## n
#define CRCL(n) __CHARCOAL_ ## n

#define RET_IF_ERROR(cmd) \
    do { int rc = cmd; if( 0 > rc ) return rc; if( 0 < rc ) return -rc; } while( 0 )

#define __CHARCOAL_RET_IF_ERROR(cmd) \
    do { int rc; if( 0 > ( rc = cmd ) ) { return rc; } } while( 0 )

/*
 * I'd really like to use C11 atomics, but cil doesn't seem to support
 * them.  So Here are some definitons that should be identical to those
 * in OpenPA.
 */

typedef struct { volatile int v; } atomic_int;

#define __CHARCOAL_SET_FLAG(x,f)   do { (x).flags |=  (f); } while( 0 )
#define __CHARCOAL_CLEAR_FLAG(x,f) do { (x).flags &= ~(f); } while( 0 )
#define __CHARCOAL_CHECK_FLAG(x,f) ( !!( (x).flags & (f) ) )

/* Thread flags */
#define __CHARCOAL_THDF_IDLE       (1 << 0)
#define __CHARCOAL_THDF_KEEP_ALIVE (1 << 1) /* even after last activity finishes */
#define __CHARCOAL_THDF_NEVER_RUN  (1 << 2)
#define __CHARCOAL_THDF_TIMER_ON   (1 << 3)

/* Activity flags */
#define __CHARCOAL_ACTF_DETACHED  (1 << 0)
#define __CHARCOAL_ACTF_WAITING   (1 << 1)
#define __CHARCOAL_ACTF_READY     (1 << 2)
#define __CHARCOAL_ACTF_DONE      (1 << 3)
#define __CHARCOAL_ACTF_OOM       (1 << 4)

/* XXX super annoying name collision on thread_t with Mach header.
 * Look into it more some day. */
typedef struct       cthread_t        cthread_t,        *cthread_p;
typedef struct      activity_t       activity_t,       *activity_p;
typedef struct    crcl(frame_t)    crcl(frame_t),    *crcl(frame_p);
typedef struct crcl(act_list_t) crcl(act_list_t), *crcl(act_list_p);

/* The size of a frame is currently 5 pointers (20/40 bytes) plus the
 * procedure-specific data. */
struct crcl(frame_t)
{
    /* The activity this frame belongs to */
    activity_p activity;

    /* The code for this frame */
    crcl(frame_p) (*fn)( crcl(frame_p) );

    /* The address to resume execution in the code.  If NULL, start at
     * the beginning */
    void *return_addr;

    /* Doubly linked call chain.  The callee link could probably be
     * optimized away, but it's not trivial and seems nice for debugging
     * anyway. */
    crcl(frame_p) caller, callee;

    /* Using the variable-sized last field trick.
     * The last field is for procedure-specific storage (parameters,
     * local variables, and the return value).
     * union {
     *     struct {
     *         params and local vars
     *     } L;
     *     return type R;
     * } */
    char specific[0];
/* TODO
    union {
        struct {
            char p[0];
        } L;
        char R[0];
    } specific;
*/
};

struct crcl(act_list_t) {
    activity_p next, prev;
};

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
struct activity_t
{
    /* The thread this activity belongs to */
    cthread_p thread;

    /* TODO: add a bit to indicate if a yield caused an activity switch */
    /* Various status bits */
    unsigned flags;

    /* Two doubly linked lists.
     *  0 - Either the ready queue or the queue of waiters for a particular event
     *  1 - The list of waiting activities that belong to a particular thread */
    crcl(act_list_t) qs[2];

    /* The queue that this activity is in (if it's in a queue) */
    activity_p *waiting_queue;

    /* A list of activities that are waiting for this one to finish. */
    /* TODO: Might replace these with a more generic event
     * thing-a-ma-jig */
    activity_p waiters;

    /* The oldest and newest frames in this activity's call chain.
     * NOTE: While an activity is running this might be stale.  It gets
     * updated when an activity switches for sure. */
    crcl(frame_p) oldest_frame, newest_frame;

    /* Debug and profiling stuff */
    int yield_calls;
};

struct cthread_t
{
    /* Used??? Should be atomic??? */
    size_t        tick;

    /* Various status bits */
    unsigned      flags;

    /* The system-specific thread object */
    uv_thread_t   sys;

    /* The indicator that the currently-running activity should yield
     * ASAP. */
    atomic_int    interrupt_activity;

    /* For execution quantum expiration */
    uv_timer_t    timer_req;

    /* waiting and ready are references to lists of activities; running
     * is the single activity that is running. Any can be NULL. */
    activity_p    waiting, ready, running;

    /* Thread management mutex and condition variable */
    uv_mutex_t    thd_management_mtx;
    uv_cond_t     thd_management_cond;

    /* The idle activity and its frame (for when all activities are
     * waiting) */
    activity_t    idle_act;
    crcl(frame_t) idle_frm;

    /* How many activities are waiting on something; If non-zero, don't quit */
    atomic_int    waiting_activities;

    /* Linked list of all threads */
    cthread_p     next, prev;
};

#define assert_impl(a,b) assert( ( !( a ) ) || ( b ) )

int thread_start( cthread_p thd, void *options );

int MYSTERIOUS_CIL_BUG( void );

crcl(frame_p) crcl(activate)(
    crcl(frame_p) f,
    void *p,
    activity_p a,
    crcl(frame_p) f2 );
int crcl(yield)( void );
crcl(frame_p) crcl(yield_impl)( crcl(frame_p) frame, void *ret_addr );
crcl(frame_p) crcl(activity_waiting_or_done)( crcl(frame_p) frm, void *ret_addr );
void crcl(add_to_waiters)( activity_p waiter, activity_p *q );

#endif /* __CHARCOAL_CORE */
