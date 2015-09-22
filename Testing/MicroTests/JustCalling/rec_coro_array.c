#include <stdlib.h>
#include <stdio.h>

int A[ 9 ] = { 4, 8, 3, 5, 6, 7, 2, 1, 0 };

#define N 1

typedef struct frame_t frame_t, *frame_p;

struct frame_t
{
    int n, x, half, tmp;
    void *ret_addr;
    frame_p caller;
    int *lhs_ptr;
    frame_p (*f)( frame_p );
};

frame_t frames[40];

int idx;

frame_p f( frame_p frame )
{
    // printf( "%d ", idx );
    if( frame->ret_addr )
        goto *frame->ret_addr;
    if( frame->n < 2 )
    {
        *frame->lhs_ptr = A[ frame->x ];
        --idx;
        return frame->caller;
    }
    frame->half = frame->n / 2;
    ++idx;
    frame_p callee = &frames[ idx ];
    callee->n = frame->n - frame->half;
    callee->x = frame->x;
    callee->caller = frame;
    callee->lhs_ptr = &frame->tmp;
    callee->f = f;
    callee->ret_addr = 0;
    frame->ret_addr = &&l1;
    return callee;
l1:
    {
        ++idx;
        frame_p callee = &frames[ idx ];
        callee->n = frame->half;
        callee->x = frame->tmp;
        callee->ret_addr = 0;
        frame->ret_addr = &&l2;
        return callee;
    }

l2:
    {
        *frame->lhs_ptr = frame->tmp;
        --idx;
        return frame->caller;
    }
}

int main( int argc, char **argv )
{
    int m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    frame_p frame = &frames[ 0 ];
    idx = 0;
    frame->n = 1 << ( m - 1 );
    frame->x = 1;
    frame->caller = 0;
    frame->lhs_ptr = &frame->tmp;
    frame->f = f;
    frame->ret_addr = 0;
    while( frame )
    {
        frame = frame->f( frame );
        // frame = f( frame );
    }
    return frames[0].tmp;
}
