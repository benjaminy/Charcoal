#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

typedef struct crcl(frame) crcl(frame), *crcl(frame_p);

struct crcl(frame)
{
    activity_p self;
    crcl(frame_p) (*fn)( crcl(frame_p) );
    crcl(frame_p) caller, callee;
    void *goto_address;
    char locals[0]; /* ... and return value? */
};

typedef struct crcl(coroutine_fn_ptr_generic) crcl(coroutine_fn_ptr_generic),
    *crcl(coroutine_fn_ptr_generic_p);

struct crcl(coroutine_fn_ptr_generic)
{
    frame_p (*init)( frame_p caller, ... );
    frame_p (*yielding)( frame_p self );
    void * (*unyielding)( ... );
};

#define __CHARCOAL_COROUTINE_FN_PTR_SPECIFIC(name, ...) \
    struct crcl(coroutine_fn_ptr_##name) \
    { \
        frame_p (*init)( frame_p caller, ... ); \
        frame_p (*yielding)( frame_p self ); \
        void * (*unyielding)( ... ); \
    };

struct activity_t
{
    thread_p container;
    crcl(frame) yield_frame;
    crcl(sem_t) can_run;
    unsigned flags;
    /* Every activity is in the (thread-local) list of all activities.
     * An activity may be in another "special" list, of which there
     * are currently two: Ready and Blocked */
    activity_p next, prev, snext, sprev;
    crcl(io_response_t) io_response;
    /* NOTE: While an activity is running "top" might be stale.  It
     * gets updated when an activity switches for sure. */
    crcl(frame_p) bottom, top;
    /* Debug and profiling stuff */
    int yield_attempts;
    /* Using the variable-sized last field of the struct hack */
    size_t ret_size;
    char return_value[ sizeof( int ) ];
};

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
