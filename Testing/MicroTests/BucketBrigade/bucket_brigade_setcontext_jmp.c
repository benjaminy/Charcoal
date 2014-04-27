#define _XOPEN_SOURCE
#define _BSD_SOURCE
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#include <ucontext.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct activity_t activity_t;

struct activity_t
{
    ucontext_t ctx;
    jmp_buf    jmp; 
    activity_t *next, *prev, *rnext, *rprev;
    int id;
};

typedef struct trampoline_t trampoline_t;

struct trampoline_t
{
    ucontext_t *prv;
    jmp_buf    *cur; 
    void (*entry)( void * );
    void *param;
    activity_t *a;
};

activity_t *activities = NULL, *ready = NULL;

int ready_len()
{
    if( ready )
    {
        activity_t *foo = ready->rnext;
        int count = 1;
        while( foo != ready )
        {
            foo = foo->rnext;
            ++count;
        }
        return count;
    }
    else
    {
        return 0;
    }
}

activity_t *self = NULL;

void add_to_ready_queue()
{
    if( ready )
    {
        ready->rprev->rnext = self;
        self->rprev = ready->rprev;
        self->rnext = ready;
        ready->rprev = self;
    }
    else
    {
        ready = self;
        self->rnext = self;
        self->rprev = self;
    }
}

void switch_to( activity_t *to )
{
    activity_t *realself = self;
    /* printf( "switch %i\n", ready_len() ); */
    add_to_ready_queue();
    self = to;
    if( _setjmp( realself->jmp ) == 0 )
    {
        _longjmp( to->jmp, 1 ); 
    }
    self = realself;
}

void schedule()
{
    /* printf( "schedule %p %i\n", ready, ready_len() ); */
    if( ready )
    {
        activity_t *to = ready;
        if( ready->rnext == ready )
        {
            ready = NULL;
        }
        else
        {
            activity_t *next = ready->rnext;
            ready->rprev->rnext = next;
            next->rprev = ready->rprev;
            ready->rnext = NULL;
            ready->rprev = NULL;
            ready = next;
        }
        activity_t *realself = self;
        self = to;
        /* printf( "a %d -> %d   %p %p\n", realself->id, to->id, realself->jmp, to->jmp ); */
        if( _setjmp( realself->jmp ) == 0 )
        {
            /* printf( "c %d -> %d\n", realself->id, to->id ); */
            _longjmp( to->jmp, 1 ); 
        }
        /* printf( "b\n" ); */
        self = realself;
    }
    else
    {
        printf( "Not equipped to handle deadlock\n" );
        exit( -1 );
    }
}

void activity_entry( void *p )
{
    trampoline_t tramp = *((trampoline_t *)p);
    self = tramp.a;
    activity_t *end = activities->prev;
    end->next = self;
    self->prev = end;
    self->next = activities;
    activities->prev = self;
    self->rnext = NULL;
    self->rprev = NULL;
    add_to_ready_queue();

    /* printf( "Generic entry switching back %p\n", tramp.param ); fflush( stdout ); */
    if ( _setjmp( *tramp.cur ) == 0 ) 
    { 
        ucontext_t tmp; 
        swapcontext( &tmp, tramp.prv ); 
    } 

    self = tramp.a;
    /* printf( "Generic entry go %p\n", tramp.param ); fflush( stdout ); */
    tramp.entry( tramp.param );
    /* XXX return? */
    /* XXX No! Never return.  Just deallocate and setcontext? */
    /* XXX No! I guess we have to set the link... or maybe not */
    /* XXX Probably bad to do anything after freeing the stack */
    /* printf( "Activity DONE?\n" ); */
    while( ready )
    {
        schedule();
    }
    printf( "I guess we're done\n" );
    exit( 1 );
}

