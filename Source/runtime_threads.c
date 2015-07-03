/*
 * This is the thread stuff for the coroutine-based implementation
 */

#include <core.h>
#include <assert.h>
#include <stdlib.h>
#include <runtime_coroutine.h>
#include <runtime_io_commands.h>
#include <opa_primitives.h>

activity_p crcl(pop_ready_queue)( cthread_p t );

static cthread_p crcl(threads);

struct thread_entry_params
{
    crcl(sem_t) s1, s2;
    cthread_p  *tptr;
    void       *options;
};

static void thread_main_loop( void *p );
static crcl(frame_p) thread_init( struct thread_entry_params *params, cthread_p thread );
static void thread_finish( cthread_p thread );
static crcl(frame_p) idle( crcl(frame_p) idle_frame );
static void add_to_threads_list( cthread_p thd );

/* XXX Probably should wrap the whole thread starting process up in a
 * critical region.  Thread starting should be relatively uncommon, so
 * the (in any case minor) performance concern is really negligible. */
int thread_start( cthread_p *thread, void *options )
{
    int rc;
    zlog_info( crcl(c), "Starting Charcoal thread %p( %p )\n", thread, options );

    // pthread_attr_t attr;
    // ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    /* Because yielding call frames are heap-allocated, the thread's
     * stack only needs to be big enough for the maximum chain of
     * no_yield calls, which shouldn't be very long.  However, TODO:
     * this should probably be configurable. */
    // ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN ) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    struct thread_entry_params params;
    rc = crcl(sem_init)( &params.s1, 0, 0 );
    assert( !rc );
    rc = crcl(sem_init)( &params.s2, 0, 0 );
    assert( !rc );
    params.tptr    = thread;
    params.options = options;

    uv_thread_t thread_id;
    if( ( rc = uv_thread_create( &thread_id, thread_main_loop, &params ) ) )
    {
        return rc;
    }
    /* Must wait for thread initialization to complete, because the
     * Charcoal thread struct is stack-allocated in the new thread. */
    crcl(sem_decr)( &params.s1 );
    (*thread)->sys = thread_id;
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

    zlog_info( crcl(c), "Charcoal thread started %p( %p )\n", thread, options );
    return 0;
}

static void thread_main_loop( void *p )
{
    assert( p );
    cthread_p thread = (cthread_p)malloc( sizeof( thread[0] ) );
    crcl(frame_p) frame =
        thread_init( (struct thread_entry_params *)p, thread );
    do
    {
        frame = frame->fn( frame );
    } while( frame );

    thread_finish( thread );
}

static crcl(frame_p) thread_init(
    struct thread_entry_params *params, cthread_p thread )
{
    zlog_info( crcl(c), "Charcoal thread initializing %p( %p )\n", thread, params );
    /* XXX  init can-run */
    *params->tptr = thread;
    // OPA_store_int( (OPA_int_t *)&thread->timeout, 0 );
    OPA_store_int( (OPA_int_t *)&thread->interrupt_activity, 0 );
    thread->timer_req.data = thread;
    /* XXX Does timer_init have to be called from the I/O thread? */
    uv_timer_init( crcl(io_loop), &thread->timer_req );
    // XXX thread->start_time = 0.0;
    // XXX thread->max_time = 0.0;

    assert( !uv_mutex_init( &thread->thd_management_mtx ) );
    assert( !uv_cond_init( &thread->thd_management_cond ) );

    thread->flags               = 0;
    thread->runnable_activities = 0;
    thread->activities          = NULL;
    thread->ready               = NULL;
    // thread->sys initialized in thread_start
    CRCL(SET_FLAG)( *thread, CRCL(THDF_IDLE) );
    CRCL(SET_FLAG)( *thread, CRCL(THDF_NEVER_RUN) );
    thread->idle_act.thread                    = thread;
    thread->idle_act.flags                     = 0;
    thread->idle_act.next                      = NULL;
    thread->idle_act.prev                      = NULL;
    thread->idle_act.snext                     = NULL;
    thread->idle_act.sprev                     = NULL;
    thread->idle_act.waiters_front             = NULL;
    thread->idle_act.waiters_back              = NULL;
    thread->idle_act.newest_frame              = &thread->idle_frm;
    thread->idle_act.oldest_frame              = &thread->idle_frm;
    thread->idle_act.yield_calls               = 0;
    thread->idle_act.oldest_frame->fn          = idle;
    thread->idle_act.oldest_frame->activity    = &thread->idle_act;
    thread->idle_act.oldest_frame->caller      = NULL;
    thread->idle_act.oldest_frame->callee      = NULL;
    thread->idle_act.oldest_frame->return_addr = NULL;
    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    assert( !crcl(sem_incr)( &params->s1 ) );
    /* Must wait for parent thread to initialize the sys field. */
    assert( !crcl(sem_decr)( &params->s2 ) );
    add_to_threads_list( thread );
    assert( !crcl(sem_incr)( &params->s1 ) );
    return thread->idle_act.oldest_frame; /* XXX */
}

static void thread_finish( cthread_p thread )
{
    zlog_info( crcl(c), "Charcoal thread finished %p\n", thread );

    crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    cmd->command = CRCL(IO_CMD_JOIN_THREAD);
    /* XXX Whoa! use after free? */
    cmd->_.thread = thread;
    enqueue( cmd );
    assert( !uv_async_send( &crcl(io_cmd) ) );
    /* zlog_debug( crcl(c), "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
}

int crcl(join_thread)( cthread_p t )
{
    zlog_info( crcl(c), "Joining thread %p n:%p p:%p ts:%p\n",
               t, t->next, t->prev, crcl(threads) );
    if( t == crcl(main_thread) )
    {
        //XXXint *p = (int *)crcl(main_activity).return_value;
        crcl(process_return_value) = 42;//*p;
    }
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
    /* XXX free t? */
    return crcl(threads) == crcl(threads)->next;
}

static crcl(frame_p) idle( crcl(frame_p) idle_frame )
{
    activity_p idle  = idle_frame->activity;
    cthread_p thread = idle->thread;

    uv_mutex_lock( &thread->thd_management_mtx );
    while( !thread->ready )
    {
        CRCL(SET_FLAG)( *thread, CRCL(THDF_IDLE) );
        uv_cond_wait( &thread->thd_management_cond,
                      &thread->thd_management_mtx );
        /* XXX Add the ability to cleanly kill a thread? */
    }
    if( !thread->activities )
    {
        /* NOTE: We arrived here because the ready flag was set and the
         * activties list is NULL. */
        uv_mutex_unlock( &thread->thd_management_mtx );
        return 0;
    }
    activity_p a = crcl(pop_ready_queue)( thread );
    assert( a );
    crcl(frame_p) next_frame = a->newest_frame;
    CRCL(CLEAR_FLAG)( *thread, CRCL(THDF_IDLE) );
    uv_mutex_unlock( &thread->thd_management_mtx );

    return next_frame;
}

/* XXX Data race??? */
static void add_to_threads_list( cthread_p t )
{
    cthread_p last = crcl(threads)->prev;
    last->next = t;
    crcl(threads)->prev = t;
    t->next = crcl(threads);
    t->prev = last;
}

int crcl(init_threads)( cthread_p thd, activity_p act )
{
    if( !thd )
        return -EINVAL;
    if( !act )
        return -EINVAL;
    crcl(threads)   = thd;
    thd->activities = act;
    thd->ready      = NULL;
    thd->next       = thd;
    thd->prev       = thd;
    thd->sys        = uv_thread_self();
    act->thread     = thd;
    return 0;
}
