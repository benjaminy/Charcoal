#include <assert.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

int error_count = 0;

void get_one( int idx );

int main( int argc, char **argv, char **env )
{
    int urls_to_get, start_idx;
    get_cmd_line_args( argc, argv, &urls_to_get, &start_idx );

    for( int i = 0; i < urls_to_get; ++i )
    {
        get_one( ( i + start_idx ) % NUM_URLs );
    }

    printf( "\nERROR COUNT: %d\n", error_count );
    return 0;
}

void get_one( int idx )
{
    const char *name = URLs[ idx ];
    struct addrinfo *info;
    int rc = getaddrinfo( name, NULL, NULL, &info );
    if( rc )
    {
        printf( "ERROR ERROR ERROR %d %d %s\n", rc, idx, name );
        ++error_count;
        return;
    }
    print_dns_info( name, info );
    freeaddrinfo( info );
}
