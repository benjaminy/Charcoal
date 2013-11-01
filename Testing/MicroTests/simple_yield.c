#include <charcoal_runtime.h>
#include <stdio.h>

void fa( void *a )
{
    int i;
    char *str = (char *)a;
    for( i = 0; i < 5; ++i )
    {
        printf( "%s %i\n", str, i );
        __charcoal_yield();
    }
}

int __charcoal_replace_main( int argc, char **argv )
{
    __charcoal_activity_t *a = __charcoal_activate( fa, "A" );
    __charcoal_activity_t *b = __charcoal_activate( fa, "B" );
    __charcoal_activity_t *c = __charcoal_activate( fa, "C" );
    __charcoal_activity_join( a );
    printf( "Joined A\n" );
    __charcoal_activity_join( b );
    printf( "Joined B\n" );
    __charcoal_activity_join( c );
    printf( "Joined C\n" );
    return 0;
}
