#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

int done = 0;
pthread_cond_t cond;
pthread_mutex_t mtx;

void *f( void *p )
{
    pthread_mutex_lock( &mtx );
    while( !done )
    {
        pthread_cond_wait( &cond, &mtx );
    }
    pthread_mutex_unlock( &mtx );
}

int main( int argc, char ** argv )
{
    long i;

    pthread_mutex_init( &mtx, NULL );
    pthread_cond_init( &cond, NULL );
    for( i = 0; i < 1000000000; ++i )
    {
        pthread_t *thread = (pthread_t *)malloc( sizeof( thread[0] ) );
        int rc = 0;
        if( thread )
            rc = pthread_create( thread, NULL, f, NULL );
        if( rc || !thread )
        {
            printf( "Failed on thread: %ld\n", i ); fflush( stdout );
            exit( 0 );
        }
    }
}
