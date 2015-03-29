/*
 * The Charcoal Runtime System
 */

#include <charcoal.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h> /* XXX remove dep eventually */
#include <stdio.h> /* XXX remove dep eventually */
#include <errno.h>
#include <limits.h>
#include <charcoal_runtime_coroutine.h>
#include <charcoal_runtime_io_commands.h>

/* Scheduler stuff */

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

cthread_p crcl(threads);
uv_key_t crcl(self_key);
// XXX static timer_t crcl(heartbeat_timer);
// XXX deprecated??? static int crcl(heartbeat_timer);

activity_p crcl(get_self_activity)( void )
{
    return (activity_p)uv_key_get( &crcl(self_key) );
}

#if 0
    deprecated?
static int crcl(start_stop_heartbeat)( int start )
{
    /* XXX I think 'its' can be stack alloc'ed.  Check this. */
    // struct itimerspec its;
    /* XXX Think about what the interval should be. */
    long long freq_nanosecs;
    if( start )
    {
        freq_nanosecs = 10*1000*1000;
    }
    else
    {
        freq_nanosecs = 0;
    }
    // its.it_value.tv_sec     = freq_nanosecs / 1000000000;
    // its.it_value.tv_nsec    = freq_nanosecs % 1000000000;
    // its.it_interval.tv_sec  = its.it_value.tv_sec;
    // its.it_interval.tv_nsec = its.it_value.tv_nsec;

    // RET_IF_ERROR( timer_settime( crcl(heartbeat_timer), 0, &its, NULL ) );
    return 0;
}
#endif

int crcl(choose_next_activity)( activity_p *p )
{
    activity_p to_run, first, self = crcl(get_self_activity)();
    cthread_p thd = self->thread;
    uv_mutex_lock( &thd->thd_management_mtx );
    first = to_run = thd->activities;
    do {
        if( !( to_run->flags & CRCL(ACTF_BLOCKED) )
            && ( to_run != self ) )
        {
            first = NULL;
            break;
        }
        to_run = to_run->next;
    } while( to_run != first );
    /* XXX What is this sv nonsense about??? */
    int rv = -1, sv = 0; // XXX crcl(sem_try_decr)( &self->can_run );
    if( !sv )
        ; //XXX crcl(sem_incr)( &self->can_run );

    if( first )
    {
        // XXX rv = crcl(sem_try_decr)( &to_run->can_run );
        if( !rv )
            ; // XXX crcl(sem_incr)( &to_run->can_run );
    }
    //commented this out for testing purposes, TODO: put back
    //printf( "SWITCH from: %p(%i)  to: %p(%i)\n",
    //        self, sv, to_run, rv );
    if( p )
    {
        *p = to_run;
    }
    uv_mutex_unlock( &thd->thd_management_mtx );
    return 0;
}

/* unyielding */ void crcl(unyielding_enter)( crcl(frame_p) frame )
{
    crcl(atomic_incr_int)( &frame->activity->thread->unyield_depth );
}

/* unyielding */ void crcl(unyielding_exit)( crcl(frame_p) frame )
{
    crcl(atomic_decr_int)( &frame->activity->thread->unyield_depth );
    /* TODO: Maybe call yield here?  Probably not.  unyielding -/> loop */
}

/* This should be called just before an activity starts or resumes from
 * yield/wait. */
void crcl(activity_start_resume)( activity_p activity )
{
    /* XXX: enqueue command */
    /* XXX: start heartbeat if runnable > 1 */
    ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    uv_key_set( &crcl(self_key), activity );
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) self->container->max_time);
    // crcl(atomic_store_int)(&self->container->timeout, 0);
    activity->yield_attempts = 0;
    /* XXX: Lots to fix here. */
}

typedef void (*crcl(switch_listener))(activity_p from, activity_p to, void *ctx);

void crcl(push_yield_switch_listener)( crcl(switch_listener) pre, void *pre_ctx, crcl(switch_listener) post, void *post_ctx )
{
}

void crcl(pop_yield_switch_listener)()
{
}

#if 0
    deprecated?
/* XXX must fix.  Still assuming activities implemented as threads */
static int crcl(yield_try_switch)( activity_p self )
{
    activity_p to;
    /* XXX error check */
    crcl(choose_next_activity)( &to );
    if( to )
    {
        // XXX crcl(sem_incr)( &to->can_run );
        // XXX crcl(sem_decr)( &self->can_run );
        crcl(activity_start_resume)( self );
    }
    return 0;
}
#endif

#if 0
    deprecated?
