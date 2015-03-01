/*
 * This is the thread stuff for the coroutine-based implementation
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <charcoal_runtime_coroutine.h>
#include <charcoal_runtime_io_commands.h>

extern cthread_p crcl(threads);
activity_p crcl(pop_ready_queue)( cthread_p t );

struct crcl(thread_entry_params)
{
    crcl(sem_t) s1, s2;
    cthread_p  *t;
    void       *options;
};

static void add_to_threads_list( cthread_p thd );
static void thread_init(
    struct crcl(thread_entry_params) *params, cthread_p thread )
{
    /* XXX  init can-run */
    *params->t = thread;
    crcl(atomic_store_int)( &thread->unyield_depth, 0 );
    // crcl(atomic_store_int)( &thread->timeout, 0 );
    crcl(atomic_store_int)( &thread->keep_going, 1 );
    thread->timer_req.data = &thread;
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
    CRCL(SET_FLAG)( thread->flags, CRCL(THDF_IDLE) );
    CRCL(SET_FLAG)( thread->flags, CRCL(THDF_NEVER_RUN) );
    
    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    /* XXX Surely this handshake could be more elegant */
    assert( !crcl(sem_incr)( &params->s1 ) );
    assert( !crcl(sem_decr)( &params->s2 ) );
    assert( !crcl(sem_incr)( &params->s1 ) );
    assert( !crcl(sem_destroy)( &params->s2 ) );
    add_to_threads_list( thread );
}

static void add_to_threads_list( cthread_p t )
{
    cthread_p last = crcl(threads)->prev;
    last->next = t;
    crcl(threads)->prev = t;
    t->next = crcl(threads);
    t->prev = last;
}

static void thread_finish( cthread_p thread )
{

    crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    cmd->command = CRCL(IO_CMD_JOIN_THREAD);
    cmd->_.thread = thread;
    enqueue( cmd );
    assert( !uv_async_send( &crcl(io_cmd) ) );
    /* printf( "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
}

static void generic_charcoal_thread( void *p )
{
    assert( p );
    cthread_t thread;
    printf( "[CRCL_RT] generic_charcoal_thread %p\n", p );
    thread_init( (struct crcl(thread_entry_params) *)p, &thread );

    crcl(atomic_int) *keep_going = &thread.keep_going;
    while( crcl(atomic_load_int)( keep_going ) )
    {
        uv_mutex_lock( &thread.thd_management_mtx );
        while( crcl(atomic_load_int)( keep_going ) && !thread.ready )
        {
            CRCL(SET_FLAG)( thread.flags, CRCL(THDF_IDLE) );
            uv_cond_wait( &thread.thd_management_cond,
                          &thread.thd_management_mtx );
        }
        if( !crcl(atomic_load_int)( keep_going ) )
        {
            /* TODO: Check reason for interruption */
            uv_mutex_unlock( &thread.thd_management_mtx );
            break;
        }
        activity_p a = crcl(pop_ready_queue)( &thread );
        crcl(frame_p) the_frame = a->top;
        CRCL(CLEAR_FLAG)( thread.flags, CRCL(THDF_IDLE) );
        uv_mutex_unlock( &thread.thd_management_mtx );
        /* The main loop!!! */
        do
        {
            the_frame = the_frame->fn( the_frame );
        } while( the_frame && crcl(atomic_load_int)( keep_going ) );
        uv_mutex_lock( &thread.thd_management_mtx );
        if( !thread.activities
            && !CRCL(CHECK_FLAG)( thread.flags, CRCL(THDF_KEEP_ALIVE) ) )
        {
            uv_mutex_unlock( &thread.thd_management_mtx );
            break;
        }
        uv_mutex_unlock( &thread.thd_management_mtx );
    }

    thread_finish( &thread );
    printf( "[CRCL_RT] generic_charcoal_thread finished\n" );
}

int thread_start( cthread_p *thd, void *options )
{
    int rc;
    printf( "[CRCL_RT] thread_start %p %p\n", thd, options );
    
    // pthread_attr_t attr;
    // ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    /* Because yielding call frames are heap-allocated, the thread's
     * stack only needs to be big enough for the maximum chain of
     * unyielding calls, which shouldn't be very long.  However, TODO:
     * this should probably be configurable. */
    // ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN ) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    /* Maybe make a fixed pool of these ctxs to avoid malloc */
    struct crcl(thread_entry_params) params;
    assert( !crcl(sem_init)( &params.s1, 0, 0 ) );
    assert( !crcl(sem_init)( &params.s2, 0, 0 ) );
    params.t       = thd;
    params.options = options;

    uv_thread_t thread_id;
    if( ( rc = uv_thread_create( &thread_id, generic_charcoal_thread, &params ) ) )
    {
        return rc;
    }
    /* Wait for thread initialization to complete. */
    crcl(sem_decr)( &params.s1 );
    (*thd)->sys = thread_id;
    crcl(sem_incr)( &params.s2 );
    crcl(sem_decr)( &params.s1 );
    assert( !crcl(sem_destroy)( &params.s1 ) );
    // assert( !pthread_attr_destroy( &attr ) );

    printf( "[CRCL_RT] thread_start finished\n" );
    return 0;
}
