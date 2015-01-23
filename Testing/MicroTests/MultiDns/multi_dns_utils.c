#include <stdlib.h>
#include <stdio.h>
#include <urls.h>
#include <multi_dns_utils.h>

int dns_error_count;

struct addrinfo hints;

void print_dns_info( const char *name, struct addrinfo *info )
{
    printf( "%20s %20s: %x\n", name, "flags",     info->ai_flags );
    printf( "%20s %20s: %d\n", name, "family",    info->ai_family );
    printf( "%20s %20s: %d\n", name, "socktype",  info->ai_socktype );
    printf( "%20s %20s: %d\n", name, "protocol",  info->ai_protocol );
    printf( "%20s %20s: %d\n", name, "addrlen",   info->ai_addrlen ); /* socklen_t ??? */
    printf( "%20s %20s: %p\n", name, "sockaddr",  info->ai_addr ); /* struct sockaddr * ??? */
    printf( "%20s %20s: %s\n", name, "canonname", info->ai_canonname );
    printf( "%20s %20s: %p\n", name, "next",      info->ai_next );
}

static int start_idx;

void get_cmd_line_args( int argc, char **argv, int *urls )
{
    hints.ai_family   = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = 0;

    *urls = DEFAULT_URLS_TO_GET;
    start_idx  = DEFAULT_START_IDX;
    if( argc > 1 )
    {
        start_idx = strtol( argv[ 1 ], NULL, 10 );
        if( start_idx < 0 )
            start_idx = 0;
    }
    if( argc > 2 )
    {
        *urls = strtol( argv[ 2 ], NULL, 10 );
        if( *urls < 1 )
            *urls = 1;
    }
}

const char *pick_name( int idx )
{
    return URLs[ ( idx + start_idx ) % NUM_URLs ];
}