static void crcl(print_special_queue)( activity_p *q )
{
    assert( q );
    activity_p a = *q, first = a;
    if( a )
    {
        printf( "Special queue: " );
        do {
            printf( "%p  ", a );
            a = a->snext;
        } while( a != first );
        printf( "\n" );
    }
    else
    {
        printf( "Special queue empty\n" );
    }
}
#endif

/* Precondition: The thread mgmt mutex is held. */
activity_p crcl(pop_special_queue)(
    unsigned queue_flag, cthread_p t, activity_p *qp )
{
    // assert( t || qp );
    // assert( !( t && qp ) );
    activity_p *q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_BLOCKED):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-pop pre " ); */
    /* crcl(print_special_queue)( q ); */
    if( *q )
    {
        activity_p a = *q;
        if( a->snext == a )
        {
            *q = NULL;
        }
        else
        {
            a->snext->sprev = a->sprev;
            a->sprev->snext = a->snext;
            *q = a->snext;
        }
        a->flags &= ~queue_flag;
        return a;
    }
    else
    {
        return NULL;
    }
    /* printf( "S-pop post " ); */
    /* crcl(print_special_queue)( q ); */
}

activity_p crcl(pop_ready_queue)( cthread_p t )
{
    return crcl(pop_special_queue)( CRCL(ACTF_READY_QUEUE), t, NULL );
}

#if 0
    deprecated?
static activity_p crcl(pop_blocked_queue)( cthread_p t )
{
    return crcl(pop_special_queue)( CRCL(ACTF_BLOCKED), t, NULL ); /* XXX */
}
#endif

/* Precondition: The thread mgmt mutex is held. */
void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a, cthread_p t, activity_p *qp )
{
    assert( t || qp );
    assert( !( t && qp ) );
    if( a->flags & queue_flag )
    {
        /* printf( "Already in queue\n" ); */
        return;
    }
    a->flags |= queue_flag;
    activity_p *q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_BLOCKED):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-push pre q:%p %p ", q, a ); */
    /* crcl(print_special_queue)( q ); */
    if( *q )
    {
        activity_p front = *q, rear = front->sprev;
        /* printf( "Front: %p(n:%p p:%p)  Rear:%p(n:%p p:%p)\n", */
        /*         front, front->snext, front->sprev, */
        /*         rear , rear ->snext, rear ->sprev ); */
        rear->snext = a;
        front->sprev = a;
        a->snext = front;
        a->sprev = rear;
    }
    else
    {
        *q = a;
        a->snext = a;
        a->sprev = a;
    }
    /* printf( "S-push post q:%p %p ", q, a ); */
    /* crcl(print_special_queue)( q ); */
}

void crcl(push_ready_queue)( activity_p a, cthread_p t )
{
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), a, t, NULL );
}

void crcl(push_blocked_queue)( activity_p a, cthread_p t )
{
    crcl(push_special_queue)( CRCL(ACTF_BLOCKED), a, t, NULL ); /* XXX */
}
                                  
/* activity_blocked should be called when an activity has nothing to do
 * right now.  Another activity or thread might switch back to it later.
 * If another activity is ready, run it.  Otherwise wait for a condition
 * variable signal from another thread (typically the I/O thread). */
crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame )
{
    // XXX --thd->runnable_activities;
    /* printf( "Actvity blocked\n" ); */
    crcl(frame_p) rv       = NULL;
    activity_p    activity = frame->activity;
    cthread_p     thd      = activity->thread;
    uv_mutex_lock( &thd->thd_management_mtx );
    if( activity->flags & CRCL(ACTF_BLOCKED) )
    {
        activity->top = frame;
        if( thd->ready )
        {
            activity_p to = crcl(pop_ready_queue)( thd );
            crcl(activity_start_resume)( to );
            rv = to->top;
        }
    }
    else
    {
        rv = frame->caller;
    }
    uv_mutex_unlock( &thd->thd_management_mtx );
    return rv;
}

void crcl(activity_set_return_value)( activity_p a, void *ret_val_ptr )
{
    memcpy( a->return_value, ret_val_ptr, a->ret_size );
}

void crcl(activity_get_return_value)( activity_p a, void **ret_val_ptr )
{
    memcpy( ret_val_ptr, a->return_value, a->ret_size );
}

void crcl(switch_from_to)( activity_p from, activity_p to )
{
    /* XXX assert from is the currently running activity? */
    cthread_p thd = from->thread;
    assert( thd );
    uv_mutex_lock( &thd->thd_management_mtx );
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), from, thd, NULL );
    uv_mutex_unlock( &thd->thd_management_mtx );
