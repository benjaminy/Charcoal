#include <pcoroutine.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <multi_dns_utils.h>

PCORO_DECL(get_one, p );

int main( int argc, char **argv, char **env )
{
    PCORO_MAIN_INIT;
    int urls_to_get;
    get_cmd_line_args( argc, argv, &urls_to_get );
    pcoroutine_p coro_handles =
        (pcoroutine_p)malloc( urls_to_get * sizeof(coro_handles[0]) );

    for( int i = 0; i < urls_to_get; ++i )
    {
        pcoroutine_p coro_handle = &coro_handles[i];
        const char *name = pick_name( i );
        int rc = pcoro_create( coro_handle, NULL, PCORO(get_one), (void *)name );
        if( rc )
        {
            printf( "Error creating coroutine %d %d %s\n", rc, i, name );
            ++dns_error_count;
        }
    }

    for( int i = 0; i < urls_to_get; ++i )
    {
        pcoroutine_p coro_handle = &coro_handles[i];
        size_t coro_return_val;
        int rc = pcoro_join( coro_handle, (void **)(&coro_return_val) );
        if( rc )
        {
            printf( "Error joining coro %p %d\n", coro_handle, rc );
            ++dns_error_count;
        }
        else if( coro_return_val )
        {
            printf( "Error returned by coro %d\n", (int)coro_return_val );
            ++dns_error_count;
        }
    }

    FINISH_DNS( 0 );
}

PCORO_DEF(get_one, const char *, name,
{
    struct addrinfo *info;
    pcoroutine_t getaddr_coro;
    size_t rc = (size_t)pcoro_getaddrinfo( name, NULL, NULL, &info, &getaddr_coro );
    if( rc )
    {
        fprintf( stderr, "Error starting getaddr: %ld\n", rc );
        ++dns_error_count;
        pcoro_return( rc );
    }
    void *coro_return_val;
    rc = pcoro_join( &getaddr_coro, &coro_return_val );
    if( rc )
    {
        fprintf( stderr, "Error waiting for getaddr: %ld\n", rc );
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
