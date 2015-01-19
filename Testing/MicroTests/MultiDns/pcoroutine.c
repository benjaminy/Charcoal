
launch
typedef struct coroutine coroutine, *coroutine_p;

struct coroutine
{
};

int pcoro_start( coroutine_p coro, void *(*f)( coroutine_p, void * ), void * )
{
    
}

int yield( coroutine_p ctx )
{
}

int pcoro_getaddrinfo(
    const char *hostname, const char *servname, const struct addrinfo *hints,
    struct addrinfo **res, pcoroutine_p coro )
{
}

int pcoro_wait_either( pcoroutine_p a, pcoroutine_p b, pcoroutine_p either )
{
}

