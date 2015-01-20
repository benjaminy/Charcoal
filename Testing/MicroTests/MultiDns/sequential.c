#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

void get_one( int idx );

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );

    for( int i = 0; i < urls_to_get; ++i )
    {
        get_one( ( i + start_idx ) % NUM_URLs );
    }

    FINISH_DNS( 0 );
}

void get_one( int idx )
{
    const char *name = URLs[ idx ];
    struct addrinfo *info;

    int rc = getaddrinfo( name, NULL, NULL, &info );
    if( rc )
    {
        printf( "Error %d %d %s\n", rc, idx, name );
        ++dns_error_count;
        return;
    }
    print_dns_info( name, info );
    freeaddrinfo( info );
}
