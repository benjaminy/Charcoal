#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

#define M 1000000
#define N 1000

unsigned int z;

void *f( void *a )
{
    unsigned int *x = (unsigned int *)a;
    unsigned int i, y = *x;
    // sleep( 1 );
    // pthread_yield();
    for( i = 0; i < M; ++i )
    {
        x[i] += x[M-(i+1)];
    }
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t[ N ];
    unsigned int i, x[M];
    z = argc;
    for( i = 0; i < M; ++i )
    {
        x[i] = i + 1;
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_create( &t[i], NULL, f, x ) );
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    z = 0;
    for( i = 0; i < M; ++i )
    {
        z += x[i];
    }
    printf( "x:%u\n", z );
    return 0;
}
