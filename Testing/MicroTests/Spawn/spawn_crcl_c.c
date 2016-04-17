#include <charcoal_base.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <charcoal_std_lib.h>

#define N 10
int m = N, i = 0;
crcl(activity_t) acts[2];
semaphore_t done_sem;

void f( void *p )
{
    if( i > 0 )
    {
        crcl(activity_join)( &acts[ ( i + 1 ) % 2 ], NULL );
    }
    ++i;
    if( i < m )
    {
        crcl(activate)( &acts[ i % 2 ], f, NULL );
    }
    else
    {
        semaphore_incr( &done_sem );
    }
}

int crcl(application_main)( int argc, char **argv )
{
    printf( "Hello World\n" );
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    semaphore_open( &done_sem, 0 );
    printf( "ActivateA 0\n" );
    crcl(activate)( &acts[0], f, NULL );
    semaphore_decr( &done_sem );
    semaphore_close( &done_sem );
    printf( "%d\n", i );
    return 0;
}
