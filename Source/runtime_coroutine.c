/*
 * The Charcoal Runtime System
 */

#include <core.h>
#include <core_runtime.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <runtime_coroutine.h>
#include <runtime_io_commands.h>
#include <atomics_wrappers.h>

/* Scheduler stuff */

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

uv_key_t crcl(self_key);

activity_p crcl(get_self_activity)( void )
{
    return (activity_p)uv_key_get( &crcl(self_key) );
}

/* This should be called just before an activity starts or resumes from
 * yield/wait.
 *
 * Precondition: The thread mgmt lock is held.
 */
crcl(frame_p) crcl(activity_start_resume)( activity_p act )
{
    cthread_p thd = act->thread;
    /* XXX: enqueue command */
    /* XXX: start heartbeat if runnable > 1 */
    // HUH??? ABORT_ON_FAIL( uv_async_send( &crcl(async_call) ) );
    uv_key_set( &crcl(self_key), act );
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) self->container->max_time);
    // atomic_store_int(&self->container->timeout, 0);
    act->yield_calls = 0;
    /* XXX Races with other interruptions coming in!!! */
    atomic_store_int( &thd->interrupt_activity, 0 );
    thd->running = act;
    // zlog_debug( crcl(c) , "Activity start: %p", act );
    /* XXX: Lots to fix here. */
    if( thd->ready && !CRCL(CHECK_FLAG)( *thd, CRCL(THDF_TIMER_ON) ) )
    {
        CRCL(SET_FLAG)( *thd, CRCL(THDF_TIMER_ON) );
        /* XXX pre-alloc somewhere else? in the activity struct? */
        crcl(async_call_p) async = &thd->timer_call;
        async->f = crcl(async_fn_start);
        async->data = (void *)thd;
        // zlog_debug( crcl(c) , "Send timer req cmd: %p\n", cmd );
        enqueue( async );
        ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    }

    return act->newest_frame;
}

#if 0
    deprecated?
static void crcl(print_activity_queue)( activity_p *q )
{
    assert( q );
    activity_p a = *q, first = a;
    if( a )
    {
        zlog_info( crcl(c), "Activity queue: " );
        do {
            zlog_info( crcl(c), "%p  ", a );
            a = a->snext;
        } while( a != first );
        zlog_info( crcl(c), "\n" );
    }
    else
    {
        zlog_info( crcl(c), "Activity queue empty\n" );
    }
}
#endif

/* precondition: act is in q */
static void remove_activity_from_queue( activity_p act, unsigned flag )
{
    assert( act );
    int ready   = !!( flag & CRCL(ACTF_READY) );
    int waiting = !!( flag & CRCL(ACTF_WAITING) );
    assert( !( ready && waiting ) );

    activity_p *q;
    int q_idx = 0;
    if( ready )
    {
        q = &act->thread->ready;
        CRCL(CLEAR_FLAG)( *act, CRCL(ACTF_READY) );
    }
    else if( waiting )
    {
        q = act->waiting_queue;
        act->waiting_queue = NULL;
        CRCL(CLEAR_FLAG)( *act, CRCL(ACTF_WAITING) );
        /* XXX notify whatever this activity was waiting for??? */
    }
    else
    {
        cthread_p thd = act->thread;
        q = &thd->waiting;
        assert( atomic_load_int( &thd->waiting_activities ) > 0 );
        atomic_decr_int( &thd->waiting_activities );
        q_idx = 1;
    }

    assert( q );
    assert( *q );
    crcl(act_list_p) acts = &act->qs[ q_idx ];

    if( act == acts->next )
    {
        *q = NULL;
    }
    else
    {
        acts->next->qs[q_idx].prev = acts->prev;
        acts->prev->qs[q_idx].next = acts->next;
    }
    if( *q == act )
    {
        *q = acts->next;
    }
    acts->next = NULL;
    acts->prev = NULL;
}

/* Precondition: The thread mgmt mutex is held, if necessary. */
static activity_p pop_activity( activity_p *q, unsigned flag )
{
    int ready   = !!( flag & CRCL(ACTF_READY) );
    int waiting = !!( flag & CRCL(ACTF_WAITING) );
    assert( ready || waiting );
    assert( !( ready && waiting ) );
    assert( q );
    activity_p act = NULL;
    if( ( act = *q ) )
    {
        if( waiting )
        {
            assert( q == act->waiting_queue );
            act->waiting_queue = NULL;
        }
        crcl(act_list_p) acts = &act->qs[ 0 ];
        if( act == acts->next )
        {
            *q = NULL;
        }
        else
        {
            acts->next->qs[0].prev = acts->prev;
            acts->prev->qs[0].next = acts->next;
            *q = acts->next;
        }
        acts->next = NULL;
        acts->prev = NULL;
        CRCL(CLEAR_FLAG)( *act, flag );
    }
    return act;
}

