#include <boost/coroutine/all.hpp>
#include <iostream>

#if 0
#define N 10
int m = N;
activity_t activities[2];
csemaphore_t sem;

void f1( int );

void f1( int i )
{
    ++i;
    if( i > 1 )
    {
        // printf( "WAIT %d\n", i );
        wait_activity_done( &activities[ i % 2 ] );
    }
    if( i < m )
    {
        // printf( "ACTIVATE %d\n", i );
        activate[ &activities[ i % 2 ] ] ( i )
        {
            f1( i );
        }
    }
    else
    {
        printf( "DONE %d\n", i );
        semaphore_incr( &sem );
    }
}

int main( int argc, char **argv )
{
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    m = 1 << m;
    semaphore_open( &sem, 0 );
    activate[ &activities[0] ] ()
    {
        f1( 0 );
    }
    semaphore_decr( &sem );
    printf( "DONE!!!\n" );
    return 0;
}
#endif

int main( int argc, char **argv )
{
    std::cout << "Hello Coroutine World" << std::endl;
    return 0;
}
