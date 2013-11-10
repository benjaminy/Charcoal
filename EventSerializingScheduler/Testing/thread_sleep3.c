#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

#define N 100

void *f( void *a )
{
    // sleep( 1 );
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t[ N ];
    unsigned int i;

    for( i = 0; i < N; ++i )
    {
        assert( !pthread_create( &t[i], NULL, f, NULL ) );
    }
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    return 0;
}
