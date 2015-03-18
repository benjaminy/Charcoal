#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

#ifdef __CHARCOAL_CIL
/* #pragma cilnoremove("func1", "var2", "type foo", "struct bar") */
#pragma cilnoremove( "struct __charcoal_frame_t" )
#pragma cilnoremove( "__charcoal_fn_generic_prologue" )
#pragma cilnoremove( "__charcoal_fn_generic_epilogueA" )
#pragma cilnoremove( "__charcoal_fn_generic_epilogueB" )
#endif

#include <charcoal_runtime_common.h>
#include <charcoal_runtime_atomics.h>
#include <charcoal_semaphore.h>
#include <stdlib.h>

typedef struct crcl(frame_t) crcl(frame_t), *crcl(frame_p);

struct cthread_t
{
    /* atomic */ size_t tick;
    unsigned            flags;
    uv_thread_t         sys;
    crcl(atomic_int)    unyield_depth, keep_going;
    uv_timer_t          timer_req;
    activity_p          activities, ready;
    uv_mutex_t          thd_management_mtx;
    uv_cond_t           thd_management_cond;
    unsigned            runnable_activities;
    /* Linked list of all threads */
    cthread_p           next, prev;
};

struct crcl(frame_t)
{
    activity_p activity;
    crcl(frame_p) (*fn)( crcl(frame_p) );
    /* NOTE: It's probably not necessary to have the callee link, but it
     * seems nice for debugging. */
    crcl(frame_p) caller, callee;
    void *goto_address;

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
};

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
struct activity_t
{
    cthread_p thread;
    /* TODO: add a bit to indicate if a yield caused an activity switch */
    unsigned flags;
    /* Every activity is in the (thread-local) list of all activities.
     * An activity may be in another "special" list, of which there
     * are currently two: Ready and Blocked */
    activity_p next, prev, snext, sprev;
    crcl(io_response_t) io_response;
    /* NOTE: While an activity is running "top" might be stale.  It
     * gets updated when an activity switches for sure. */
    /* "bottom" probably isn't necessary.  It might be handy for
     * debugging, but I'm not sure about that. */
    crcl(frame_p) bottom, top;
    /* XXX So hard to decide about yield and the predictable branch */
    crcl(frame_t) yield_frame;
    // crcl(sem_t) can_run;

    /* Debug and profiling stuff */
    int yield_attempts;

    /* Using the variable-sized last field of the struct hack */
    size_t ret_size;
    char return_value[ sizeof( int ) ];
};

void crcl(activity_start_resume)( activity_p activity );
int activate_in_thread(
    cthread_p thread, activity_p activity, crcl(frame_p) frame );
int crcl(activate)( crcl(frame_p) caller, activity_p activity, crcl(frame_p) f );
crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame );

crcl(frame_p) crcl(fn_generic_prologue)(
    size_t sz, void *return_ptr, crcl(frame_p) caller,
    crcl(frame_p) (*fn)( crcl(frame_p) ) );

crcl(frame_p) crcl(fn_generic_epilogueA)( crcl(frame_p) frame );

void crcl(fn_generic_epilogueB)( crcl(frame_p) frame );

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
