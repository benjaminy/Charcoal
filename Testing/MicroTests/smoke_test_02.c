#include <charcoal_base.h>
#include <charcoal_runtime.h>
#include <stdlib.h>
#include <stdio.h>

void f( void *p )
{
    printf( "Goodbye\n" );
}

int __charcoal_application_main( int argc, char **argv )
{
    printf( "Hello World\n" );
    CRCL(activity_t) *a = (CRCL(activity_t) *)malloc( sizeof( a[0] ) );
    CRCL(activate)( a, f, NULL );
    return 0;
}
