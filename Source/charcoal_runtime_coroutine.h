#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

#include <uv.h>
#include <charcoal_runtime_common.h>
#include <charcoal_runtime_atomics.h>
#include <charcoal_semaphore.h>

typedef struct crcl(frame_t) crcl(frame_t), *crcl(frame_p);

struct cthread_t
{
    /* atomic */ size_t tick;
    uv_thread_t sys;
    crcl(atomic_int) unyield_depth, keep_going;
    uv_timer_t timer_req;
    activity_p activities, ready;
    uv_mutex_t thd_management_mtx;
    uv_cond_t thd_management_cond;
    unsigned flags;
    unsigned runnable_activities;
    /* Linked list of all threads */
    cthread_p next, prev;
};

struct crcl(frame_t)
{
    activity_p activity;
    crcl(frame_p) (*fn)( crcl(frame_p) );
    crcl(frame_p) caller, callee;
    void *goto_address;
    char locals[0]; /* ... and return value? */
};

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
struct activity_t
{
    cthread_p thread;
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

typedef struct crcl(coroutine_fn_ptr_generic) crcl(coroutine_fn_ptr_generic),
    *crcl(coroutine_fn_ptr_generic_p);

struct crcl(coroutine_fn_ptr_generic)
{
    crcl(frame_p) (*init)( crcl(frame_p) caller, ... );
    crcl(frame_p) (*yielding)( crcl(frame_p) self );
    void * (*unyielding)( int dummy, ... );
};

#define __CHARCOAL_COROUTINE_FN_PTR_SPECIFIC(name, ...) \
    struct crcl(coroutine_fn_ptr_##name) \
    { \
        frame_p (*init)( frame_p caller, ... ); \
        frame_p (*yielding)( frame_p self ); \
        void * (*unyielding)( ... ); \
    };


#define __CHARCOAL_GENERIC_INIT(locals_size) \
    do { \
        size_t ls = locals_size; \
        __charcoal_frame *f = (__charcoal_frame *)malloc( \
            sizeof( f[0] ) + ls ); \
        f->fn = crcl(YYY_yielding); \
        f->goto_address = NULL; \
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

int crcl(activate)( crcl(frame_p) caller, activity_p activity, crcl(frame_p) f );
crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame );

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
