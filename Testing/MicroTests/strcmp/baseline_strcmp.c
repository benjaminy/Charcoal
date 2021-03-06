#include <stdlib.h>

#define N ( 1024 * 1024 )
#define M 10000

int my_strcmp( const char *s1, const char *s2 )
{
    while( *s1 )
    {
        if( *s1 - *s2 )
            break;
        ++s1;
        ++s2;
    }
    return *s1 - *s2;
}

int main()
{
    char *s1 = malloc( N ), *s2 = malloc( N );
    int i;
    if( !s1 || !s2 )
        return -1;
    for( i = 0; i < N; ++i )
    {
        char x = rand();
        x = ( x == 0 ? 1 : x );
        s2[i] = s1[i] = x;
    }
    s2[N-1] = s1[N-1] = '\0';
    int y = 0;
    for( i = 0; i < M; i++ )
    {
        y += my_strcmp( s1, s2 );
        s2[N-2] = s1[N-2] = i;
    }
    return y;
}
