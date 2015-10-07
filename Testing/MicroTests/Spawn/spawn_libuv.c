#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <uv.h>

#define N 10
int m = N;
uv_idle_t events[ 2 ];

static void callback( uv_idle_t *handle )
{
    int *i_ptr = (int *)handle->data;
    int i = *i_ptr;
    ++i;
    if( i > 1 )
    {
        uv_idle_stop( &events[ i % 2 ] );
    }
    if( i < m )
    {
        // printf( "%d ", i ); fflush( stdout );
        events[ i % 2 ].data = handle->data;
        *i_ptr = i;
        uv_idle_start( &events[ i % 2 ], callback );
    }
    else
    {
        uv_idle_stop( handle );
    }
}

int main( int argc, char **argv )
{
    uv_loop_t *loop = uv_default_loop();
    assert( loop );
    int i = 0;
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    m = 1 << m;

    uv_idle_init( loop, &events[ 0 ] );
    uv_idle_init( loop, &events[ 1 ] );
    events[ 0 ].data = &i;
    uv_idle_start( &events[ 0 ], callback );
    uv_run( loop, UV_RUN_DEFAULT);
    uv_loop_close( loop );
    return 0;
}
