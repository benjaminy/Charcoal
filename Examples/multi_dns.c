#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void multi_dns_seq( size_t n, char **names, struct addrinfo **infos )
{
    size_t i;
    for( i = 0; i < n; ++i )
    {
        struct addrinfo *p;
        assert( 0 == getaddrinfo( names[i], NULL, NULL, &infos[i] ) );
        for( p = infos[i]; p != NULL; p = p->ai_next )
        {
            struct sockaddr *addr = p->ai_addr;
            size_t j, ds = sizeof( addr->sa_data ) / sizeof( addr->sa_data[0] );
            for( j = 0; j < ds; ++j )
            {
                unsigned int x = (unsigned char)addr->sa_data[j];
                printf( " %u", x );
            }
            printf( "\n" );
        }
    }
}

int main( int argc, char **argv )
{
    char *names = "google.com";
    struct addrinfo *infos;
    multi_dns_seq( 1, &names, &infos );
    return 0;
}

#if 0

void multi_dns_seq( size_t n, char **names, struct addrinfo **infos )
{
    size_t i;
    for( i = 0; i < n; ++i )
    {
        assert( 0 == getaddrinfo( names[i], NULL, NULL, &infos[i] ) );
    }
}

void multi_dns_conc( size_t n, char **names, struct addrinfo **infos )
{
    size_t i, done = 0;
    semaphore done_sem;
    sem_init( &done_sem, 0 );
    for( i = 0; i < n; ++i )
    {
        activate ( i )
        {
            assert( 0 == getaddrinfo( names[i], NULL, NULL, &infos[i] ) );
            ++done;
            if( done == n )
                sem_inc( &done_sem );
        }
    }
    sem_dec( &done_sem );
}

#define DEFAULT_MULTI_DNS_LIMIT 10

void multi_dns_conc_lim( size_t n, size_t lim, char **names, struct addrinfo **infos )
{
    size_t i, done = 0;
    semaphore done_sem, lim_sem;
    sem_init( &done_sem, 0 );
    sem_init( &lim_sem, lim < 1 ? DEFAULT_MULTI_DNS_LIMIT : lim );
    for( i = 0; i < n; ++i )
    {
        sem_dec( &lim_sem );
        activate ( i )
        {
            assert( 0 == getaddrinfo( names[i], NULL, NULL, &infos[i] ) );
            sem_inc( &lim_sem );
            ++done;
            if( done == n )
                sem_inc( &done_sem );
        }
    }
    sem_dec( &done_sem );
}

void multi_dns_conc_lim_nonblock(
    size_t n, size_t lim, char **names, struct addrinfo **infos, activity *dns_act )
{
    if( dns_act )
    {
        activate *dns_act = ( n, lim, names, infos )
        {
            multi_dns_conc_lim( n, lim, names, infos );
        }
    }
    else
    {
        multi_dns_conc_lim( n, lim, names, infos );
    }
}

void client_code()
{
    active_call dns_act = multi_dns_conc_lim( n, lim, names, infos );
}

#endif
