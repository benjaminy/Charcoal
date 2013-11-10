#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

pthread_mutex_t m;

#define N 100
#define M 100

void *f( void *a )
{
    unsigned int i;
    for( i = 0; i < M; ++i )
    {
        assert( !pthread_mutex_lock( &m ) );
        assert( !sched_yield() );
        assert( !pthread_mutex_unlock( &m ) );
    }
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t[ N ];
    unsigned int i;

    assert( !pthread_mutex_init( &m, NULL ) );
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
