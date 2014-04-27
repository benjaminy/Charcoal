#ifndef __CHARCOAL_RUNTIME_IO_COMMANDS
#define __CHARCOAL_RUNTIME_IO_COMMANDS

#include <charcoal_runtime.h>

extern uv_loop_t *crcl(io_loop);
extern uv_async_t crcl(io_cmd);

typedef enum
{
    CRCL(IO_CMD_START),
    CRCL(IO_CMD_JOIN_THREAD),
    CRCL(IO_CMD_GETADDRINFO),
} crcl(io_cmd_op);

typedef struct crcl(io_cmd_t) crcl(io_cmd_t);

struct crcl(io_cmd_t)
{
    crcl(io_cmd_op) command;
    /* XXX Not _every_ command needs an activity, but I guess most will */
    crcl(activity_t) *activity;
    union
    {
        crcl(thread_t) *thread;
        struct
        {
            uv_getaddrinfo_t *resolver;
            const char *node;
            const char *service;
            const struct addrinfo *hints;
        } addrinfo;
    } _;
    uint64_t time_ns;
    __charcoal_io_cmd_t *next;
};

void enqueue( __charcoal_io_cmd_t *cmd );

int dequeue( __charcoal_io_cmd_t *cmd_ref );

void crcl(io_cmd_cb)( uv_async_t *handle, int status /*UNUSED*/ );

#endif /* __CHARCOAL_RUNTIME_IO_COMMANDS */
