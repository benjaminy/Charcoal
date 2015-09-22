#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <uv.h>

#define N 10

typedef struct
{
    unsigned i, ctr;
} ctx_t;

uv_async_t watchers[N];
int m = N;

static void close_cb( uv_handle_t* handle )
{
    uv_async_t *w = (uv_async_t*)handle;
    printf( "Close! %d\n", ((ctx_t*)w->data)->i );
}

static void callback( uv_async_t *handle )
{
    ctx_t *ctx = (ctx_t*)handle->data;
    ++ctx->ctr;
    if( ctx->ctr < m )
    {
        if( uv_async_send( &watchers[ ( ctx->i + 1 ) % N ] ) )
        {
            exit( -1 );
        }
    }
    else
    {
        if( ctx->i < ( N - 1 ) )
        {
            if( uv_async_send( &watchers[ ( ctx->i + 1 ) % N ] ) )
            {
                exit( -1 );
            }
        }
        uv_close( (uv_handle_t*)&watchers[ ctx->i ], close_cb );
    }
}

uv_loop_t *loop;

int main( int argc, char **argv )
{
    uv_loop_t *loop = uv_default_loop();
    unsigned i;

    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    m = ( 1 << m ) / N;

    for( i = 0; i < N; ++i )
    {
        ctx_t *ctx = (ctx_t*)malloc( sizeof( ctx[0] ) );
        watchers[i].data = ctx;
        ctx->i = i;
        ctx->ctr = 0;
        if( uv_async_init( loop, &watchers[i], callback ) )
        {
            exit( -1 );
        }
    }

    if( uv_async_send( &watchers[ 0 ] ) )
    {
        exit( -1 );
    }

    return uv_run( loop, UV_RUN_DEFAULT );
}
