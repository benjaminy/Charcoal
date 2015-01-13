#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>
#include <uv.h>

int error_count = 0;

void got_one( uv_getaddrinfo_t *resolver, int status, struct addrinfo *res )
{
    if( status )
    {
        fprintf( stderr, "Error passed to callback %s\n", uv_err_name( status ) );
        ++error_count;
        return;
    }
    const char *name = (const char *)resolver->data;
    print_dns_info( name, res );
    uv_freeaddrinfo(res);
}

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );
    uv_loop_t *loop = uv_default_loop();
    uv_getaddrinfo_t *resolvers =
        (uv_getaddrinfo_t *)malloc( urls_to_get * sizeof(resolvers[0]) );

    struct addrinfo hints;
    hints.ai_family   = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = 0;

    for( int i = 0; i < urls_to_get; ++i )
    {
        int idx = ( i + start_idx ) % NUM_URLs;
        resolvers[i].data = (void *)URLs[ idx ];
        int rc = uv_getaddrinfo(
            loop, &resolvers[i], got_one, URLs[ idx ], NULL, &hints );

        if( rc )
        {
            fprintf( stderr, "Error registering callback: %s\n", uv_err_name( rc ) );
            ++error_count;
        }
    }
    printf( "\nERROR COUNT: %d\n", error_count );
    return uv_run(loop, UV_RUN_DEFAULT);
}
