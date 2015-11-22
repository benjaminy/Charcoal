#include <stdlib.h>
#include <stdio.h>

int A[ 9 ] = { 4, 8, 3, 5, 6, 7, 2, 1, 0 };

#define N 1

int f( long lont n, int x )
{
    if( n < 2 )
        return A[ x ];
    else
    {
        long long half = n / 2;
        return f( n - half, f( half, x ) );
    }
}

int main( int argc, char **argv )
{
    long long m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    int rv = f( ((long long)1) << ( m - 1 ), 1 );
    return rv;
}
