#include <stdarg.h>
#include <stdio.h>

void f2( float y, va_list args )
{
    double z = va_arg( args, double );
    printf( "B %f\n", z );
}

void f1( float x, ... )
{
    va_list args;
    va_start( args, x );
    double y = va_arg( args, double );
    if( x < 1 )
    {
        printf( "A %f\n", y );
    }
    else
    {
        f2( y, args );
    }
    va_end( args );
}

int main( int argc, char ** argv )
{
    f1( 0, 3.4 );
    f1( 2, 3.4, 4.5 );
    return 0;
}
