#include <core.h>
#include <core_runtime.h>
#include <assert.h>
// #include <stdio.h>
#include <stdlib.h>
#include <runtime_io_commands.h>
#include <runtime_coroutine.h>
#include <atomics_wrappers.h>

uv_loop_t *crcl(io_loop);
uv_async_t crcl(io_cmd);
static uv_idle_t start_the_world;

/* NOTE: Using the classic two-stack queue implementation */
typedef struct
{
    crcl(async_call_p) front, back;
    uv_mutex_t mtx;
} crcl(cmd_queue_t);

static crcl(cmd_queue_t) crcl(cmd_queue);

void enqueue( crcl(async_call_p) cmd )
{
    /* The queue takes ownership of *cmd */
    uv_mutex_lock( &crcl(cmd_queue).mtx );
    cmd->next = crcl(cmd_queue).back;
    crcl(cmd_queue).back = cmd;
    uv_mutex_unlock( &crcl(cmd_queue).mtx );
}

/*
 * Removes an item from the command queue.  The caller takes ownership
 * of the memory.
 */
crcl(async_call_p) dequeue( void )
{
    if( !crcl(cmd_queue).front )
    {
        uv_mutex_lock( &crcl(cmd_queue).mtx );
        while( crcl(cmd_queue).back )
        {
            crcl(async_call_p) tmp = crcl(cmd_queue).back->next;
            crcl(cmd_queue).back->next = crcl(cmd_queue).front;
            crcl(cmd_queue).front = crcl(cmd_queue).back;
            crcl(cmd_queue).back = tmp;
        }
        uv_mutex_unlock( &crcl(cmd_queue).mtx );
    }
    if( crcl(cmd_queue).front )
    {
        crcl(async_call_p) f = crcl(cmd_queue).front;
        crcl(cmd_queue).front = f->next;
        return f;
    }
    else
    {
        return NULL;
    }
}

static void timer_expired( uv_timer_t* handle )
{
    cthread_p thd = (cthread_p)handle->data;
    uv_mutex_lock( &thd->thd_management_mtx );
    // zlog_debug( crcl(c) , "Timer expired! thd:%p %p %p", thd,
    //            &thd->interrupt_activity, thd->ready );
    CRCL(CLEAR_FLAG)( *thd, CRCL(THDF_TIMER_ON) );
    if( thd->ready )
        atomic_store_int( &thd->interrupt_activity, 1 );
    uv_mutex_unlock( &thd->thd_management_mtx );
}

static int wake_up_waiters( activity_p *waiters )
{
    /* XXX multithreading :( */
    activity_p waiter, next_waiter = crcl(pop_waiting_queue)( waiters );
    while( next_waiter )
    {
        /* NOTE: Tricky timing here with the deallocation of the
         * original invoker's stack frame. */
        waiter = next_waiter;
        next_waiter = crcl(pop_waiting_queue)( waiters );
        cthread_p thd = waiter->thread;
        uv_mutex_lock( &thd->thd_management_mtx );
        crcl(push_ready_queue)( waiter );
        uv_mutex_unlock( &thd->thd_management_mtx );
        /* If idle */
        uv_cond_signal( &thd->thd_management_cond );
    }
    return 0;
}

void crcl(async_fn_start)( uv_loop_t *loop, uv_handle_t *handle, crcl(async_call_p) async )
{
    cthread_p thd = (cthread_p)async->specific;
    // zlog_debug( crcl(c) , "Timer req recved cmd: %p thd: %p\n", cmd, thd );
    if( CRCL(CHECK_FLAG)( *thd, CRCL(THDF_TIMER_ON) ) )
    {
        /* Very weird timing, but probably possible */
        uv_timer_stop( &thd->timer_req );
    }
    int rc = uv_timer_start( &thd->timer_req, timer_expired, 10, 0);
    assert( !rc );
}

static void io_cmd_cb( uv_async_t *handle )
{
    /* zlog_debug( crcl(c), "IO THING\n" ); */
    crcl(async_call_p) call;
    /* Multiple async_sends might result in a single callback call, so
     * we need to loop until the queue is empty.  (I assume it will be
     * extremely uncommon for this queue to actually grow
     * signnificantly.) */
    while( ( call = dequeue() ) )
    {
        call->f( crcl(io_loop), (uv_handle_t *)handle, call );
    }
}

static void start_cb( uv_idle_t* handle ) {
    int (*f)( void ) = (int (*)( void ))handle->data;
    int rc = f();
    if( rc )
    {
        zlog_error( crcl(c), "Failure: Launch of application main: %d\n", rc );
        uv_stop( crcl(io_loop) );
    }
    uv_idle_stop( handle );
}

/*
 * The initial thread that exists when a Charcoal program starts will
 * be the I/O thread.  Before it starts its I/O duties it launches
 * another thread that will call the application's main procedure.
 */
int crcl(init_io_loop)( int (*f)( void ) )
{
    RET_IF_ERROR( uv_key_create( &crcl(self_key) ) );
    crcl(io_loop) = uv_default_loop();
    RET_IF_ERROR(
        uv_async_init( crcl(io_loop), &crcl(io_cmd), io_cmd_cb ) );

    crcl(cmd_queue).front = NULL;
    crcl(cmd_queue).back = NULL;
    RET_IF_ERROR( uv_mutex_init( &crcl(cmd_queue).mtx ) );

    /* app_main doesn't run until the event loop is actually started */
    RET_IF_ERROR( uv_idle_init( crcl(io_loop), &start_the_world ) );
    start_the_world.data = f;
    RET_IF_ERROR( uv_idle_start( &start_the_world, start_cb ) );

    return 0;
}
