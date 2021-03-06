#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define N 10
int num_switches = N;
pthread_mutex_t mtx;
pthread_cond_t cond;
pthread_t threads[2];
int i, done = 0;

void *f( void *p )
{
    if( i > 1 )
    {
        pthread_join( threads[ i % 2 ], NULL );
    }
    ++i;
    if( i < num_switches )
    {
        assert( !pthread_create( &threads[ ( i + 1 ) % 2 ], NULL, f, NULL ) );
    }
    else
    {
        done = 1;
        pthread_cond_signal( &cond );
    }
    return p;
}

int main( int argc, char **argv )
{
    num_switches = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    num_switches = 1 << num_switches;
    printf( "NUM_SWITCHES: %d\n", num_switches );
    pthread_mutex_init( &mtx, NULL );
    pthread_cond_init( &cond, NULL );
    assert( !pthread_create( &threads[0], NULL, f, NULL ) );
    pthread_mutex_lock( &mtx );
    while( !done )
    {
        pthread_cond_wait( &cond, &mtx );
    }
    pthread_mutex_unlock( &mtx );
    printf( "%d\n", i );
    return 0;
}
