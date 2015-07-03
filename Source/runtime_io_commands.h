#ifndef __CHARCOAL_RUNTIME_IO_COMMANDS
#define __CHARCOAL_RUNTIME_IO_COMMANDS

#include <core_runtime.h>

extern uv_loop_t *crcl(io_loop);
extern uv_async_t crcl(io_cmd);

union crcl(io_response_t)
{
    struct
    {
        int rc;
        struct addrinfo *info;
    } addrinfo;
};

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
    activity_p activity;
    union
    {
        cthread_p thread;
        struct
        {
            uv_getaddrinfo_t *resolver;
            const char *node;
            const char *service;
            const struct addrinfo *hints;
        } addrinfo;
    } _;
    uint64_t time_ns;
    crcl(io_cmd_t) *next;
};

void enqueue( crcl(io_cmd_t) *cmd );

crcl(io_cmd_t) *dequeue( void );

void crcl(io_cmd_cb)( uv_async_t *handle );

int crcl(init_io_loop)( cthread_p t, activity_p a, int (*)( void ) );

#endif /* __CHARCOAL_RUNTIME_IO_COMMANDS */
