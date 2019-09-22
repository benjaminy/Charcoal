#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define EXIT_ON_ERR( code ) \
    do { \
        if( code ) \
        { \
        printf( "Mystery error %d\n", code ); \
            exit( 1 ); \
        } \
    } while( 0 )


#define N 1000
struct thread_info_s
{
    int *balance_ptr;
    int worked;
};

void *thread_fn( void *param )
{
    struct thread_info_s *info = (struct thread_info_s *)param;
    info->worked = 0;
    if( *info->balance_ptr >= 100 )
    {
        int rc = usleep( 100 );
        EXIT_ON_ERR( rc );
        *info->balance_ptr -= 100;
        info->worked = 1;
    }
    return NULL;
}

int main( int argc, char **argv )
{
    pthread_t threads[ N ];
    struct thread_info_s info[ N ];
    int bank_balance = 1000;

    for( int i = 0; i < N; ++i )
    {
        info[ i ].balance_ptr = &bank_balance;
        int rc = pthread_create( &threads[ i ], NULL, thread_fn, &info[ i ] );
        EXIT_ON_ERR( rc );
    }

    for( int i = 0; i < N; ++i )
    {
        int rc = pthread_join( threads[ i ], NULL );
        EXIT_ON_ERR( rc );
    }

    printf( "Balance: %d\n", bank_balance );
    int worked_count = 0;
    for( int i = 0; i < N; ++i )
    {
        if( info[ i ].worked )
        {
            ++worked_count;
        }
    }

    printf( "Worked count: %d\n", worked_count );
}
