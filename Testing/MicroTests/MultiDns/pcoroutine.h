#include <pthread.h>
#include <netdb.h>

typedef struct pcoroutine_t pcoroutine_t, *pcoroutine_p;

struct pcoroutine_t
{
    pthread_t tid;
};

struct __pcoro_start_record
{
    pcoroutine_p coro;
    void *param;
};

extern pcoroutine_p __pcoro_self_global;

#define PCORO_DECL(coro_name, param_name) \
    void *__pcoro_##coro_name( void *__pcoro_param );

#define PCORO(coro_name) __pcoro_##coro_name

#define PCORO_DEF(coro_name, param_type, param_name, body)       \
    void *__pcoro_##coro_name( void *__pcoro_param ) \
    { \
        pcoroutine_p __pcoro_self; \
        param_type param_name; \
        { \
            struct __pcoro_start_record *s = (struct __pcoro_start_record *)__pcoro_param; \
            param_name = ((param_type)(s->param));                      \
            __pcoro_self = s->coro; \
            __pcoro_self_global = __pcoro_self; \
        } \
        void *__pcoro_return_value = 0; \
        { body } \
        __pcoro_after_body_label: \
        /* set status and release waiters */; \
        return __pcoro_return_value; \
    }

#define pcoro_return(rv) \
    do { \
        __pcoro_return_value = ((void *)(rv)); \
        goto __pcoro_after_body_label; \
    } while( 0 )

#define PCORO_MAIN_INIT \
    pcoroutine_p __pcoro_self; \
    do { \
        pcoroutine_t main_coro; \
        __pcoro_self = &main_coro; \
        __pcoro_self_global = __pcoro_self; \
    } while( 0 )

int pcoroutine_init( void );

int pcoro_create(
    pcoroutine_p coro,
    void *(*f)( pcoroutine_p, void * ),
    void *options,
    void *p );

int pcoro_join( pcoroutine_p c, void **res );

int pcoro_getaddrinfo(
    const char *hostname,
    const char *servname,
    const struct addrinfo *hints,
    struct addrinfo **res,
    pcoroutine_p coro );

int yield( pcoroutine_p ctx );
