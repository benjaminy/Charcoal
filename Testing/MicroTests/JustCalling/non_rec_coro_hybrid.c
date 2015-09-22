#include <assert.h>
#include <stdlib.h>

#define FIRST_CORO f25
#define FIRST_DIRECT d05

int d01( int a )
{
    return ( a * 17 + 29 ) % 1553;
}

int d02( int a ) { return d01( d01( a ) ); }
int d03( int a ) { return d02( d02( a ) ); }
int d04( int a ) { return d03( d03( a ) ); }
int d05( int a ) { return d04( d04( a ) ); }
int d06( int a ) { return d05( d05( a ) ); }
int d07( int a ) { return d06( d06( a ) ); }
int d08( int a ) { return d07( d07( a ) ); }
int d09( int a ) { return d08( d08( a ) ); }
int d10( int a ) { return d09( d09( a ) ); }
int d11( int a ) { return d10( d10( a ) ); }
int d12( int a ) { return d11( d11( a ) ); }
int d13( int a ) { return d12( d12( a ) ); }
int d14( int a ) { return d13( d13( a ) ); }
int d15( int a ) { return d14( d14( a ) ); }
int d16( int a ) { return d15( d15( a ) ); }
int d17( int a ) { return d16( d16( a ) ); }
int d18( int a ) { return d17( d17( a ) ); }
int d19( int a ) { return d18( d18( a ) ); }
int d20( int a ) { return d19( d19( a ) ); }
int d21( int a ) { return d20( d20( a ) ); }
int d22( int a ) { return d21( d21( a ) ); }
int d23( int a ) { return d22( d22( a ) ); }
int d24( int a ) { return d23( d23( a ) ); }
int d25( int a ) { return d24( d24( a ) ); }
int d26( int a ) { return d25( d25( a ) ); }
int d27( int a ) { return d26( d26( a ) ); }
int d28( int a ) { return d27( d27( a ) ); }
int d29( int a ) { return d28( d28( a ) ); }
int d30( int a ) { return d29( d29( a ) ); }
int d31( int a ) { return d30( d30( a ) ); }
int d32( int a ) { return d31( d31( a ) ); }
int d33( int a ) { return d32( d32( a ) ); }
int d34( int a ) { return d33( d33( a ) ); }
int d35( int a ) { return d34( d34( a ) ); }
int d36( int a ) { return d35( d35( a ) ); }
int d37( int a ) { return d36( d36( a ) ); }
int d38( int a ) { return d37( d37( a ) ); }
int d39( int a ) { return d38( d38( a ) ); }
int d40( int a ) { return d39( d39( a ) ); }

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
    curr->return_val = FIRST_DIRECT( curr->param );
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
    f->fn = FIRST_CORO;
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
