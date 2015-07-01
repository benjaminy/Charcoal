int A[ 9 ] = { 4, 8, 3, 5, 6, 7, 2, 1, 0 };

#define N 100000000

int f( int n, int x )
{
    if( n < 2 )
        return A[ x ];
    else
        return f( ( n + 1 ) / 2, f( n / 2, x ) );
}

int main( int argc, char **argv )
{
    return f( N, 1 );
}
