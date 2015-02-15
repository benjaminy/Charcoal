#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

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

/* This is a silly little helper for Cil */
extern int crcl(dummy_variable);

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

#define __CHARCOAL_COROUTINE_FN_PTR_SPECIFIC(name, ret_type, ...) \
    struct crcl(coroutine_fn_ptr_##name) \
    { \
        frame_p (*prologue)( frame_p caller, void *ret_ptr, ... ); \
        void (*after_return)( frame_p frame, ret_type *lhs ); \
        ret_type (*unyielding)( ... ); \
    };

int crcl(activate)( crcl(frame_p) caller, activity_p activity, crcl(frame_p) f );
crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame );

/* NOTE: It might make good performance sense to inline this, but I'm
 * going to leave that decision to the system for now. */
static frame_p crcl(fn_generic_prologue)(
    size_t sz, void *return_ptr, frame_p caller, frame_p (*fn)( frame_p ) )
{
    /* XXX make malloc/free configurable? */
    frame_p f = (frame_p)malloc( sz + sizeof( f[0] ) );
    if( !f )
    {
        /* XXX die */
    }
    f->activity     = NULL;
    f->fn           = fn;
    f->caller       = caller;
    f->callee       = NULL;
    f->goto_address = NULL;
    if( caller )
    {
        caller->callee = f;
        caller->goto_address = return_ptr;
        f->activity = caller->activity;
    }
    return f;
}

/* XXX Not sure if it's sensible to be generic about after-return */
static void crcl(fn_generic_after_return)( frame_p frame )
{
    /* XXX make malloc/free configurable? */
    free( frame->callee );
    /* NOTE Zeroing the callee field is not strictly necessary, and
     * therefore might be wasteful.  However, one should not be
     * nickel-and-diming the performance of yielding calls anyway (use
     * unyielding calls instead). */
    frame->callee = NULL;
}

/* XXX Not sure if it's sensible to be generic about epilogue */
static frame_p crcl(fn_generic_epilogue)( frame_p frame )
{
    return frame->caller;
}

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
