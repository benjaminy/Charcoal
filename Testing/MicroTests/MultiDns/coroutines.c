#include <pcoroutine.h>
#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

PCORO_DECL(get_one, p );

int main( int argc, char **argv, char **env )
{
    PCORO_MAIN_INIT;
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );
    pcoroutine_p coro_handles =
        (pcoutine_p)malloc( urls_to_get * sizeof(coro_handles[0]) );

    for( int i = 0; i < urls_to_get; ++i )
    {
        int idx = ( i + start_idx ) % NUM_URLs;
        pcoutine_p coro_handle = &coro_handles[i];
        const char *name = URLs[ idx ];
        int rc = pcoro_create( coro_handle, NULL, PCORO(get_one), (void *)name );
        if( rc )
        {
            printf( "Error creating coroutine %d %d %s\n", rc, idx, name );
            ++dns_error_count;
        }
    }

    for( int i = 0; i < urls_to_get; ++i )
    {
        pcoroutine_p = &coro_handles[i];
        size_t coro_return_val;
        int rc = pcoro_join( thread_handle, (void **)(&coro_return_val) );
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

    FINISH_DNS( 0 );
}

PCORO_DEF(get_one, p,
{
    const char *name = (const char *)p;
    struct addrinfo *info;
    pcoroutine_t getaddr_coro;
    size_t rc = pcoro_getaddrinfo( name, NULL, NULL, &info, &getaddr_coro );
    if( rc )
    {
        fprintf( stderr, "Error starting getaddr: %d\n", rc );
        ++dns_error_count;
        pcoro_return( rc );
    }
    void *coro_return_val;
    rc = pcoro_join( &getaddr_coro, &coro_return_val );
    if( rc )
    {
        fprintf( stderr, "Error waiting for getaddr: %d\n", rc );
        ++dns_error_count;
        pcoro_return( rc );
    }
    if( coro_return_val )
    {
        fprintf( stderr, "Error returned from getaddr: %p\n", coro_return_val );
        ++dns_error_count;
        pcoro_return( coro_return_val );
    }

    print_dns_info( name, info );
    freeaddrinfo( info );
    pcoro_return( 0 );
} )