void activate( activity_t *a, void (*f)( void * ), void *p )
{
    /* printf( "Activate!\n" ); */
    assert( !getcontext( &a->ctx ) );
    a->ctx.uc_stack.ss_size = SIGSTKSZ;
    a->ctx.uc_stack.ss_sp = malloc( a->ctx.uc_stack.ss_size );
    a->ctx.uc_stack.ss_flags = 0; /* SA_DISABLE and/or SA_ONSTACK */
    a->ctx.uc_link = NULL; /* XXX fix. */
    sigemptyset( &a->ctx.uc_sigmask );

    ucontext_t tmp;
    trampoline_t tramp;
    tramp.prv = &tmp;
    tramp.cur = &a->jmp;
    tramp.entry = f;
    tramp.param = p;
    tramp.a = a;

    activity_t *real_self = self;

    makecontext( &a->ctx, activity_entry, 1, &tramp );
    swapcontext( &tmp, &a->ctx );
    self = real_self;
}

typedef struct
{
    int i;
    activity_t *waiter;
} dumb_sem;

void dumb_sem_init( dumb_sem *s, int i )
{
    s->i = i;
    s->waiter = NULL;
}

void dumb_sem_destroy( dumb_sem *s )
{
}

void dumb_sem_inc( dumb_sem *s )
{
    /* printf( "inc %i  %p\n", s->i, s->waiter ); */
    ++(s->i);
    if( s->waiter )
    {
        activity_t *w = s->waiter;
        s->waiter = NULL;
        switch_to( w );
    }
}

void dumb_sem_dec( dumb_sem *s )
{
    /* printf( "dec %i\n", s->i ); */
    while( s->i < 1 )
    {
        if( s->waiter )
        {
            printf( "HUH??? %p %p\n", self, s->waiter );
            // exit( -2 );
        }
        s->waiter = self;
        /* printf( "blam %i\n", s->i ); */
        schedule();
    }
    --(s->i);
}

#define N 10
int m = N;
dumb_sem s[N];
int done_count = 0;
dumb_sem done_sem;

void f( void *p )
{
    int me = *((int *)p);
    unsigned int i;
    /* printf( "Activity %i started!\n", me ); */
    for( i = 0; i < m; ++i )
    {
        /* printf( "(A %d %d  %p %d) \n", me, i, ready, ready_len() ); */
        dumb_sem_dec( &s[ me ] );
        /* printf( "(B %d %d  %p %d) \n", me, i, ready, ready_len() ); */
        dumb_sem_inc( &s[ ( me + 1 ) % N ] );
    }
    ++done_count;
    /* printf( "Almost hooray %d\n", done_count ); */
    if( done_count >= N )
    {
        dumb_sem_inc( &done_sem );
    }
}

int main( int argc, char **argv )
{
    activity_t main_activity;
    self = &main_activity;
    activities = self;
    assert( !getcontext( &main_activity.ctx ) );
    main_activity.next = self;
    main_activity.prev = self;
    main_activity.rnext = NULL;
    main_activity.rprev = NULL;
    main_activity.id = 12345;
    struct timeval t1, t2;

    assert( !gettimeofday( &t1, NULL ) );

    activity_t as[N];

    unsigned int i;
    m = ( argc > 1 ) ? (int)atol( argv[1] ) : N;
    dumb_sem_init( &done_sem, 0 );
    for( i = 0; i < N; ++i )
    {
        dumb_sem_init( &s[i], 0 );
    }
    for( i = 0; i < N; ++i )
    {
        int *p = (int *)malloc( sizeof(p[0]) );
        *p = i;
        as[i].id = i;
        activate( &as[i], f, p );
    }
    printf( "All activated\n" );
    dumb_sem_inc( &s[0] );
    for( i = 0; i < N; ++i )
    {
        // assert( !pthread_join( t[i], NULL ) );
    }
    for( i = 0; i < N; ++i )
    {
        dumb_sem_destroy( &s[i] );
    }
    printf( "Waiting for done\n" );
    dumb_sem_dec( &done_sem );
    assert( !gettimeofday( &t2, NULL ) );
    long long unsigned us = t2.tv_sec * 1000000 + t2.tv_usec;
    us -= (t1.tv_sec * 1000000 + t1.tv_usec);
    double per = (1.0 * us / m) * 1000 / N;
    printf( "Really? %g\n", per );
    while( ready )
    {
        schedule();
    }
    return 0;
}
