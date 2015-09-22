#include <charcoal_runtime.h>
#include <stdio.h>

void f( void *a )
{
    printf( "bar\n" );
}

int crcl(replace_main)( int argc, char **argv )
{
    printf( "foo\n" );

    crcl(activity_t) *a = crcl(activate)( f, NULL );
    crcl(activity_join)( a );
    return 0;
}
