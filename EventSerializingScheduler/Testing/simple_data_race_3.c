#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

#define M 2000000
#define N 100

int x;

void *f( void *a )
{
    unsigned *seed = (unsigned *)a;
    unsigned int i;
    int xl = 0;
    for( i = 0; i < M; ++i )
    {
        xl += rand_r( seed );
    }
    x += xl;
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t[ N ];
    unsigned i, seeds[ N*100 ];
    srandom( 42 );
    x = 0;
    for( i = 0; i < N; ++i )
    {
        seeds[i] = i;
        assert( !pthread_create( &t[i], NULL, f, &seeds[i*100] ) );
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    printf( "x:%i\n", x );
    return 0;
}
