#include <charcoal_runtime.h>
#include <stdio.h>

void f( void *a )
{
    printf( "bar\n" );
}

int main( int argc, char **argv )
{
    printf( "foo\n" );

    __charcoal_activity_t *a = __charcoal_activate( f, NULL );

    return 0;
}
