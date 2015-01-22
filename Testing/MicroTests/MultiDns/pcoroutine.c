#include <pcoroutine.h>

pcoroutine_p __pcoro_self_global;

int pcoro_create(
    pcoroutine_p coro,
    void *(*f)( pcoroutine_p, void * ),
    void *options,
    void *p )
{
    return 0;
}

int pcoro_join( pcoroutine_p c, void **res )
{
    return 0;
}

int pcoro_yield( pcoroutine_p ctx )
{
    return 0;
}

static void *getaddrinfo_in_thread( void *p )
{
    
}

int pcoro_getaddrinfo(
    const char *hostname,
    const char *servname,
    const struct addrinfo *hints,
    struct addrinfo **res,
    pcoroutine_p coro )
{
    return pcoro_create();
}

int pcoro_wait_either( pcoroutine_p a, pcoroutine_p b, pcoroutine_p either )
{
    return 0;
}

