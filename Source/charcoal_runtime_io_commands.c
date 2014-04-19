#include <charcoal_base.h>
#include <stdio.h>
#include <charcoal_runtime_io_commands.h>
#include <charcoal_runtime.h>

uv_loop_t *__charcoal_io_loop;
uv_async_t __charcoal_io_cmd;

typedef struct
{
    __charcoal_io_cmd_t *front, *back;
    uv_mutex_t mtx;
} __charcoal_cmd_queue_t;

static __charcoal_cmd_queue_t __charcoal_cmd_queue;

void enqueue( __charcoal_io_cmd_t *cmd )
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
int dequeue( __charcoal_io_cmd_t *cmd_ref )
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

void CRCL(io_cmd_cb)( uv_async_t *handle, int status /*UNUSED*/ )
{
    fprintf( stderr, "IO THING\n" );
    CRCL(io_cmd_t) cmd;
    /* Multiple async_sends might result in a single callback call, so
     * we need to loop until the queue is empty.  (I assume that this
     * queue actually growing will be an extremely uncommon
     * situation.) */
    while( !dequeue( &cmd ) )
    {
        /* XXX implement stuff */
        switch( cmd.command )
        {
        case __CRCL_IO_CMD_START:
            uv_timer_start( &cmd.sender->container->timer_req, the_thing, 5000, 2000);

        }
    }
}

