#include <charcoal_runtime.h>
#include <stdio.h>

void fa( void *a )
{
    int i;
    char *str = (char *)a;
    for( i = 0; i < 5; ++i )
    {
        printf( "%s %i\n", str, i );
        crcl(yield)();
    }
}

int crcl(replace_main)( int argc, char **argv )
{
    crcl(activity_t) *a = crcl(activate)( fa, "A" );
    crcl(activity_t) *b = crcl(activate)( fa, "B" );
    crcl(activity_t) *c = crcl(activate)( fa, "C" );
    crcl(activity_join)( a );
    printf( "Joined A\n" );
    crcl(activity_join)( b );
    printf( "Joined B\n" );
    crcl(activity_join)( c );
    printf( "Joined C\n" );
    return 0;
}
