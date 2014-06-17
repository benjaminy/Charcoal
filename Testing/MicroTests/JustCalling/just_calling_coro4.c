#include <assert.h>
#include <stdlib.h>

typedef struct _frame _frame, *frame;

typedef enum
{
    CORO_CALL,
    CORO_RETURN,
} coro_status;

struct _frame
{
    coro_status status;
    frame prev;
    int return_address, param, return_val, *return_val_ptr;
    frame (*fn)( frame );
};

frame f1( frame f )
{
    int a = f->param;
    *f->return_val_ptr = ( a * 17 + 29 ) % 43;
    f->status = CORO_RETURN;
    return NULL;
}

#define FXX( n1, f2 ) \
frame f2( frame f ) { \
    static _frame n; \
    switch( f->return_address ) \
    { \
    case 1: \
    { \
        frame next = &n; \
        next->return_address = 1; \
        next->param = f->param; \
        next->return_val_ptr = &f->return_val; \
        next->fn = f##n1; \
        next->prev = f; \
        f->return_address = 2; \
        f->status = CORO_CALL; \
        return next; \
    } \
    case 2: \
    { \
        frame next = &n; \
        next->return_address = 1; \
        next->param = f->return_val; \
        f->return_address = 3; \
        return next; \
    } \
    case 3: \
    { \
        *f->return_val_ptr = f->return_val; \
        f->status = CORO_RETURN; \
        return NULL; \
    } \
    default: \
        break; \
    } \
    assert( 0 ); \
}

FXX( 1, f2 )
FXX( 2, f3 )
FXX( 3, f4 )
FXX( 4, f5 )
FXX( 5, f6 )
FXX( 6, f7 )
FXX( 7, f8 )
FXX( 8, f9 )
FXX( 9, f10 )
FXX( 10, f11 )
FXX( 11, f12 )
FXX( 12, f13 )
FXX( 13, f14 )
FXX( 14, f15 )
FXX( 15, f16 )
FXX( 16, f17 )
FXX( 17, f18 )
FXX( 18, f19 )
FXX( 19, f20 )
FXX( 20, f21 )
FXX( 21, f22 )
FXX( 22, f23 )
FXX( 23, f24 )
FXX( 24, f25 )
FXX( 25, f26 )
FXX( 26, f27 )
FXX( 27, f28 )
FXX( 28, f29 )
FXX( 29, f30 )
FXX( 30, f31 )
FXX( 31, f32 )
FXX( 32, f33 )
FXX( 33, f34 )
FXX( 34, f35 )
FXX( 35, f36 )
FXX( 36, f37 )
FXX( 37, f38 )
FXX( 38, f39 )
FXX( 39, f40 )

int main( int argc, char **argv )
{
    frame f = (frame)malloc( sizeof( f[0] ) );
    int return_val;
    f->return_address = 1;
    f->param = argc;
    f->status = CORO_CALL;
    f->prev = NULL;
    f->return_val_ptr = &return_val;
    f->fn = f33;
    while( f )
    {
        frame next = f->fn( f );
        switch( f->status )
        {
        case CORO_CALL:
            f = next;
            break;
        case CORO_RETURN:
            next = f->prev;
            // free( f );
            f = next;
            break;
        default:
            assert( 0 );
        }
    }
    return return_val;
}
