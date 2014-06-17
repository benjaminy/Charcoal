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

_frame frames[100];

#define FXX( f1, f2, n )                                \
frame f2##b( frame f );                                 \
frame f2##c( frame f );                                 \
frame f2##a( frame f ) {                                \
    frame next = &frames[n];                            \
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
    frame next = &frames[n];                            \
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
    return NULL;                                        \
}

FXX( f01, f02, 1 )
FXX( f02, f03, 2 )
FXX( f03, f04, 3 )
FXX( f04, f05, 4 )
FXX( f05, f06, 5 )
FXX( f06, f07, 6 )
FXX( f07, f08, 7 )
FXX( f08, f09, 8 )
FXX( f09, f10, 9 )
FXX( f10, f11, 10 )
FXX( f11, f12, 11 )
FXX( f12, f13, 12 )
FXX( f13, f14, 13 )
FXX( f14, f15, 14 )
FXX( f15, f16, 15 )
FXX( f16, f17, 16 )
FXX( f17, f18, 17 )
FXX( f18, f19, 18 )
FXX( f19, f20, 19 )
FXX( f20, f21, 20 )
FXX( f21, f22, 21 )
FXX( f22, f23, 22 )
FXX( f23, f24, 23 )
FXX( f24, f25, 24 )
FXX( f25, f26, 25 )
FXX( f26, f27, 26 )
FXX( f27, f28, 27 )
FXX( f28, f29, 28 )
FXX( f29, f30, 29 )

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
