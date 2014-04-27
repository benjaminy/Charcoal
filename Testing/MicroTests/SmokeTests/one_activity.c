#include <charcoal_base.h>
#include <charcoal_runtime.h>
#include <stdlib.h>
#include <stdio.h>

void f( void *p )
{
    printf( "Goodbye\n" );
}

int crcl(application_main)( int argc, char **argv )
{
    printf( "Hello World\n" );
    crcl(activity_t) *a = (crcl(activity_t) *)malloc( sizeof( a[0] ) );
    crcl(activate)( a, f, NULL );
    return 0;
}
