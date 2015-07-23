#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

int done;
pthread_cond_t *conds;
pthread_mutex_t *mtxs;

void *f( void *p )
{
    long i = (long)p;
    pthread_mutex_lock( &mtxs[i] );
    while( !done )
    {
        pthread_cond_wait( &conds[i], &mtxs[i] );
    }
    pthread_mutex_unlock( &mtxs[i] );
}

int main( int argc, char ** argv )
{
    long limit, i;

    for( limit = 1; limit < 1000000000; limit *= 2 )
    {
        printf( "Lets try spawning %ld threads\n", limit ); fflush( stdout );
        pthread_t *threads = (pthread_t *)malloc( limit * sizeof( threads[0] ) );
        conds =  (pthread_cond_t *)malloc( limit * sizeof( conds[0] ) );
        mtxs  = (pthread_mutex_t *)malloc( limit * sizeof( mtxs[0] ) );
        done = 0;
        for( i = 0; i < limit; ++i )
        {
            pthread_mutex_init( &mtxs[i], NULL );
            pthread_cond_init( &conds[i], NULL );
            pthread_create( &threads[i], NULL, f, (void *)i );
        }
        done = 1;
        for( i = 0; i < limit; ++i )
        {
            pthread_mutex_lock( &mtxs[i] );
            pthread_mutex_unlock( &mtxs[i] );
            pthread_cond_signal( &conds[i] );
            pthread_join( threads[i], NULL );
        }
        free( threads );
        free( conds );
        free( mtxs );
    }
}
