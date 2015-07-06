#include <core.h>
#include <assert.h>
// #include <stdio.h>
#include <stdlib.h>
#include <runtime_io_commands.h>
#include <runtime_coroutine.h>
#include <opa_primitives.h>

uv_loop_t *crcl(io_loop);
uv_async_t crcl(io_cmd);
static uv_idle_t start_the_world;

/* NOTE: Using the classic two-stack queue implementation */
typedef struct
{
    crcl(io_cmd_t) *front, *back;
    uv_mutex_t mtx;
} crcl(cmd_queue_t);

static crcl(cmd_queue_t) crcl(cmd_queue);

void enqueue( crcl(io_cmd_t) *cmd )
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
crcl(io_cmd_t) *dequeue( void )
{
    if( !crcl(cmd_queue).front )
    {
        uv_mutex_lock( &crcl(cmd_queue).mtx );
        while( crcl(cmd_queue).back )
        {
            crcl(io_cmd_t) *tmp = crcl(cmd_queue).back->next;
            crcl(cmd_queue).back->next = crcl(cmd_queue).front;
            crcl(cmd_queue).front = crcl(cmd_queue).back;
            crcl(cmd_queue).back = tmp;
        }
        uv_mutex_unlock( &crcl(cmd_queue).mtx );
    }
    if( crcl(cmd_queue).front )
    {
        crcl(io_cmd_t) *f = crcl(cmd_queue).front;
        crcl(cmd_queue).front = f->next;
        return f;
    }
    else
    {
        return NULL;
    }
}

void the_thing( uv_timer_t* handle )
{
    cthread_p thd = (cthread_p)handle->data;
    uv_mutex_lock( &thd->thd_management_mtx );
    // zlog_debug( crcl(c) , "Timer expired! thd:%p %p %p", thd,
    //            &thd->interrupt_activity, thd->ready );
    CRCL(CLEAR_FLAG)( *thd, CRCL(THDF_TIMER_ON) );
    if( thd->ready )
        OPA_store_int( (OPA_int_t *)&thd->interrupt_activity, 1 );
    uv_mutex_unlock( &thd->thd_management_mtx );
}

static void crcl(io_cmd_close)( uv_handle_t *h )
{
    /* uv_async_t *a = (uv_async_t *)h; */
    /* zlog_debug( crcl(c), "CLOSE %p\n", a ); fflush(stdout); */
}

static int crcl(wake_up_requester)( activity_p a )
{
    a->flags &= ~CRCL(ACTF_WAITING);
    cthread_p thd = a->thread;
    uv_mutex_lock( &thd->thd_management_mtx );
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), a, thd, NULL );
    uv_mutex_unlock( &thd->thd_management_mtx );
    uv_cond_signal( &thd->thd_management_cond );
    return 0;
}

static void crcl(getaddrinfo_callback)(
    uv_getaddrinfo_t* req, int status, struct addrinfo* res )
{
    activity_p a = (activity_p)req->data;
    // XXX a->io_response.addrinfo.rc   = status;
    // XXX a->io_response.addrinfo.info = res;
    /* XXX handle errors? */
    crcl(wake_up_requester)( a );
}

void crcl(io_cmd_cb)( uv_async_t *handle )
{
    /* zlog_debug( crcl(c), "IO THING\n" ); */
    crcl(io_cmd_t) *cmd;
    /* Multiple async_sends might result in a single callback call, so
     * we need to loop until the queue is empty.  (I assume it will be
     * extremely uncommon for this queue to actually grow
     * signnificantly.) */
    while( ( cmd = dequeue() ) )
    {
        int rc;
        /* XXX implement stuff */
        switch( cmd->command )
        {
        case CRCL(IO_CMD_START):
        {
            cthread_p thd = cmd->_.thread;
            // zlog_debug( crcl(c) , "Timer req recved cmd: %p thd: %p\n", cmd, thd );
            if( CRCL(CHECK_FLAG)( *thd, CRCL(THDF_TIMER_ON) ) )
            {
                /* Very weird timing, but probably possible */
                uv_timer_stop( &thd->timer_req );
            }
            rc = uv_timer_start( &thd->timer_req, the_thing, 10, 0);
            assert( !rc );
            break;
        }
        case CRCL(IO_CMD_JOIN_THREAD):
            if( crcl(join_thread)( cmd->_.thread ) )
            {
                /* zlog_debug( stderr, "Close, please\n" ); */
                /* XXX What about when there are more events???. */
                uv_close( (uv_handle_t *)handle, crcl(io_cmd_close) );
            }
            break;
        case CRCL(IO_CMD_GETADDRINFO):
        {
            int rc;
            if( ( rc = uv_getaddrinfo(crcl(io_loop),
                                      cmd->_.addrinfo.resolver,
                                      crcl(getaddrinfo_callback),
                                      cmd->_.addrinfo.node,
                                      cmd->_.addrinfo.service,
                                      cmd->_.addrinfo.hints ) ) )
            {
                activity_p a = (activity_p)cmd->_.addrinfo.resolver->data;
                // XXX a->io_response.addrinfo.rc = rc;
                crcl(wake_up_requester)( a );
            }
            else
            {
                /* it worked! */
            }
            break;
        }
        default:
            /* XXX error message? */
            exit( 1 );
        }
        free( cmd );
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
        uv_async_init( crcl(io_loop), &crcl(io_cmd), crcl(io_cmd_cb) ) );

    crcl(cmd_queue).front = NULL;
    crcl(cmd_queue).back = NULL;
    RET_IF_ERROR( uv_mutex_init( &crcl(cmd_queue).mtx ) );

    /* app_main doesn't run until the event loop is actually started */
    RET_IF_ERROR( uv_idle_init( crcl(io_loop), &start_the_world ) );
    start_the_world.data = f;
    RET_IF_ERROR( uv_idle_start( &start_the_world, start_cb ) );

    return 0;
}