/* Precondition: The thread mgmt mutex is held. */
activity_p crcl(pop_ready_queue)( cthread_p thd )
{
    assert( thd );
    activity_p act = pop_activity( &thd->ready,  CRCL(ACTF_READY) );
    // thd->running = act;
    // zlog_debug( crcl(c), "POP READY a:%p f:%x thd:%p q:%p",
    //             act, act ? act->flags : 0xFFFFFFFF, thd, &thd->ready );
    return act;
}

activity_p crcl(pop_waiting_queue)( activity_p *q )
{
    activity_p act = pop_activity( q, CRCL(ACTF_WAITING) );
    if( act )
    {
        remove_activity_from_queue( act, 0 );
    }
    // zlog_debug( crcl(c), "POP WAITING a:%p q:%p *q:%p", act, q, *q );
    return act;
}

/* Precondition: The thread mgmt mutex is held, if necessary. */
static void push_activity(
    activity_p act, activity_p *q, unsigned flag )
{
    int ready   = !!( flag & CRCL(ACTF_READY) );
    int waiting = !!( flag & CRCL(ACTF_WAITING) );
    assert( !( ready && waiting ) );
    assert( act );
    if( q )
    {
        assert( waiting );
        act->waiting_queue = q;
    }
    else
    {
        cthread_p thd = act->thread;
        q = ready ? &thd->ready : &thd->waiting;
    }
    if( flag )
    {
        assert( !CRCL(CHECK_FLAG)( *act, flag ) );
        CRCL(SET_FLAG)( *act, flag );
    }
    else
    {
        atomic_incr_int( &act->thread->waiting_activities );
    }
    int q_idx = ready || waiting ? 0 : 1;
    crcl(act_list_p) acts = &act->qs[ q_idx ];
    if( *q )
    {
        activity_p front = *q;
        crcl(act_list_p) fronts = &front->qs[ q_idx ];
        activity_p rear = fronts->prev;

        rear->qs[q_idx].next = act;
        fronts->prev = act;
        acts->next = front;
        acts->prev = rear;
    }
    else
    {
        *q = act;
        acts->next = act;
        acts->prev = act;
    }
}

/* Precondition: The thread mgmt mutex is held. */
void crcl(push_ready_queue)( activity_p act )
{
    // zlog_debug( crcl(c), "PUSH READY a:%p thd:%p q:%p", act, thd, &thd->ready );
    assert( act );
    push_activity( act, NULL, CRCL(ACTF_READY) );
    /* XXX maybe start the timer here */
}

void crcl(push_waiting_queue)( activity_p act, activity_p *q )
{
    // zlog_debug( crcl(c), "PUSH WAITING a:%p q:%p *q:%p", act, q, *q );
    assert( act );
    assert( q );
    push_activity( act, q, CRCL(ACTF_WAITING) );
    push_activity( act, NULL, 0 );
}

static crcl(frame_p) switch_from_to( activity_p from, activity_p to )
{
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_DONE) ) );
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_READY) ) );
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_WAITING) ) );
    cthread_p thd = from->thread;
    assert( thd );
    assert( thd == to->thread );
    uv_mutex_lock( &thd->thd_management_mtx );
    // zlog_debug( crcl(c) , "Switch thd: %p  from: %p  to: %p  ready: %p", thd,
    //            from, to, thd->ready );
    assert( from == thd->running );
    crcl(push_ready_queue)( from );
    crcl(frame_p) next_frame = crcl(activity_start_resume)( to );
    uv_mutex_unlock( &thd->thd_management_mtx );
    //zlog_debug( crcl(c), "Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
    return next_frame;
}

crcl(frame_p) crcl(switch_to)( activity_p act )
{
    return switch_from_to( crcl(get_self_activity)(), act );
}

