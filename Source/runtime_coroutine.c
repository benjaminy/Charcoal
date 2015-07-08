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
#include <opa_primitives.h>

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
    // HUH??? ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    uv_key_set( &crcl(self_key), act );
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) self->container->max_time);
    // OPA_store_int(&self->container->timeout, 0);
    act->yield_calls = 0;
    /* XXX Races with other interruptions coming in!!! */
    OPA_store_int( (OPA_int_t *)&thd->interrupt_activity, 0 );

    /* XXX: Lots to fix here. */
    if( thd->ready && !CRCL(CHECK_FLAG)( *thd, CRCL(THDF_TIMER_ON) ) )
    {
        CRCL(SET_FLAG)( *thd, CRCL(THDF_TIMER_ON) );
        crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
        cmd->command = CRCL(IO_CMD_START);
        cmd->_.thread = thd;
        // zlog_debug( crcl(c) , "Send timer req cmd: %p\n", cmd );
        enqueue( cmd );
        ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    }

    return act->newest_frame;
}

#if 0
    deprecated?
static void crcl(print_special_queue)( activity_p *q )
{
    assert( q );
    activity_p a = *q, first = a;
    if( a )
    {
        zlog_info( crcl(c), "Special queue: " );
        do {
            zlog_info( crcl(c), "%p  ", a );
            a = a->snext;
        } while( a != first );
        zlog_info( crcl(c), "\n" );
    }
    else
    {
        zlog_info( crcl(c), "Special queue empty\n" );
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
    case CRCL(ACTF_WAITING):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* zlog_info( crcl(c), "S-pop pre " ); */
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
        CRCL(CLEAR_FLAG)( *a, queue_flag );
        return a;
    }
    else
    {
        return NULL;
    }
    /* zlog_info( crcl(c), "S-pop post " ); */
    /* crcl(print_special_queue)( q ); */
}

activity_p crcl(pop_ready_queue)( cthread_p t )
{
    return crcl(pop_special_queue)( CRCL(ACTF_READY_QUEUE), t, NULL );
}

#if 0
    deprecated?
static activity_p crcl(pop_waiting_queue)( cthread_p t )
{
    return crcl(pop_special_queue)( CRCL(ACTF_WAITING), t, NULL ); /* XXX */
}
#endif

/* Precondition: The thread mgmt mutex is held. */
void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a, cthread_p t, activity_p *qp )
{
    assert( t || qp );
    assert( !( t && qp ) );
    if( CRCL(CHECK_FLAG)( *a, queue_flag ) )
    {
        /* zlog_debug( crcl(c), "Already in queue\n" ); */
        return;
    }
    CRCL(SET_FLAG)( *a, queue_flag );
    activity_p *q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_WAITING):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* zlog_debug( crcl(c), "S-push pre q:%p %p ", q, a ); */
    /* crcl(print_special_queue)( q ); */
    if( *q )
    {
        activity_p front = *q, rear = front->sprev;
        /* zlog_debug( crcl(c), "Front: %p(n:%p p:%p)  Rear:%p(n:%p p:%p)\n", */
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
    /* zlog_debug( crcl(c), "S-push post q:%p %p ", q, a ); */
    /* crcl(print_special_queue)( q ); */
}

void crcl(push_ready_queue)( activity_p a, cthread_p t )
{
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), a, t, NULL );
}

void crcl(push_waiting_queue)( activity_p a, cthread_p t )
{
    crcl(push_special_queue)( CRCL(ACTF_WAITING), a, t, NULL ); /* XXX */
}

static crcl(frame_p) switch_from_to( activity_p from, activity_p to )
{
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_DONE) ) );
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_READY_QUEUE) ) );
    assert( !CRCL(CHECK_FLAG)( *from, CRCL(ACTF_WAITING) ) );
    cthread_p thd = from->thread;
    assert( thd );
    uv_mutex_lock( &thd->thd_management_mtx );
    crcl(push_ready_queue)( from, thd );
    crcl(frame_p) rv = crcl(activity_start_resume)( to );
    // zlog_debug( crcl(c) , "Switch from->to thd:%p %p %p %p", thd,
    //             from, to, thd->ready );
    uv_mutex_unlock( &thd->thd_management_mtx );
    //zlog_debug( crcl(c), "Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
    return rv;
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
crcl(frame_p) crcl(yield_impl)( crcl(frame_p) frm, void *ret_addr ){
    activity_p act = frm->activity;
    cthread_p  thd = act->thread;
    int         *p = (int *)&frm->callee;

    frm->return_addr = ret_addr;
    ++act->yield_calls;
    int interrupt = OPA_load_int( (OPA_int_t *)&thd->interrupt_activity );

    *p = 0;
    if( !interrupt )
    {
        return frm;
    }
    /* "else": The current activity's quantum has expired. */
    // zlog_debug( crcl(c) , "Yield switch %p", act );
    *p = 1;
    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p next = crcl(pop_ready_queue)( thd );
    uv_mutex_unlock( &thd->thd_management_mtx );
    /* We really shouldn't be getting interrupted if there's nothing in
     * the ready queue */
    assert( next );
    return switch_from_to( act, next );
}

