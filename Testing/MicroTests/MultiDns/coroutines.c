#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

typedef struct coroutine coroutine, *coroutine_p;

struct coroutine
{
};

int coroutine_start( coroutine_p coro, void *(*f)( coroutine_p, void * ), void * )
{
    
}

int error_count = 0;

void *getaddrinfo_entry( void *p )
{
    const char *name = (const char *)p;
    struct addrinfo *info;
    int rc = getaddrinfo( name, NULL, NULL, &info );
    if( !rc )
    {
        print_dns_info( name, info );
    }
    freeaddrinfo( info );
    return (void *)((long)rc);
}

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );
    pthread_t *thread_handles =
        (pthread_t *)malloc( urls_to_get * sizeof(thread_handles[0]) );
    for( int i = 0; i < urls_to_get; ++i )
    {
        int idx = ( i + start_idx ) % NUM_URLs;
        const char *name = URLs[ idx ];
        int rc = pthread_create( &thread_handles[i], NULL, getaddrinfo_entry, (void *)name );
        if( rc )
        {
            printf( "ERROR IN THREAD CREATION %d %d %s\n", rc, idx, name );
            ++error_count;
        }
    }
    printf( "\nErrors in thread creation? %d\n", error_count );

    for( int i = 0; i < urls_to_get; ++i )
    {
        int thread_return;
        int rc = pthread_join( thread_handles[i], (void **)(&thread_return) );
        if( rc )
        {
            printf( "ERROR IN THREAD JOIN %d\n", rc );
            ++error_count;
        }
        else if( thread_return )
        {
            printf( "ERROR CODE FROM THREAD %d\n", thread_return );
            ++error_count;
        }
    }
    printf( "\nERROR COUNT: %d\n", error_count );
    return 0;
}
