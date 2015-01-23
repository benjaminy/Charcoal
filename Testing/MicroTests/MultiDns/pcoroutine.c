#include <pcoroutine.h>
#include <stdlib.h>

pcoroutine_p __pcoro_self_global;

int pcoro_create(
    pcoroutine_p coro,
    void *(*f)( pcoroutine_p, void * ),
    void *options,
    void *p )
{
    return 0;
    // return pthread_create( &coro->tid, NULL, getaddrinfo_in_thread, p );
}

int pcoro_join( pcoroutine_p c, void **res )
{
    return 0;
}

int pcoro_yield( pcoroutine_p ctx )
{
    return 0;
}

struct __pcoro_getaddrinfo_args
{
    const char *hostname;
    const char *servname;
    const struct addrinfo *hints;
    struct addrinfo **res;
};

static void *getaddrinfo_in_thread( void *p )
{
    struct __pcoro_getaddrinfo_args *args = (struct __pcoro_getaddrinfo_args *)p;
    size_t rc = (size_t)getaddrinfo( args->hostname, args->servname, args->hints, args->res );
    free( args );
    return ((void *)rc);
}

int pcoro_getaddrinfo(
    const char *hostname,
    const char *servname,
    const struct addrinfo *hints,
    struct addrinfo **res,
    pcoroutine_p coro )
{
    struct __pcoro_getaddrinfo_args *p =
        (struct __pcoro_getaddrinfo_args*)malloc( sizeof(p[0]) );
    p->hostname = hostname;
    p->servname = servname;
    p->hints = hints;
    p->res = res;
    return pthread_create( &coro->tid, NULL, getaddrinfo_in_thread, p );
}

int pcoro_wait_either( pcoroutine_p a, pcoroutine_p b, pcoroutine_p either )
{
    return 0;
}