void crcl(add_to_waiters)( activity_p waiter, activity_p waitee )
{
    waiter->snext = NULL;
    if( waitee->waiters_front )
    {
        assert( waitee->waiters_back );
        waitee->waiters_back->snext = waiter;
        waiter->sprev = waitee->waiters_back;
        waitee->waiters_back = waiter;
    }
    else
    {
        assert( !waitee->waiters_front );
        waitee->waiters_front = waitee->waiters_back = waiter;
        waiter->sprev = NULL;
    }
    CRCL(SET_FLAG)( *waiter, CRCL(ACTF_WAITING) );
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

static void insert_activity_into_thread( activity_p a, cthread_p t )
{
    /* zlog_debug( crcl(c), "Insert activity %p  %p\n", a, t ); */
    if( t->activities )
    {
        activity_p rear = t->activities->prev;
        /* zlog_debug( crcl(c), "f:%p  r:%p\n", t->activities, rear ); */
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
}

static void crcl(remove_activity_from_thread)( activity_p a, cthread_p t)
{
    /* zlog_debug( crcl(c), "Remove activity %p  %p\n", a, t ); */
    if( a == a->next )
    {
        t->activities = NULL;
    }
    if( t->activities == a )
    {
        t->activities = a->next;
    }
    a->next->prev = a->prev;
    a->prev->next = a->next;
    a->next = NULL;
    a->prev = NULL;

    // zlog_debug( crcl(c), "f:%p  r:%p\n", t->activities, t->activities->prev );
}

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
    bool done = CRCL(CHECK_FLAG)( *act, CRCL(ACTF_DONE) );
    if( !done )
    {
        crcl(push_waiting_queue)( act, thd );
    }
    activity_p next = crcl(pop_ready_queue)( thd );
    zlog_debug( crcl(c) , "Activity waiting/done %d %p", done, next );
    if( next )
    {
        /* Hooray! */
    }
    else if( !done /* TODO: || keep thread alive */ )
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

crcl(frame_p) crcl(activity_epilogue)( crcl(frame_p) frame )
{
    assert( frame );
    activity_p act = frame->activity;
    zlog_debug( crcl(c) , "Activity finished %p %p %p", frame, act, act->newest_frame );
    assert( frame == act->oldest_frame );
    assert( NULL == act->newest_frame );
    /* XXX I think this return value business is broken currently. */
    CRCL(SET_FLAG)( *act, CRCL(ACTF_DONE) );
    /* XXX */
    act->epilogueB( frame, 0 );
    cthread_p t = act->thread;
    /* XXX locking necessary? */
    uv_mutex_lock( &t->thd_management_mtx );
    crcl(remove_activity_from_thread)( act, t );
    uv_mutex_unlock( &t->thd_management_mtx );
    return crcl(activity_waiting_or_done)( frame, NULL );
}

/* Initialize 'activity' and add it to 'thread'. */
/* (Do not start the new activity running) */
void activate_in_thread(
    cthread_p thread,
    activity_p activity,
    crcl(frame_p) caller,
    crcl(frame_p) frame,
    crcl(epilogueB_t) epi )
{
    assert( activity );
    assert( frame );
    assert( caller );
    assert( caller->activity );
    assert( thread );
    assert( epi );
    assert( caller->activity != activity );

    zlog_debug( crcl(c), "Activate %p", activity );

    activity->thread        = thread;
    activity->flags         = 0;
    activity->yield_calls   = 0;
    activity->newest_frame  = frame;
    activity->oldest_frame  = frame;
    activity->snext         = NULL;
    activity->sprev         = NULL;
    activity->waiters_front = NULL;
    activity->waiters_back  = NULL;
    activity->epilogueB     = epi;
    frame->caller           = NULL;
    frame->activity         = activity;
    /* Compensate for the wrongness introduced by generic_prologue: */
    caller->activity->newest_frame = caller;
    uv_mutex_lock( &thread->thd_management_mtx );
    insert_activity_into_thread( activity, thread );
    uv_mutex_unlock( &thread->thd_management_mtx );
}

/*
 * Assume: "activity" is allocated but uninitialized
 * Assume: "f" is the frame returned by the relevant init procedure
 */
crcl(frame_p) crcl(activate)(
    crcl(frame_p)     caller,
    void             *ret_addr,
    activity_p        activity,
    crcl(frame_p)     frm,
    crcl(epilogueB_t) epi )
{
    assert( !caller == !ret_addr );
    assert( activity );
    assert( frm );
    assert( epi );
    zlog_info( crcl(c), "crcl(activate) %p %p", caller, caller ? caller->activity : 0 );
    if( caller )
    {   /* Currently in yielding context */
        caller->return_addr = ret_addr;
        activity_p caller_act = caller->activity;
        activate_in_thread( caller_act->thread, activity, caller, frm, epi );
        return switch_from_to( caller_act, activity );
    }
    else
    {   /* Currently in no_yield context */
        zlog_info( crcl(c), "UNIMP: Activation in no_yield context\n" );
        exit( 1 );
    }
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
    crcl(frame_p) frm = (crcl(frame_p))malloc( sz + sizeof( frm[0] ) );
    if( !frm )
    {
        exit( -ENOMEM );
    }
    zlog_debug( crcl(c), "generic_prologue %p %p", caller, caller->activity );
    caller->callee      = frm;
    caller->return_addr = return_ptr;
    frm->fn             = fn;
    frm->caller         = caller;
    frm->callee         = NULL;
    frm->return_addr    = NULL;
    frm->activity       = caller->activity;
    /* WARNING: The following line is correct when used for procedure
     * calls, but not activates.  To keep calls as fast as possible we
     * allow this wrongness here and compensate for it in activate. */
    caller->activity->newest_frame = frm;
    return frm;
}

crcl(frame_p) crcl(fn_generic_epilogueA)( crcl(frame_p) frm )
{
    frm->activity->newest_frame = frm->caller;
    return frm->caller;
}

void crcl(fn_generic_epilogueB)( crcl(frame_p) frm )
{
    /* XXX make malloc/free configurable? */
    free( frm->callee );
    /* NOTE Zeroing the callee field is not strictly necessary, and
     * therefore might be wasteful.  However, one should not be
     * nickel-and-diming the performance of yielding calls anyway (use
     * no_yield calls instead). */
    frm->callee = NULL;
}
