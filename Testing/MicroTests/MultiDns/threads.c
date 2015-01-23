#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

void launch_one( int idx, pthread_t *thread_handle );
void *get_one( void *p );
void wait_one( pthread_t thread_handle );

const char *slow_one;

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );
    pthread_t *thread_handles =
        (pthread_t *)malloc( urls_to_get * sizeof(thread_handles[0]) );

    slow_one = "";
    // slow_one = URLs[ ( 3 + start_idx ) % NUM_URLs ];

    for( int i = 0; i < urls_to_get; ++i )
    {
        launch_one( ( i + start_idx ) % NUM_URLs, &thread_handles[i] );
    }

    for( int i = 0; i < urls_to_get; ++i )
    {
        wait_one( thread_handles[i] );
    }

    FINISH_DNS( 0 );
}

void launch_one( int idx, pthread_t *thread_handle )
{
    const char *name = URLs[ idx ];
    int rc = pthread_create( thread_handle, NULL, get_one, (void *)name );
    if( rc )
    {
        printf( "Error creating thread %d %d %s\n", rc, idx, name );
        ++dns_error_count;
    }
}

void *get_one( void *p )
{
    const char *name = (const char *)p;
    if( !strcmp( name, slow_one ) )
    {
        sleep( 3 );
    }
    struct addrinfo *info;
    int rc = getaddrinfo( name, NULL, NULL, &info );
    if( !rc )
    {
        print_dns_info( name, info );
        freeaddrinfo( info );
    }
    return (void *)((size_t)rc);
}

void wait_one( pthread_t thread_handle )
{
    size_t thread_return;
    int rc = pthread_join( thread_handle, (void **)(&thread_return) );
    if( rc )
    {
        printf( "Error joining thread %p %d\n", thread_handle, rc );
        ++dns_error_count;
    }
    else if( thread_return )
    {
        printf( "Error returned by thread %d\n", (int)thread_return );
        ++dns_error_count;
    }
}
