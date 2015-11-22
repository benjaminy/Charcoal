#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

typedef struct
{
    int i;
    pthread_mutex_t m;
    pthread_cond_t c;
} dumb_sem;

void dumb_sem_init( dumb_sem *s, int i )
{
    s->i = i;
    assert( !pthread_mutex_init( &s->m, NULL ) );
    assert( !pthread_cond_init ( &s->c, NULL ) );
}

void dumb_sem_destroy( dumb_sem *s )
{
    assert( !pthread_mutex_destroy( &s->m ) );
    assert( !pthread_cond_destroy ( &s->c ) );
}

void dumb_sem_inc( dumb_sem *s )
{
    int sig = 0;
    assert( !pthread_mutex_lock( &s->m ) );
    sig = ( s->i == 0 );
    ++(s->i);
    assert( !pthread_mutex_unlock( &s->m ) );
    if( sig )
    {
        assert( !pthread_cond_signal( &s->c ) );
    }
}

void dumb_sem_dec( dumb_sem *s )
{
    assert( !pthread_mutex_lock( &s->m ) );
    while( s->i < 1 )
    {
        assert( !pthread_cond_wait( &s->c, &s->m ) );
    }
    --(s->i);
    assert( !pthread_mutex_unlock( &s->m ) );
}

#define N 100
int m = N;
dumb_sem s[N];

void *f( void *p )
{
    int me = *((int *)p);
    unsigned int i;
    for( i = 0; i < m; ++i )
    {
        dumb_sem_dec( &s[ me ] );
        dumb_sem_inc( &s[ ( me + 1 ) % N ] );
    }
    return p;
}

int main( int argc, char **argv )
{
    pthread_t t[N];
    unsigned int i;
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    m = ( 1 << m ) / N;
    for( i = 0; i < N; ++i )
    {
        dumb_sem_init( &s[i], 0 );
    }
    for( i = 0; i < N; ++i )
    {
        int *p = (int *)malloc( sizeof(p[0]) );
        *p = i;
        assert( !pthread_create( &t[i], NULL, f, p ) );
    }
    dumb_sem_inc( &s[0] );
    for( i = 0; i < N; ++i )
    {
        assert( !pthread_join( t[i], NULL ) );
    }
    for( i = 0; i < N; ++i )
    {
        dumb_sem_destroy( &s[i] );
    }
    return 0;
}
