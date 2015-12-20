/*
 * This is the thread stuff for the coroutine-based implementation
 */

#include <core.h>
#include <core_runtime.h>
#include <assert.h>
#include <stdlib.h>
#include <runtime_coroutine.h>
#include <runtime_io_commands.h>
#include <atomics_wrappers.h>

static cthread_p crcl(threads) = NULL;

typedef struct
{
    crcl(sem_t) s1, s2;
    cthread_p   thd;
    void       *options;
} thread_entry_params;

static crcl(frame_p) thread_init( thread_entry_params *params );
static void thread_main_loop( void *p );
static void thread_finish( cthread_p thread );
static crcl(frame_p) idle( crcl(frame_p) idle_frame );
static void add_to_threads_list( cthread_p thd );

/* XXX Probably should wrap the whole thread starting process up in a
 * critical region.  Thread starting should be relatively uncommon, so
 * the (in any case minor) performance concern is really negligible. */
int thread_start( cthread_p thd, void *options )
{
    int rc;
    zlog_info( crcl(c), "Thread start.  thread: %p  opts: %p", thd, options );

    // pthread_attr_t attr;
    // ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    /* Because yielding call frames are heap-allocated, the thread's
     * stack only needs to be big enough for the maximum chain of
     * no_yield calls, which shouldn't be very long.  However, TODO:
     * this should probably be configurable. */
    // ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN ) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    thread_entry_params params;
    rc = crcl(sem_init)( &params.s1, 0, 0 );
    assert( !rc );
    rc = crcl(sem_init)( &params.s2, 0, 0 );
    assert( !rc );
    params.thd    = thd;
    params.options = options;

    uv_thread_t thread_id;
    if( ( rc = uv_thread_create( &thread_id, thread_main_loop, &params ) ) )
    {
        return rc;
    }
    /* Must wait for thread initialization to complete, because the
     * Charcoal thread struct is stack-allocated in the new thread. */
    crcl(sem_decr)( &params.s1 );
    thd->sys = thread_id;
    crcl(sem_incr)( &params.s2 );
    /* Must wait for acknowledgment from new thread, because the
     * semaphores are stack-allocated here, so can't be deallocated
     * until the new thread is ready. */
    crcl(sem_decr)( &params.s1 );
    rc = crcl(sem_destroy)( &params.s1 );
    assert( !rc );
    rc = crcl(sem_destroy)( &params.s2 );
    assert( !rc );
    // assert( !pthread_attr_destroy( &attr ) );

    // zlog_info( crcl(c), "Charcoal thread started %p( %p )", thd, options );
    return 0;
}

static void thread_main_loop( void *p )
{
    assert( p );
    thread_entry_params *params = (thread_entry_params *)p;
    assert( params->thd );
    volatile cthread_p thd = params->thd;
    crcl(frame_p) frm = thread_init( params );
    if( setjmp( thd->thread_main_jmp_buf ) )
    {
        frm = thd->running->newest_frame;
    }
    if( !frm )
        exit( -EINVAL );
    do {
        frm = frm->fn( frm );
    } while( frm );

    thread_finish( thd );
}

