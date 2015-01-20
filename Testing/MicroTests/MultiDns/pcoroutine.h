#include <pthread.h>

typedef struct coroutine coroutine, *coroutine_p;

struct coroutine
{
    pthread_t tid;
};

struct __pcoro_start_record
{
    coroutine_p coro;
    void *param;
};

extern coroutine_p __pcoro_self_global;

#define PCORO_DECL(coro_name, param_name) \
    void *__pcoro_##coro_name( void *__pcoro_param );

#define PCORO(coro_name) __pcoro_##coro_name

#define PCORO_DEF(coro_name, param_name, body)       \
    void *__pcoro_##coro_name( void *__pcoro_param ) \
    { \
        struct __pcoro_start_record *s = (__pcoro_start_record *)__pcoro_param; \
        __pcoro_self = s->coro; \
        __pcoro_self_global = __pcoro_self; \
        void *__pcoro_return_value = 0;
        param_name = s->param; \
        { body } \
        __pcoro_after_body_label: \
        pcoroutine_end; \
    }

#define pcoro_return(rv) \
    do { \
        __pcoro_return_value = ((void *)(rv)); \
        goto __pcoro_after_body_label; \
    } while( 0 )

int pcoroutine_init( void )
{
}

int coroutine_start( coroutine_p coro, void *(*f)( coroutine_p, void * ), void * )
{
    
}

int yield( coroutine_p ctx )
{
}

