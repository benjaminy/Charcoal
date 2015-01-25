#include <assert.h>
#include <stdio.h>
#include <multi_dns_utils.h>

void get_one( int idx );

int main( int argc, char **argv, char **env )
{
    int urls_to_get;
    get_cmd_line_args( argc, argv, &urls_to_get );

    for( int i = 0; i < urls_to_get; ++i )
    {
        if( i == 3 )
        {
            sleep( 3 );
        }
        get_one( i );
    }

    FINISH_DNS( 0 );
}

void get_one( int idx )
{
    const char *name = pick_name( idx );
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
