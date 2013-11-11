#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

#define M 200000
#define N 100

void *f( void *a )
{
    long *x = (long *)a;
    unsigned int i;
    for( i = 0; i < M; ++i )
    {
        *x += random();
    }
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t[ N ];
    srandom( 42 );
    long i, x = 0;
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_create( &t[i], NULL, f, &x ) );
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    printf( "x:%li\n", x );
    return 0;
}
