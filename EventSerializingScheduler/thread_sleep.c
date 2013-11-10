#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>

pthread_mutex_t m;

void *f( void *a )
{
    struct timeval tp;
    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_mutex_lock( &m ) );
    assert( !gettimeofday( &tp, NULL ) );
    sleep( 1 );
    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_mutex_unlock( &m ) );
    assert( !gettimeofday( &tp, NULL ) );
    return a;
}

int main( int argc, char **argv )
{
    pthread_t t;
    struct timeval tp;

    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_mutex_init( &m, NULL ) );
    assert( !pthread_create( &t, NULL, f, NULL ) );
    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_mutex_lock( &m ) );
    assert( !gettimeofday( &tp, NULL ) );
    sleep( 1 );
    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_mutex_unlock( &m ) );
    assert( !gettimeofday( &tp, NULL ) );
    assert( !pthread_join( t, NULL ) );
    return 0;
}
