unsigned int f01( unsigned int a )
{
    return ( a + 29 ) % 113;
}

unsigned int f02( unsigned int a ) { return f01( f01( a ) ); }
unsigned int f03( unsigned int a ) { return f02( f02( a ) ); }
unsigned int f04( unsigned int a ) { return f03( f03( a ) ); }
unsigned int f05( unsigned int a ) { return f04( f04( a ) ); }
unsigned int f06( unsigned int a ) { return f05( f05( a ) ); }
unsigned int f07( unsigned int a ) { return f06( f06( a ) ); }
unsigned int f08( unsigned int a ) { return f07( f07( a ) ); }
unsigned int f09( unsigned int a ) { return f08( f08( a ) ); }
unsigned int f10( unsigned int a ) { return f09( f09( a ) ); }
unsigned int f11( unsigned int a ) { return f10( f10( a ) ); }
unsigned int f12( unsigned int a ) { return f11( f11( a ) ); }
unsigned int f13( unsigned int a ) { return f12( f12( a ) ); }
unsigned int f14( unsigned int a ) { return f13( f13( a ) ); }
unsigned int f15( unsigned int a ) { return f14( f14( a ) ); }
unsigned int f16( unsigned int a ) { return f15( f15( a ) ); }
unsigned int f17( unsigned int a ) { return f16( f16( a ) ); }
unsigned int f18( unsigned int a ) { return f17( f17( a ) ); }
unsigned int f19( unsigned int a ) { return f18( f18( a ) ); }
unsigned int f20( unsigned int a ) { return f19( f19( a ) ); }
unsigned int f21( unsigned int a ) { return f20( f20( a ) ); }
unsigned int f22( unsigned int a ) { return f21( f21( a ) ); }
unsigned int f23( unsigned int a ) { return f22( f22( a ) ); }
unsigned int f24( unsigned int a ) { return f23( f23( a ) ); }
unsigned int f25( unsigned int a ) { return f24( f24( a ) ); }
unsigned int f26( unsigned int a ) { return f25( f25( a ) ); }
unsigned int f27( unsigned int a ) { return f26( f26( a ) ); }
unsigned int f28( unsigned int a ) { return f27( f27( a ) ); }
unsigned int f29( unsigned int a ) { return f28( f28( a ) ); }
unsigned int f30( unsigned int a ) { return f29( f29( a ) ); }
unsigned int f31( unsigned int a ) { return f30( f30( a ) ); }
unsigned int f32( unsigned int a ) { return f31( f31( a ) ); }
unsigned int f33( unsigned int a ) { return f32( f32( a ) ); }
unsigned int f34( unsigned int a ) { return f33( f33( a ) ); }
unsigned int f35( unsigned int a ) { return f34( f34( a ) ); }
unsigned int f36( unsigned int a ) { return f35( f35( a ) ); }
unsigned int f37( unsigned int a ) { return f36( f36( a ) ); }
unsigned int f38( unsigned int a ) { return f37( f37( a ) ); }
unsigned int f39( unsigned int a ) { return f38( f38( a ) ); }
unsigned int f40( unsigned int a ) { return f39( f39( a ) ); }

int main( int argc, char **argv )
{
    return f29( argc );
}
