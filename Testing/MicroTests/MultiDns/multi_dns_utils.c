#include <stdlib.h>
#include <stdio.h>
#include <multi_dns_utils.h>

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

void get_cmd_line_args( int argc, char **argv, int *urls, int *idx )
{
    *urls = DEFAULT_URLS_TO_GET;
    *idx  = DEFAULT_START_IDX;
    if( argc > 1 )
    {
        *idx = strtol( argv[ 1 ], NULL, 10 );
        if( *idx < 0 )
            *idx = 0;
    }
    if( argc > 2 )
    {
        *urls = strtol( argv[ 2 ], NULL, 10 );
        if( *urls < 1 )
            *urls = 1;
    }
}
