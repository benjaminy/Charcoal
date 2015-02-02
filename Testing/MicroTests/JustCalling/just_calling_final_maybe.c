#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct frame_t frame_t, *frame_p;

typedef enum
{
    CORO_CALL,
    CORO_RETURN,
} coro_status;

struct frame_t
{
    frame_p prev;
    void *return_address;
    coro_status (*fn)( frame_p, frame_p * );
    int return_val, param, local;
};

coro_status f01( frame_p curr, frame_p *callee )
{
    curr->return_val = ( curr->param * 17 + 29 ) % 1553;
    return CORO_RETURN;
}

#define FXX( f1, f2 ) \
    coro_status f2( frame_p curr, frame_p *callee ) {             \
    if( curr->return_address ) \
    { \
        goto *curr->return_address; \
    } \
    *callee = (frame_p)malloc( sizeof( callee[0][0] ) ); \
    (*callee)->prev = curr; \
    (*callee)->return_address = 0; \
    (*callee)->param = curr->param; \
    (*callee)->fn = f1; \
    curr->return_address = &&step2; \
    return CORO_CALL; \
step2: \
    (*callee)->return_address = 0;              \
    (*callee)->param = (*callee)->return_val;      \
    curr->return_address = &&step3; \
    return CORO_CALL; \
step3: \
    curr->return_val = (*callee)->return_val;  \
    free( *callee ); \
    return CORO_RETURN; \
}

FXX( f01, f02 )
FXX( f02, f03 )
FXX( f03, f04 )
FXX( f04, f05 )
FXX( f05, f06 )
FXX( f06, f07 )
FXX( f07, f08 )
FXX( f08, f09 )
FXX( f09, f10 )
FXX( f10, f11 )
FXX( f11, f12 )
FXX( f12, f13 )
FXX( f13, f14 )
FXX( f14, f15 )
FXX( f15, f16 )
FXX( f16, f17 )
FXX( f17, f18 )
FXX( f18, f19 )
FXX( f19, f20 )
FXX( f20, f21 )
FXX( f21, f22 )
FXX( f22, f23 )
FXX( f23, f24 )
FXX( f24, f25 )
FXX( f25, f26 )
FXX( f26, f27 )
FXX( f27, f28 )
FXX( f28, f29 )
FXX( f29, f30 )

int main( int argc, char **argv )
{
    frame_p f = (frame_p)malloc( sizeof( f[0] ) );
    int return_val;
    frame_p callee;
    f->return_address = 0;
    f->param = argc;
    f->prev = NULL;
    f->fn = f26;
    while( f )
    {
        coro_status status;
        //if( f->fn == f01 )
        //    status = f01( f, &callee );
        //else
            status = f->fn( f, &callee );
        switch( status )
        {
        case CORO_CALL:
            f = callee;
            break;
        case CORO_RETURN:
            callee = f;
            return_val = f->return_val;
            f = f->prev;
            break;
        default:
            assert( 0 );
        }
    }
    return return_val;
}
