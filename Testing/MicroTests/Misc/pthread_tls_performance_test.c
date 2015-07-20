#include <pthread.h>
#include <assert.h>

#define NUM_TESTERS 100
#define TEST_ITERS 10000000

typedef void (*f_t)( pthread_key_t * );
typedef void (*f2_t)( size_t * );

inline void f1( pthread_key_t *key )
{
    void *val = pthread_getspecific( *key );
    *((size_t *)val) += 1;
    assert( 0 == pthread_setspecific( *key, val ) );
}

inline void f2( pthread_key_t *key )
{
    void *val = pthread_getspecific( *key );
    *((size_t *)val) += 2;
    assert( 0 == pthread_setspecific( *key, val ) );
}

inline void f3( size_t *v )
{
    *v += 1;
}

inline void f4( size_t *v )
{
    *v += 2;
}

void *tls_tester( void *param )
{
    pthread_key_t *key = (pthread_key_t *)param;
    size_t i, val;
    f_t fs[ 2 ];
    f2_t f2s[ 2 ];
    if( (long)param & (long)0x20 )
    { fs[0] = f1; fs[1] = f2; f2s[0] = f3; f2s[1] = f4; }
    else
    { fs[1] = f1; fs[0] = f2; f2s[1] = f3; f2s[0] = f4; }
    assert( 0 == pthread_setspecific( *key, &val ) );

    switch( (long)param & (long)0x1 )
    {
    case 0:
        for( i = 0; i < TEST_ITERS; ++i )
        {
            fs[0]( key );
            fs[1]( key );
        }
        break;
    case 2:
        for( i = 0; i < TEST_ITERS; ++i )
        {
            f2s[0]( &val );
            f2s[1]( &val );
        }
        break;
    case 3:
        for( i = 0; i < TEST_ITERS; ++i )
        {
            f1( key );
            f2( key );
        }
        break;
    case 1:
        for( i = 0; i < TEST_ITERS; ++i )
        {
            f3( &val );
            f4( &val );
        }
        break;
    default:
        break;
    }
    return NULL;
}

int main( int  argc, char **argv )
{
    size_t i;
    pthread_t testers[ NUM_TESTERS ];
    pthread_key_t key;
    assert( 0 ==  pthread_key_create( &key, NULL /*void (*destructor)(void *))*/ ) );
    for( i = 0; i < NUM_TESTERS; ++i )
    {
        assert( 0 == pthread_create( &testers[ i ], NULL, tls_tester, &key ) );
    }
    for( i = 0; i < NUM_TESTERS; ++i )
    {
        assert( 0 == pthread_join( testers[ i ], NULL ) );
    }
    return 0;
}