static crcl(frame_p) thread_init( thread_entry_params *params )
{
    if( !params->thd )    exit( -1 );
    if( !params ) exit( -1 );
    cthread_p thd = params->thd;
    zlog_info( crcl(c), "New thread.  thread: %p  params: %p", thd, params );
    /* XXX  init can-run */
    // atomic_store_int( &thd->timeout, 0 );
    atomic_store_int( &thd->interrupt_activity, 0 );
    thd->timer_req.data = thd;
    /* XXX Does timer_init have to be called from the I/O thread? */
    if( uv_timer_init( crcl(io_loop), &thd->timer_req ) )
        exit( -1 );
    // XXX thd->start_time = 0.0;
    // XXX thd->max_time = 0.0;

    if( uv_mutex_init( &thd->thd_management_mtx ) )
        exit( -1 );
    if( uv_cond_init( &thd->thd_management_cond ) )
        exit( -1 );

    thd->flags               = 0;
    atomic_store_int( &thd->waiting_activities, 0 );
    thd->waiting             = NULL;
    thd->ready               = NULL;
    // thd->sys initialized in thread_start
    CRCL(SET_FLAG)( *thd, CRCL(THDF_IDLE) );
    CRCL(SET_FLAG)( *thd, CRCL(THDF_NEVER_RUN) );
    thd->idle_act.thread                    = thd;
    thd->idle_act.flags                     = 0;
    thd->idle_act.qs[0].next                = NULL;
    thd->idle_act.qs[0].prev                = NULL;
    thd->idle_act.qs[1].next                = NULL;
    thd->idle_act.qs[1].prev                = NULL;
    thd->idle_act.waiters                   = NULL;
    thd->idle_act.newest_frame              = &thd->idle_frm;
    thd->idle_act.oldest_frame              = &thd->idle_frm;
    thd->idle_act.yield_calls               = 0;
    thd->idle_act.oldest_frame->fn          = idle;
    thd->idle_act.oldest_frame->activity    = &thd->idle_act;
    thd->idle_act.oldest_frame->caller      = NULL;
    thd->idle_act.oldest_frame->callee      = NULL;
    thd->idle_act.oldest_frame->return_addr = NULL;
    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope
    if( crcl(sem_incr)( &params->s1 ) ) exit( -1 );
    /* Must wait for parent thread to initialize the sys field. */
    if( crcl(sem_decr)( &params->s2 ) ) exit( -1 );
    add_to_threads_list( thd );
    if( crcl(sem_incr)( &params->s1 ) ) exit( -1 );
    return thd->idle_act.newest_frame;
}

/* XXX this is all wrong. Needs fixing for multithreading to work */
static void async_call_close( uv_handle_t *h )
{
    /* uv_async_t *a = (uv_async_t *)h; */
    /* zlog_debug( crcl(c), "CLOSE %p\n", a ); fflush(stdout); */
}

static void thread_finish_impl(
    uv_loop_t *loop, uv_handle_t *handle, crcl(async_call_p) async )
{
    cthread_p thd = (cthread_p)async->specific;
    if( crcl(join_thread)( thd ) )
    {
        /* zlog_debug( stderr, "Close, please\n" ); */
        /* XXX What about when there are more events???. */
        uv_close( (uv_handle_t *)handle, async_call_close );
    }
}

static void thread_finish( cthread_p thread )
{
    zlog_info( crcl(c), "Thread finished %p", thread );

    /* XXX alloc issues??? */
    crcl(async_call_p) async = &thread->finished_call;
    async->f = thread_finish_impl;
    /* XXX Whoa! use after free? */
    async->specific = (void *)thread;
    enqueue( async );
    assert( !uv_async_send( &crcl(io_cmd) ) );
    /* zlog_debug( crcl(c), "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
}

int crcl(join_thread)( cthread_p t )
{
    int rv = 0;
    zlog_info( crcl(c), "Cleaning up thread %p  n: %p  p: %p  ts: %p",
               t, t->next, t->prev, crcl(threads) );
    if( t == t->next )
    {
        // zlog_info( crcl(c), "Joining the last thread" );
        rv = 1;
    }
    else
    {
        t->next->prev = t->prev;
        t->prev->next = t->next;
    }
    assert( !uv_thread_join( &t->sys ) );
    /* XXX free t? */
    return rv;
}

static crcl(frame_p) idle( crcl(frame_p) idle_frame )
{
    activity_p idle = idle_frame->activity;
    cthread_p   thd = idle->thread;

    uv_mutex_lock( &thd->thd_management_mtx );
    activity_p next;
    while( !( next = crcl(pop_ready_queue)( thd ) ) )
    {
        CRCL(SET_FLAG)( *thd, CRCL(THDF_IDLE) );
        uv_cond_wait( &thd->thd_management_cond,
                      &thd->thd_management_mtx );
        /* XXX Add the ability to cleanly kill a thread? */
    }
    // zlog_info( crcl(c), "IDLE WAKE UP i:%p n:%p\n", idle, next );
    crcl(frame_p) next_frame = crcl(activity_start_resume)( next );
    CRCL(CLEAR_FLAG)( *thd, CRCL(THDF_IDLE) );
    uv_mutex_unlock( &thd->thd_management_mtx );

    return next_frame;
}

/* XXX Data race??? */
static void add_to_threads_list( cthread_p thd )
{
    if( crcl(threads) )
    {
        cthread_p last = crcl(threads)->prev;
        last->next = thd;
        crcl(threads)->prev = thd;
    }
    else
    {
        crcl(threads) = thd;
        thd->next = thd;
        thd->prev = thd;
    }
}
