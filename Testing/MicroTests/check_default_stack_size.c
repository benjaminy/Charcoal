#include <pthread.h>
#include <stdio.h>

int main( int argc, char **argv )
{
    pthread_attr_t attr;
    size_t stack_size;
    if( pthread_attr_init( &attr ) )
    {
        printf( "pthread_attr_init failed\n" );
        return 1;
    }
    if( pthread_attr_getstacksize( &attr, &stack_size ) )
    {
        printf( "pthread_attr_getstacksize failed\n" );
        return 1;
    }
    printf( "Default stack size: %iB  %ikB  %gMB\n", ((int)stack_size),
            ((int)(stack_size >> 10)), stack_size / (1024.0*1024.0) );
    return 0;
}