/*
 * The current implementation strategy for yield is to put almost all of
 * the logic into the library code, as opposed to in generated code at
 * yield invocation sites.  A yield in Charcoal source should translate
 * to:
 *
 *     return yield_impl( frame, &after_yield_N );
 *     after_yield_N:
 *     ...
 *
 * In the common case that the current activity should not be
 * interrupted, yield_impl will return the frame that is passed in,
 * which will be returned to the main loop.  Therefore the cost of a
 * yield that keeps going will be roughly:
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
crcl(frame_p) crcl(yield_impl)( crcl(frame_p) frm, void *ret_addr ){
    activity_p act = frm->activity;
    cthread_p  thd = act->thread;
    int         *p = (int *)&frm->callee;

    frm->return_addr = ret_addr;
    ++act->yield_calls;
    int interrupt = atomic_load_int( &thd->interrupt_activity );

    *p = 0;
    if( !interrupt )
    {
        return frm;
    }
    /* "else": The current activity should be interrupted for some
     * reason.  More smarts should go here eventually. */
    *p = 1;
    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p next = crcl(pop_ready_queue)( thd );
    uv_mutex_unlock( &thd->thd_management_mtx );
    // zlog_debug( crcl(c) , "YIELD y:%p n:%p", act, next );
    /* XXX Something is wonky with the interrupt thing. Look into it. */
    if( next )
        return switch_from_to( act, next );
    else
    {
        atomic_store_int( &thd->interrupt_activity, 0 );
        return frm;
    }
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

/* XXX Still have to think more about signal handling (and masking)??? */

/*
 * activity_waiting_or_done should be called when an activity has
 * nothing to do right now.  Another activity or thread might switch
 * back to it later.  There are a few cases to consider to decide what
 * happens next:
 * 1) If there is a ready activity, switch to one
 * 2) Otherwise, if there are waiting activities switch to idle
 * 3) Otherwise, clean up thread
 */
crcl(frame_p) crcl(activity_waiting_or_done)( crcl(frame_p) frm, void *ret_addr )
{
    assert( frm );
    assert( frm->activity );
    frm->return_addr = ret_addr;
    activity_p act = frm->activity;
    cthread_p  thd = act->thread;
    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p next = crcl(pop_ready_queue)( thd );
    int num_waiting = atomic_load_int( &thd->waiting_activities );
    // zlog_debug( crcl(c) , "Activity waiting/done waiters:%d n:%p",
    //             num_waiting, next );
    if( next )
    {
        /* Hooray! */
    }
    else if( num_waiting > 0 /* TODO: || keep thread alive */ )
    {
        next = &thd->idle_act;
    }
    else
    {
        /* Clean up thread? */
    }
    crcl(frame_p) rv = NULL;
    if( next )
        rv = crcl(activity_start_resume)( next );
    uv_mutex_unlock( &thd->thd_management_mtx );
    return rv;
}

static void clean_up_activity( activity_p act )
{
    cthread_p thd = act->thread;
    /* XXX locking necessary? */
    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p waiter;
    while( ( waiter = crcl(pop_waiting_queue)( &act->waiters ) ) )
    {
        crcl(push_ready_queue)( waiter );
    }
    uv_mutex_unlock( &thd->thd_management_mtx );
}

crcl(frame_p) crcl(activity_epilogue)( crcl(frame_p) frame )
{
    assert( frame );
    activity_p act = frame->activity;
    // zlog_debug( crcl(c) , "Activity finished %p %p %p", frame, act, act->newest_frame );
    assert( frame == act->oldest_frame );
    assert( frame == act->newest_frame );
    assert( !CRCL(CHECK_FLAG)( *act, CRCL(ACTF_READY) ) );
    assert( !CRCL(CHECK_FLAG)( *act, CRCL(ACTF_WAITING) ) );
    assert( act->thread->running == act );
    CRCL(SET_FLAG)( *act, CRCL(ACTF_DONE) );
    clean_up_activity( act );
    crcl(frame_p) rv = crcl(activity_waiting_or_done)( frame, NULL );
    free( frame );
    return rv;
}

/* Initialize 'activity' and add it to 'thread'. */
/* (Do not start the new activity running) */
void activate_in_thread(
    cthread_p thread,
    activity_p activity,
    crcl(frame_p) caller,
    crcl(frame_p) frame )
{
    assert( activity );
    assert( frame );
    assert( caller );
    assert( caller->activity );
    assert( thread );
    assert( caller->activity != activity );

    // zlog_debug( crcl(c), "Activate %p", activity );

    activity->thread        = thread;
    activity->flags         = 0;
    activity->yield_calls   = 0;
    activity->newest_frame  = frame;
    activity->oldest_frame  = frame;
    activity->qs[0].next    = NULL;
    activity->qs[0].prev    = NULL;
    activity->qs[1].next    = NULL;
    activity->qs[1].prev    = NULL;
    activity->waiters       = NULL;
    frame->caller           = NULL;
    frame->activity         = activity;
    /* Compensate for the wrongness introduced by generic_prologue: */
    caller->activity->newest_frame = caller;
}

/*
 * Assume: "activity" is allocated but uninitialized
 * Assume: "frm" is the frame returned by the relevant init procedure
 */
