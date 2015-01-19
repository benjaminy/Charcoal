#include <pthread.h>

typedef struct coroutine coroutine, *coroutine_p;

struct coroutine
{
    pthread_t tid;
};

int pcoroutine_init( void )
{
}

int coroutine_start( coroutine_p coro, void *(*f)( coroutine_p, void * ), void * )
{
    
}

int yield( coroutine_p ctx )
{
}

