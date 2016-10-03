function* foo() {    for( x = 0; x < 10; ++x )    {        yield x;        try { console.log( "BLAH:"+this.coolio ); } catch( e ) { log( "NO" ); }        console.log( "IN: "+ x );    } }

g = foo();

d = true;
while( d )
{
    g.coolio = 17;
    r = g.next( 42 );
    d = r.done;
}
