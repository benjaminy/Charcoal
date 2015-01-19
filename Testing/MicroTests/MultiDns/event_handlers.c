#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>
#include <uv.h>

void register_one( int idx, uv_loop_t *dispatcher, uv_getaddrinfo_t *resolvers );
void got_one( uv_getaddrinfo_t *resolver, int status, struct addrinfo *res );

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );
    uv_loop_t *dispatcher = uv_default_loop();
    uv_getaddrinfo_t *resolvers =
        (uv_getaddrinfo_t *)malloc( urls_to_get * sizeof(resolvers[0]) );

    for( int i = 0; i < urls_to_get; ++i )
    {
        register_one( ( i + start_idx ) % NUM_URLs, dispatcher, &resolvers[i] );
    }

    int rc = uv_run( dispatcher, UV_RUN_DEFAULT );
    FINISH_DNS( rc );
}

void register_one( int idx, uv_loop_t *dispatcher, uv_getaddrinfo_t *resolver )
{
    resolver->data = (void *)URLs[ idx ];
    int rc = uv_getaddrinfo(
        dispatcher, resolver, got_one, URLs[ idx ], NULL, &hints );

    if( rc )
    {
        fprintf( stderr, "Error registering callback: %s\n", uv_err_name( rc ) );
        ++dns_error_count;
    }
}

void got_one( uv_getaddrinfo_t *resolver, int status, struct addrinfo *res )
{
    if( status )
    {
        fprintf( stderr, "Error passed to callback %s\n", uv_err_name( status ) );
        ++dns_error_count;
        return;
    }
    const char *name = (const char *)resolver->data;
    print_dns_info( name, res );
    uv_freeaddrinfo(res);
}