#if 0
    if( _setjmp( from->jmp ) == 0 )
    {
        crcl(activity_start_resume)( to );
        _longjmp( to->jmp, 1 );
    }
#endif
    //printf("Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

void crcl(switch_to)( activity_p act )
{
    crcl(switch_from_to)( crcl(get_self_activity)(), act );
}

crcl(atomic_int) *crcl(yield_ticker);

/*
 * The current implementation strategy for yield is to put almost all of
 * the logic into the library code, as opposed to in generated code at
 * yield invocation sites.  A yield in Charcoal source should translate
 * to:
 *
 *     frame_p next_frame = yield_impl( frame, &after_yield_N );
 *     return next_frame;
 *     after_yield_N:
 *     ...
 *
 * In the common case that the current activity's quantum has not
 * expired, yield_impl will return the current frame, which will be
 * returned to the main loop.  Therefore the cost of a yield that keeps
 * going will be roughly:
 *  1 - Call yield_impl
 *  2 - Compute tick diff, branch (highly predictable)
 *  3 - Return (to yield site)
 *  4 - Return (to main loop)
 *  5 - Indirect call (to yield site function)
 *  6 - Indirect branch (to after yield site)
 * This will probably cost at least a couple dozen clock cycles, which
 * seems high.  However:
 * 1) In well-tuned Charcoal code, yield frequency should be in the
 *    microseconds to milliseconds range, so a couple dozen clock cycles
 *    is very small, relatively speaking.
 * 2) The worst case is not catastrophic.  We're only talking about
 *    slowing down CPU-bound code by a modest factor.
 * 3) This scheme will hopefully pollute processor resources like the
 *    branch predictor and instruction cache as little as possible.
 *    These factors can have a surprisingly large impact on
 *    whole-application performance
 */
crcl(frame_p) crcl(yield_impl)( crcl(frame_p) frame, void *ret_addr ){
    frame->goto_address = ret_addr;
    size_t     current_yield_tick = crcl(atomic_load_int)( crcl(yield_ticker) );
    activity_p activity           = frame->activity;
    ssize_t    diff               = activity->thread->tick - current_yield_tick;
    if( diff < 0 )
    {
        return frame;
    }
    /* "else": The current activity's quantum has expired. */
    activity_p self = crcl(get_self_activity)();

    /* XXX DEBUG foo->yield_attempts++; */
    int unyield_depth = crcl(atomic_load_int)( &(self->thread->unyield_depth) );
    if( unyield_depth == 0 )
    {
        cthread_p thd = self->thread;
        /* XXX handle locking errors? */
        uv_mutex_lock( &thd->thd_management_mtx );
        if( !thd->ready )
        {
            uv_mutex_unlock( &thd->thd_management_mtx );
            return NULL /* XXX */;
        }
        activity_p to = crcl(pop_special_queue)(
            CRCL(ACTF_READY_QUEUE), thd, NULL );
        uv_mutex_unlock( &thd->thd_management_mtx );
        crcl(switch_from_to)( self, to );
    }
    /* XXX */ return NULL;
}

/* XXX remove problem!!! */
int crcl(activity_join)( activity_p a, void *p )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & CRCL(ACTF_DONE) )
    {
        return 0;
    }
    activity_p self = crcl(get_self_activity)();
    // XXX crcl(push_blocked_queue)( CRCL(ACTF_BLOCKED), self, NULL, &a->joining );
    crcl(push_blocked_queue)( self, NULL );
    // XXX RET_IF_ERROR( crcl(activity_blocked)( self ) );
    return 0;
}

int crcl(activity_detach)( activity_p a )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & CRCL(ACTF_DETACHED) )
    {
        /* XXX error? */
    }
    a->flags |= CRCL(ACTF_DETACHED);
    return 0;
}

#if 0
    deprecated?
/* compiler inserts call to this fn at the end of each activity */
static crcl(frame_p) crcl(activity_finished)( crcl(frame_p) frame )
{
    /* TODO: the compiler generates the code to copy the activity
     * return value to activity memory. */
    activity_p activity = frame->activity;
    cthread_p thd = activity->thread;
    /* printf( "Finishing %p\n", a ); */
    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p next = crcl(pop_ready_queue)( thd );
    uv_mutex_unlock( &thd->thd_management_mtx );
    /* printf( "Jumping to %p\n", next ); */
    if( next )
    {
        crcl(activity_start_resume)( next );
    }
    else
    {
        return NULL;
    }
    /* XXX activity migration between threads will cause problems??? */

    /* LOG: Actual activity starting. */
    /* LOG: Actual activity complete. */
    return frame;
}
#endif

