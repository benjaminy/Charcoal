#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _frame _frame, *frame;

typedef enum
{
    CORO_CALL,
    CORO_RETURN,
} coro_status;

struct _frame
{
    coro_status status;
    frame prev, local;
    int param, return_val, *return_val_ptr;
    frame (*fn)( frame );
};

frame f01a( frame f )
{
    int a = f->param;
    *f->return_val_ptr = ( a * 17 + 29 ) % 43;
    f->status = CORO_RETURN;
    return NULL;
}

#define FXX( f1, f2 ) \
frame f2##b( frame f );                                 \
frame f2##c( frame f );                                 \
frame f2##a( frame f ) {                                \
    frame next = (frame)malloc( sizeof( next[0] ) );    \
    /*printf( "" #f2 "a\n" );*/                         \
    next->param = f->param;                             \
    next->return_val_ptr = &f->return_val;              \
    /*next->return_val_ptr = &f->param;*/               \
    next->fn = f1##a;                                   \
    next->prev = f;                                     \
    f->local = next;                                    \
    f->fn = f2##b;                                      \
    f->status = CORO_CALL;                              \
    return next;                                        \
}                                                       \
frame f2##b( frame f )                                  \
{                                                       \
    frame next = f->local;                              \
    /*printf( "" #f2 "b\n" );*/                         \
    next->param = f->return_val;                        \
    next->fn = f1##a;                                   \
    f->fn = f2##c;                                      \
    return next;                                        \
}                                                       \
frame f2##c( frame f )                                  \
{                                                       \
    /*printf( "" #f2 "c\n" );*/                         \
    *f->return_val_ptr = f->return_val;                 \
    f->status = CORO_RETURN;                            \
    free( f->local );                                   \
    return NULL;                                        \
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
    frame f = (frame)malloc( sizeof( f[0] ) );
    int return_val;
    f->param = argc;
    f->status = CORO_CALL;
    f->prev = NULL;
    f->return_val_ptr = &return_val;
    f->fn = f25a;
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
