#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <multi_dns_utils.h>
#include <uv.h>

uv_getaddrinfo_t *slow_one;

void register_one( int idx, uv_loop_t *dispatcher, uv_getaddrinfo_t *resolvers );
void got_one( uv_getaddrinfo_t *resolver, int status, struct addrinfo *res );

int main( int argc, char **argv, char **env )
{
    int urls_to_get;
    get_cmd_line_args( argc, argv, &urls_to_get );
    uv_loop_t *dispatcher = uv_default_loop();
    uv_getaddrinfo_t *resolvers =
        (uv_getaddrinfo_t *)malloc( urls_to_get * sizeof(resolvers[0]) );

    // slow_one = NULL;
    slow_one = &resolvers[3];

    for( int i = 0; i < urls_to_get; ++i )
    {
        register_one( i, dispatcher, &resolvers[i] );
    }

    int rc = uv_run( dispatcher, UV_RUN_DEFAULT );
    FINISH_DNS( rc );
}

void register_one( int idx, uv_loop_t *dispatcher, uv_getaddrinfo_t *resolver )
{
    resolver->data = (void *)pick_name( idx );
    int rc = uv_getaddrinfo(
        dispatcher, resolver, got_one, pick_name( idx ), NULL, &hints );

    if( rc )
    {
        fprintf( stderr, "Error registering callback: %d\n", rc );
        ++dns_error_count;
    }
}

void got_one( uv_getaddrinfo_t *resolver, int rc, struct addrinfo *res )
{
    if( slow_one == resolver )
    {
        sleep( 4 );
    }
    if( rc )
    {
        fprintf( stderr, "Error passed to callback. %d\n", rc );
        ++dns_error_count;
        return;
    }
    const char *name = (const char *)resolver->data;
    print_dns_info( name, res );
    uv_freeaddrinfo(res);
}
