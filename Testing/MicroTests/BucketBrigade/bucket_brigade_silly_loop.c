#include <stdlib.h>
#include <stdio.h>

#define N 10
int m = N;

typedef struct
{
    void (*f)( int, int );
    int x, y;
} closure_t;

closure_t closures[N];
closure_t *c = &closures[0];

void f( int x, int y )
{
    c = &closures[ ( x + 1 ) % N ];
    if( x == ( N - 1 ) )
    {
        ++c->y;
        if( y == m )
        {
            c = NULL;
        }
    }
    else
    {
        c->y = closures[ x ].y;
    }
}

int main( int argc, char **argv )
{
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    unsigned i;
    for( i = 0; i < N; ++i )
    {
        closures[i].f = f;
        closures[i].x = i;
        closures[i].y = 0;
    }
    while( c )
    {
        c->f( c->x, c->y );
    }
    printf( "Done!\n" );
}
