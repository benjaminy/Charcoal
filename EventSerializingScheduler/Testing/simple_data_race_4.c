#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

unsigned N, M;
long *X;

void *f( void *a )
{
    unsigned i, *s = (unsigned *)a;
    for( i = 0; i < M; ++i )
    {
        *X += rand_r( s );
    }
    return a;
}

int main( int argc, char **argv )
{
    N = 100; M = 100;
    if( argc > 2 )
    {
        long m = strtol( argv[2], NULL, 10 );
        if( m > 0 )
            M = m;
    }
    if( argc > 1 )
    {
        long n = strtol( argv[1], NULL, 10 );
        if( n > 0 )
            N = n;
    }

    pthread_t t[ N ];
    unsigned seeds[ N * 10 ];
    long i, x = 0;
    X = &x;
    for( i = 0; i < N; ++i )
    {
        seeds[ i ] = i;
        assert( !pthread_create( &t[i], NULL, f, &seeds[ i * 10 ] ) );
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    printf( "x:%li\n", x );
    return 0;
}
