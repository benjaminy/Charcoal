#include <assert.h>
#include <charcoal.h>
#include <stdio.h>
#include <stdlib.h>
#include <charcoal_runtime_io_commands.h>
#include <charcoal_runtime_coroutine.h>

uv_loop_t *crcl(io_loop);
uv_async_t crcl(io_cmd);

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
 * Removes an item from the command queue.  Returns 0 on success and 1
 * if the queue is empty.
 */
int dequeue( crcl(io_cmd_t) *cmd_ref )
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
        *cmd_ref = *crcl(cmd_queue).front;
        crcl(cmd_queue).front = crcl(cmd_queue).front->next;
        return 0;
    }
    else
    {
        return 1;
    }
}

void the_thing( uv_timer_t* handle, int status )
{
}

static void crcl(io_cmd_close)( uv_handle_t *h )
{
    /* uv_async_t *a = (uv_async_t *)h; */
    /* printf( "CLOSE %p\n", a ); fflush(stdout); */
}

static int crcl(wake_up_requester)( activity_t *a )
{
    a->flags &= ~CRCL(ACTF_BLOCKED);
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
    activity_t *a = (activity_t *)req->data;
    a->io_response.addrinfo.rc   = status;
    a->io_response.addrinfo.info = res;
    /* XXX handle errors? */
    crcl(wake_up_requester)( a );
}

void crcl(io_cmd_cb)( uv_async_t *handle, int status /*UNUSED*/ )
{
    /* fprintf( stderr, "IO THING\n" ); */
    crcl(io_cmd_t) cmd;
    /* Multiple async_sends might result in a single callback call, so
     * we need to loop until the queue is empty.  (I assume it will be
     * extremely uncommon for this queue to actually grow
     * signnificantly.) */
    while( !dequeue( &cmd ) )
    {
        /* XXX implement stuff */
        switch( cmd.command )
        {
        case CRCL(IO_CMD_START):
            /* XXX this should go in the start_resume function */
            uv_timer_start( &cmd.activity->thread->timer_req,
                            the_thing, 5000, 2000);
            break;
        case CRCL(IO_CMD_JOIN_THREAD):
            if( crcl(join_thread)( cmd._.thread ) )
            {
                /* printf( "Close, please\n" ); */
                /* XXX What about when there are more events???. */
                uv_close( (uv_handle_t *)handle, crcl(io_cmd_close) );
            }
            break;
        case CRCL(IO_CMD_GETADDRINFO):
        {
            int rc;
            if( ( rc = uv_getaddrinfo(crcl(io_loop),
                                      cmd._.addrinfo.resolver,
                                      crcl(getaddrinfo_callback),
                                      cmd._.addrinfo.node,
                                      cmd._.addrinfo.service,
                                      cmd._.addrinfo.hints ) ) )
            {
                activity_t *a = (activity_t *)cmd._.addrinfo.resolver->data;
                a->io_response.addrinfo.rc = rc;
                crcl(wake_up_requester)( a );
            }
            else
            {
                /* it worked! */
            }
            break;
        }
        default:
            exit( 1 );
        }
    }
}

/*
 * The initial thread that exists when a Charcoal program starts will
 * be the I/O thread.  Before it starts its I/O duties it launches
 * another thread that will call the application's main procedure.
 */
int crcl(init_io_loop)( cthread_p t, activity_p a )
{
    crcl(threads) = t;

    RET_IF_ERROR(
        pthread_key_create( &crcl(self_key), NULL /* XXX destructor */ ) );

    /* Most of the thread and activity fields are not relevant to the I/O thread */
    t->activities     = a;
    t->ready          = NULL;
    t->next           = crcl(threads);
    t->prev           = crcl(threads);
    /* XXX Not sure about the types of self: */
    // XXX t->sys            = uv_thread_self();
    a->thread         = t;

    crcl(io_loop) = uv_default_loop();
    RET_IF_ERROR(
        uv_async_init( crcl(io_loop), &crcl(io_cmd), crcl(io_cmd_cb) ) );

    crcl(cmd_queue).front = NULL;
    crcl(cmd_queue).back = NULL;
    assert( !uv_mutex_init( &crcl(cmd_queue).mtx ) );

    return 0;
}
