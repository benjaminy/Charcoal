#ifndef __CHARCOAL_RUNTIME_IO_COMMANDS
#define __CHARCOAL_RUNTIME_IO_COMMANDS

#include <charcoal_runtime.h>

extern uv_loop_t *CRCL(io_loop);
extern uv_async_t CRCL(io_cmd);

typedef enum
{
    __CRCL_IO_CMD_START,
    __CRCL_IO_CMD_JOIN_THREAD,
    __CRCL_IO_CMD_GETADDRINFO,
} CRCL(io_cmd_op);

typedef struct CRCL(io_cmd_t) CRCL(io_cmd_t);

struct CRCL(io_cmd_t)
{
    CRCL(io_cmd_op) command;
    /* XXX Not _every_ command needs an activity, but I guess most will */
    CRCL(activity_t) *activity;
    union
    {
        CRCL(thread_t) *thread;
        struct
        {
            uv_getaddrinfo_t *resolver;
        } addrinfo;
    } _;
    uint64_t time_ns;
    __charcoal_io_cmd_t *next;
};

void enqueue( __charcoal_io_cmd_t *cmd );

int dequeue( __charcoal_io_cmd_t *cmd_ref );

void CRCL(io_cmd_cb)( uv_async_t *handle, int status /*UNUSED*/ );

#endif /* __CHARCOAL_RUNTIME_IO_COMMANDS */