/* XXX Still have to think more about signal handling (and masking)??? */

static void insert_activity_into_thread( activity_p a, cthread_p t )
{
    /* printf( "Insert activity %p  %p\n", a, t ); */
    if( t->activities )
    {
        activity_p rear = t->activities->prev;
        /* printf( "f:%p  r:%p\n", t->activities, rear ); */
        rear->next = a;
        t->activities->prev = a;
        a->prev = rear;
        a->next = t->activities;
    }
    else
    {
        t->activities = a;
        a->prev = a;
        a->next = a;
    }
    crcl(push_ready_queue)( a, t );
    ++t->runnable_activities;
}

/* Initialize 'activity' and add it to 'thread'. */
/* (Do not start the new activity running) */
crcl(frame_p) activate_in_thread(
    cthread_p thread, activity_p activity, crcl(frame_p) frame )
{
    activity->thread          = thread;
    activity->flags           = 0;
    activity->yield_attempts  = 0;
    activity->top             = frame;
    activity->snext           = activity->sprev = NULL;
    activity->bottom.activity = activity;
    activity->bottom.fn       = 0; /* XXX clean up */
    activity->bottom.caller   = 0;
    activity->bottom.callee   = frame;
    activity->bottom.goto_address = 0;
    uv_mutex_lock( &thread->thd_management_mtx );
    insert_activity_into_thread( activity, thread );
    uv_mutex_unlock( &thread->thd_management_mtx );
    return 0; /* XXX TROUBLE */
}

/*
 * Assume: "activity" is allocated but uninitialized
 * Assume: "f" is the frame returned by the relevant init procedure
 */
crcl(frame_p) crcl(activate)(
    crcl(frame_p) caller, void *ret_addr, activity_p activity, crcl(frame_p) f )
{
    assert( !caller == !ret_addr );
    assert( activity );
    printf( "crcl(activate) %p %p\n", caller, caller ? caller->activity : 0 );
    if( caller )
    { /* Currently in yielding context */
        caller->goto_address = ret_addr;
        f->activity = activity;
        return activate_in_thread(
            caller->activity->thread, activity, f );
    }
    else
    { /* Currently in unyielding context */
        printf( "Activation in unyielding context unimplemented\n" );
        exit( 1 );
    }
}

int crcl(join_thread)( cthread_p t )
{
    printf( "join_thread %p n:%p p:%p ts:%p\n", t, t->next, t->prev, crcl(threads) );
    if( t == t->next )
    {
        /* XXX Trying to remove the last thread. */
        exit( 1 );
    }
    if( t == crcl(threads) )
    {
        /* XXX Trying to remove the I/O thread. */
        exit( 1 );
    }
    t->next->prev = t->prev;
    t->prev->next = t->next;
    assert( !uv_thread_join( &t->sys ) );
    return crcl(threads) == crcl(threads)->next;
}

#if 0
    deprecated?
static void crcl(report_thread_done)( activity_p a )
{
    /* printf( "XXX Thread is done!!!\n" ); */
}
#endif

/* NOTE: It might make good performance sense to inline all these
 * generic yielding call helper functions.  I'm going to leave that
 * decision to the system for now. */
/* NOTE: These generic helpers would be a convenient place to put in
 * debugging stuff. */

crcl(frame_p) crcl(fn_generic_prologue)(
    size_t sz, void *return_ptr, crcl(frame_p) caller, crcl(frame_p) (*fn)( crcl(frame_p) ) )
{
    /* XXX make malloc/free configurable? */
    crcl(frame_p) f = (crcl(frame_p))malloc( sz + sizeof( f[0] ) );
    if( !f )
    {
        /* XXX die */
    }
    f->activity     = NULL;
    f->fn           = fn;
    f->caller       = caller;
    f->callee       = NULL;
    f->goto_address = NULL;
    printf( "generic_prologue %p %p\n", caller, caller ? caller->activity : 0 );
    if( caller )
    {
        caller->callee = f;
        caller->goto_address = return_ptr;
        f->activity = caller->activity;
    }
    return f;
}

crcl(frame_p) crcl(fn_generic_epilogueA)( crcl(frame_p) frame )
{
    return frame->caller;
}

void crcl(fn_generic_epilogueB)( crcl(frame_p) frame )
{
    /* XXX make malloc/free configurable? */
    free( frame->callee );
    /* NOTE Zeroing the callee field is not strictly necessary, and
     * therefore might be wasteful.  However, one should not be
     * nickel-and-diming the performance of yielding calls anyway (use
     * unyielding calls instead). */
    frame->callee = NULL;
}
