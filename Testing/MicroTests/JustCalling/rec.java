class rec
{
    static int A[] = { 4, 8, 3, 5, 6, 7, 2, 1, 0 };

    static int f( int n, int x )
    {
        if( n < 2 )
            return A[ x ];
        else
            return f( ( n + 1 ) / 2, f( n / 2, x ) );
    }

    public static void main( String args[] )
    {
        int N = 100000000;
        System.out.println( ""+f( N, 1 ) );
    }
}
