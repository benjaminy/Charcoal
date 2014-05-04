#include <charcoal_base.h>
#include <stdio.h>
#include <stdlib.h>
#include <charcoal_runtime_io_commands.h>
#include <charcoal_runtime.h>

uv_loop_t *crcl(io_loop);
uv_async_t crcl(io_cmd);

typedef struct
{
    __charcoal_io_cmd_t *front, *back;
    uv_mutex_t mtx;
} __charcoal_cmd_queue_t;

static __charcoal_cmd_queue_t __charcoal_cmd_queue;

void enqueue( crcl(io_cmd_t) *cmd )
{
    /* The queue takes ownership of *cmd */
    uv_mutex_lock( &__charcoal_cmd_queue.mtx );
    cmd->next = __charcoal_cmd_queue.back;
    __charcoal_cmd_queue.back = cmd;
    uv_mutex_unlock( &__charcoal_cmd_queue.mtx );
}

/*
 * Removes an item from the command queue.  Returns 0 on success and 1
 * if the queue is empty.
 */
int dequeue( crcl(io_cmd_t) *cmd_ref )
{
    if( !__charcoal_cmd_queue.front )
    {
        uv_mutex_lock( &__charcoal_cmd_queue.mtx );
        while( __charcoal_cmd_queue.back )
        {
            __charcoal_io_cmd_t *tmp = __charcoal_cmd_queue.back->next;
            __charcoal_cmd_queue.back->next = __charcoal_cmd_queue.front;
            __charcoal_cmd_queue.front = __charcoal_cmd_queue.back;
            __charcoal_cmd_queue.back = tmp;
        }
        uv_mutex_unlock( &__charcoal_cmd_queue.mtx );
    }
    if( __charcoal_cmd_queue.front )
    {
        *cmd_ref = *__charcoal_cmd_queue.front;
        __charcoal_cmd_queue.front = __charcoal_cmd_queue.front->next;
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

static int crcl(wake_up_requester)( crcl(activity_t) *a )
{
    a->flags &= ~CRCL(ACTF_BLOCKED);
    crcl(thread_t) *thd = a->container;
    RET_IF_ERROR( pthread_mutex_lock( &thd->thd_management_mtx ) );
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), a, thd, NULL );
    RET_IF_ERROR( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    RET_IF_ERROR( pthread_cond_signal( &thd->thd_management_cond ) );
    return 0;
}

static void crcl(getaddrinfo_callback)(
    uv_getaddrinfo_t* req, int status, struct addrinfo* res )
{
    crcl(activity_t) *a = (crcl(activity_t) *)req->data;
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
            uv_timer_start( &cmd.activity->container->timer_req,
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
                crcl(activity_t) *a = (crcl(activity_t) *)cmd._.addrinfo.resolver->data;
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

