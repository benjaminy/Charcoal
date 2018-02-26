
#include <errno.h>
#include <stdio.h>

int main()
{
    FILE *f = fopen( "C:\\Windows\\Temp\\Charcoal\\pc_2764.a06088", "w" );
    if( f ) {
        fprintf( f, "TEST\n" );
        fclose( f );
    }
    else {
        printf( "FAILED %d\n", errno );
    }
}
