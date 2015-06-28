int A[ 10 ] = { 4, 0, 3, 5, 6, 7, 2, 1 };

int f( int n, int x )
{
    if( n < 2 )
    {
        return A[ x ];
    }
    else
    {
        int half1 = n / 2;
        int half2 = n - half1;
        return f( half1, f( half2, x ) );
    }
}

int main( int argc, char **argv )
{
    return f( 100000000, 1 );
}
