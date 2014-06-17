class just_calling
{

    public static int f01( int a )
    { return ( a * 17 + 29 ) % 43; }

    public static int f02( int a ) { return f01( f01( a ) ); }
    public static int f03( int a ) { return f02( f02( a ) ); }
    public static int f04( int a ) { return f03( f03( a ) ); }
    public static int f05( int a ) { return f04( f04( a ) ); }
    public static int f06( int a ) { return f05( f05( a ) ); }
    public static int f07( int a ) { return f06( f06( a ) ); }
    public static int f08( int a ) { return f07( f07( a ) ); }
    public static int f09( int a ) { return f08( f08( a ) ); }
    public static int f10( int a ) { return f09( f09( a ) ); }
    public static int f11( int a ) { return f10( f10( a ) ); }
    public static int f12( int a ) { return f11( f11( a ) ); }
    public static int f13( int a ) { return f12( f12( a ) ); }
    public static int f14( int a ) { return f13( f13( a ) ); }
    public static int f15( int a ) { return f14( f14( a ) ); }
    public static int f16( int a ) { return f15( f15( a ) ); }
    public static int f17( int a ) { return f16( f16( a ) ); }
    public static int f18( int a ) { return f17( f17( a ) ); }
    public static int f19( int a ) { return f18( f18( a ) ); }
    public static int f20( int a ) { return f19( f19( a ) ); }
    public static int f21( int a ) { return f20( f20( a ) ); }
    public static int f22( int a ) { return f21( f21( a ) ); }
    public static int f23( int a ) { return f22( f22( a ) ); }
    public static int f24( int a ) { return f23( f23( a ) ); }
    public static int f25( int a ) { return f24( f24( a ) ); }
    public static int f26( int a ) { return f25( f25( a ) ); }
    public static int f27( int a ) { return f26( f26( a ) ); }
    public static int f28( int a ) { return f27( f27( a ) ); }
    public static int f29( int a ) { return f28( f28( a ) ); }
    public static int f30( int a ) { return f29( f29( a ) ); }
    public static int f31( int a ) { return f30( f30( a ) ); }

    public static void main( String args[] )
    {
        System.out.println( ""+f30( args.length ) );
    }
}