crcl(frame_p) crcl(activate)(
    crcl(frame_p)     caller,
    void             *ret_addr,
    activity_p        activity,
    crcl(frame_p)     frm )
{
    assert( !caller == !ret_addr );
    assert( activity );
    caller->return_addr = ret_addr;
    if( !frm )
    {
        CRCL(SET_FLAG)( *activity, CRCL(ACTF_OOM) );
        return caller;
    }
    if( caller )
    {   /* Currently in yielding context */
        // zlog_info( crcl(c), "crcl(activate) Y new:%p old:%p", activity, caller->activity );
        activity_p caller_act = caller->activity;
        activate_in_thread( caller_act->thread, activity, caller, frm );
        return switch_from_to( caller_act, activity );
    }
    else
    {   /* Currently in no_yield context */
        zlog_info( crcl(c), "UNIMP crcl(activate) NY %p", activity );
        exit( 1 );
    }
}

/*
 * Assume: act and currently running activity belong to the same thread
 * Assume: act is different from the currently running activity
 */
void crcl(activity_cancel_impl)( activity_p act )
{
    /* act must have been waiting or ready */
    int ready   = !!( act->flags & CRCL(ACTF_READY) );
    int waiting = !!( act->flags & CRCL(ACTF_WAITING) );
    assert( ready || waiting );
    assert( !( ready && waiting ) );
    crcl(frame_p) frm = act->newest_frame;
    while( frm )
    {
        crcl(frame_p) caller = frm->caller;
        free( frm );
        frm = caller;
    }
    unsigned flag = ready ? CRCL(ACTF_READY) : CRCL(ACTF_WAITING);
    remove_activity_from_queue( act, flag );
    if( waiting )
    {
        remove_activity_from_queue( act, 0 );
    }
    clean_up_activity( act );
}

#if 0
    deprecated?
static void crcl(report_thread_done)( activity_p a )
{
    /* zlog_info( crcl(c), "XXX Thread is done!!!\n" ); */
}
#endif

/* NOTE: It might make good performance sense to inline all these
 * generic yielding call helper functions.  I'm going to leave that
 * decision to the system for now. */
/* NOTE: These generic helpers would be a convenient place to put in
 * debugging stuff. */

crcl(frame_p) crcl(fn_generic_prologue)(
    size_t sz,
    void *return_ptr,
    crcl(frame_p) caller,
    crcl(frame_p) (*fn)( crcl(frame_p) ) )
{
    assert( caller );
    assert( caller->activity );
    assert( fn );
    /* NOTE: malloc and free are a considerable source of overhead in
     * Charcoal.  Some day we should experiment with different
     * allocators. */
    size_t frm_size = sz + sizeof( caller[0] );
    crcl(frame_p) frm = (crcl(frame_p))malloc( frm_size );
    if( !frm )
    {
        return NULL;
    }
    // zlog_debug( crcl(c), "generic_prologue %p %p", caller, caller->activity );
    caller->callee      = frm;
    caller->return_addr = return_ptr;
    frm->size           = frm_size;
    frm->fn             = fn;
    frm->caller         = caller;
    frm->callee         = NULL;
    frm->return_addr    = NULL;
    frm->activity       = caller->activity;
    /* WARNING: The following line is correct when used for procedure
     * calls, but not activates.  To keep calls as fast as possible we
     * allow this wrongness here and compensate for it in activate. */
    /* NOTE: We might be able to get away with updating newest only on
     * context switches. */
    caller->activity->newest_frame = frm;
    return frm;
}

crcl(frame_p) crcl(fn_generic_epilogue)( crcl(frame_p) frm )
{
    crcl(frame_p) caller = frm->caller;
    /* NOTE: We might be able to get away with updating newest only on
     * context switches. */
    frm->activity->newest_frame = caller;
    /* XXX make malloc/free configurable? */
    free( frm );
#if 0
    /* NOTE: Zeroing the callee field is not strictly necessary, and
     * therefore might be wasteful.  However, one should not be
     * nickel-and-diming the performance of yielding calls anyway (use
     * no_yield calls instead). */
    if( caller )
        caller->callee = NULL;
#endif
    return caller;
}

crcl(frame_p) crcl(alloca)( crcl(frame_p) frm, size_t s, void **p )
{
    assert( p );
    size_t pre_size = frm->size;
    size_t post_size = pre_size + s;
    crcl(frame_p) new_frame = (crcl(frame_p))realloc( frm, post_size );
    if( !new_frame )
    {
        exit( 1 );
    }
    *p = ((void *)new_frame) + pre_size;
    new_frame->size = post_size;
    /* XXX Must make sure all pointers to this frame are updated. */
    new_frame->activity->newest_frame = new_frame;
    return new_frame;
}
