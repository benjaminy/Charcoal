#include <charcoal_base.h>
#include <charcoal_runtime.h>
#include <stdlib.h>
#include <stdio.h>

#define N 10

void f( void *p )
{
    int *i = (int *)p;
    printf( "Goodbye %d\n", *i );
}

int crcl(application_main)( int argc, char **argv )
{
    printf( "Hello World\n" );
    crcl(activity_t) as[N];
    unsigned i;
    for( i = 0; i < N; ++i )
    {
        crcl(activity_t) *a = (crcl(activity_t) *)malloc( sizeof( a[0] ) );
        int *i_copy = (int *)malloc( sizeof( i_copy[0] ) );
        *i_copy = i;
        crcl(activate)( a, f, i_copy );
    }
    return N;
}
