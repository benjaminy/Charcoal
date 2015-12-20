#ifndef __CHARCOAL_RUNTIME_IO_COMMANDS
#define __CHARCOAL_RUNTIME_IO_COMMANDS

#include <core_runtime.h>

extern uv_async_t crcl(io_cmd);

void       crcl(async_fn_start)( uv_loop_t *, uv_handle_t *, crcl(async_call_p) );
void crcl(async_fn_join_thread)( uv_loop_t *, uv_handle_t *, crcl(async_call_p) );
// void crcl(async_fn_getaddrinfo)( uv_loop_t *, uv_handle_t *, crcl(async_call_p) );
// void       crcl(async_fn_sleep)( uv_loop_t *, uv_handle_t *, crcl(async_call_p) );

/* typedef struct crcl(io_cmd_t) crcl(io_cmd_t), *crcl(io_cmd_p); */

/* struct crcl(io_cmd_t) */
/* { */
/*     crcl(io_cmd_op) command; */
/*     /\* XXX Not _every_ command needs an activity, but I guess most will *\/ */
/*     activity_p activity, waiters; */
/*     union */
/*     { */
/*         cthread_p thread; */
/*         struct */
/*         { */
/*             uv_getaddrinfo_t *resolver; */
/*             const char *node; */
/*             const char *service; */
/*             const struct addrinfo *hints; */
/*             struct addrinfo **res; */
/*             int rc; */
/*         } addrinfo; */
/*         struct */
/*         { */
/*             uv_timer_t   *timer; */
/*             unsigned int seconds; */
/*             unsigned int remaining; */
/*         } sleep; */
/*     } _; */
/*     crcl(io_cmd_p) next; */
/* }; */

int crcl(init_io_loop)( int (*)( void ) );

#endif /* __CHARCOAL_RUNTIME_IO_COMMANDS */
