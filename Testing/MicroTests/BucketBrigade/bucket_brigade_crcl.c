#include <charcoal_base.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <charcoal_std_lib.h>

#define N 10
int m = N;
semaphore_t s[N];

void f( void *p )
{
    int me = *((int *)p);
    unsigned int i;
    for( i = 0; i < m; ++i )
    {
        semaphore_decr( &s[ me ] );
        semaphore_incr( &s[ ( me + 1 ) % N ] );
    }
    printf( "Done %d\n", me );
}

int __charcoal_application_main( int argc, char **argv )
{
    CRCL(activity_t) as[N];
    printf( "sizeof container      %d\n", (int)sizeof(as[0].container) );
    printf( "sizeof can_run        %d\n", (int)sizeof(as[0].can_run) );
    printf( "sizeof flags          %d\n", (int)sizeof(as[0].flags) );
    printf( "sizeof next x5        %d\n", (int)sizeof(as[0].next) );
    printf( "sizeof ctx            %d\n", (int)sizeof(as[0].ctx) );
    printf( "sizeof jmp            %d\n", (int)sizeof(as[0].jmp) );
    printf( "sizeof yield_attempts %d\n", (int)sizeof(as[0].yield_attempts) );
    printf( "sizeof ret_size       %d\n", (int)sizeof(as[0].ret_size) );
    printf( "sizeof return_value   %d\n", (int)sizeof(as[0].return_value) );

    unsigned int i;
    printf( "Hello World\n" );
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    for( i = 0; i < N; ++i )
    {
        semaphore_open( &s[i], 0 );
    }
    for( i = 0; i < N; ++i )
    {
        int *i_copy = (int *)malloc( sizeof( i_copy[0] ) );
        *i_copy = i;
        CRCL(activate)( &as[i], f, i_copy );
    }
    semaphore_incr( &s[0] );
    for( i = 0; i < N; ++i )
    {
        /* printf( "trying to join %d  %p\n", i, &as[i] ); */
        assert( !CRCL(activity_join)( &as[i], NULL ) );
    }
    for( i = 0; i < N; ++i )
    {
        semaphore_close( &s[i] );
    }
    return 0;
}
