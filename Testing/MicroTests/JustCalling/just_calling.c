int f01( int a )
{
    return ( a * 17 + 29 ) % 43;
}

int f02( int a ) { return f01( f01( a ) ); }
int f03( int a ) { return f02( f02( a ) ); }
int f04( int a ) { return f03( f03( a ) ); }
int f05( int a ) { return f04( f04( a ) ); }
int f06( int a ) { return f05( f05( a ) ); }
int f07( int a ) { return f06( f06( a ) ); }
int f08( int a ) { return f07( f07( a ) ); }
int f09( int a ) { return f08( f08( a ) ); }
int f10( int a ) { return f09( f09( a ) ); }
int f11( int a ) { return f10( f10( a ) ); }
int f12( int a ) { return f11( f11( a ) ); }
int f13( int a ) { return f12( f12( a ) ); }
int f14( int a ) { return f13( f13( a ) ); }
int f15( int a ) { return f14( f14( a ) ); }
int f16( int a ) { return f15( f15( a ) ); }
int f17( int a ) { return f16( f16( a ) ); }
int f18( int a ) { return f17( f17( a ) ); }
int f19( int a ) { return f18( f18( a ) ); }
int f20( int a ) { return f19( f19( a ) ); }
int f21( int a ) { return f20( f20( a ) ); }
int f22( int a ) { return f21( f21( a ) ); }
int f23( int a ) { return f22( f22( a ) ); }
int f24( int a ) { return f23( f23( a ) ); }
int f25( int a ) { return f24( f24( a ) ); }
int f26( int a ) { return f25( f25( a ) ); }
int f27( int a ) { return f26( f26( a ) ); }
int f28( int a ) { return f27( f27( a ) ); }
int f29( int a ) { return f28( f28( a ) ); }
int f30( int a ) { return f29( f29( a ) ); }
int f31( int a ) { return f30( f30( a ) ); }
int f32( int a ) { return f31( f31( a ) ); }
int f33( int a ) { return f32( f32( a ) ); }
int f34( int a ) { return f33( f33( a ) ); }
int f35( int a ) { return f34( f34( a ) ); }
int f36( int a ) { return f35( f35( a ) ); }
int f37( int a ) { return f36( f36( a ) ); }
int f38( int a ) { return f37( f37( a ) ); }
int f39( int a ) { return f38( f38( a ) ); }
int f40( int a ) { return f39( f39( a ) ); }

int main( int argc, char **argv )
{
    return f33( argc );
}
