/*
 * The Charcoal Runtime System
 */

#include<signal.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<limits.h>
#include<charcoal_runtime.h>

void __charcoal_unyielding_enter()
{
}

void __charcoal_unyielding_exit()
{
}

int __charcoal_sem_init( __charcoal_sem_t *s, int pshared, unsigned int value )
{
    /* assert( 0 == pshared ) */
    if( pthread_mutex_init( &s->m, NULL ) )
    {
        return 1;
    }
    if( pthread_cond_init( &s->c, NULL ) )
    {
        return 1;
    }
    return 0;
}

int __charcoal_sem_destroy(  __charcoal_sem_t *s )
{
    return 42; /* XXX */
}

int __charcoal_sem_getvalue( __charcoal_sem_t * __restrict s, int * __restrict vp )
{
    if( !s || !vp )
    {
        /* error handling */
    }
    pthread_mutex_lock( &s->m );
    *vp = s->value;
    pthread_mutex_unlock( &s->m );
    return 0;
}

int __charcoal_sem_post( __charcoal_sem_t *s )
{
    unsigned waiters = 0;
    if( !s )
    {
        /* XXX error handling */
    }
    pthread_mutex_lock( &s->m );
    /* XXX overflow */
    waiters = s->waiters;
    ++s->value;
    pthread_mutex_unlock( &s->m );
    if( s->waiters > 0 )
    {
        pthread_cond_signal( &s->c );
    }
    return 0;
}

int __charcoal_sem_trywait( __charcoal_sem_t *s )
{
    int rv;
    if( !s )
    {
        /* XXX error handling */
    }
    pthread_mutex_lock( &s->m );
    if( s->value > 0 )
    {
        --s->value;
        rv = 0;
    }
    else
    {
        rv = -1;
        errno = EAGAIN;
    }
    pthread_mutex_unlock( &s->m );
    return rv;
}

int __charcoal_sem_wait( __charcoal_sem_t *s )
{
    if( !s )
    {
        /* XXX error handling */
    }
    pthread_mutex_lock( &s->m );
    while( s->value < 1 )
    {
        ++s->waiters;
        /* XXX err checking */ pthread_cond_wait( &s->c, &s->m );
        --s->waiters;
    }
    --s->value;
    pthread_mutex_unlock( &s->m );
    return 0;
}

pthread_key_t selfish;

int __charcoal_yield()
{
    int rv = 0;
    __charcoal_activity_t *foo = (__charcoal_activity_t *)pthread_getspecific( selfish );
    // int unyield_depth = OPA_load_int( &foo->_opa_const );
    if( 0 /* == unyield_depth */ )
    {
        /* rv = invoke the scheduler */
    }
    return rv;
}

void __charcoal_activity_set_return_value( void *ret_val_ptr )
{
}

void __charcoal_activity_get_return_value( __charcoal_activity_t *act, void *ret_val_ptr )
{
}

void *__charcoal_start_activity( void *p )
{
    __charcoal_activity_t *_self = (__charcoal_activity_t *)p;
    if( pthread_setspecific( selfish, _self ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    _self->f( _self->args );
    /* wait around until the activity should deallocate itself */
    return NULL; /* XXX */
}

/* static assert PTHREAD_STACK_MIN > sizeof activity */

void __charcoal_switch_from_to( __charcoal_activity_t *from, __charcoal_activity_t *to )
{
    /* XXX assert from is the currently running activity */
    if( __charcoal_sem_post( &to->can_run ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    if( __charcoal_sem_wait( &from->can_run ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

void __charcoal_switch_to( __charcoal_activity_t *act )
{
    __charcoal_sem_post( &act->can_run );
    // sem_wait();
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
__charcoal_activity_t *__charcoal_activate( void (*f)( void *args ), void *args )
{
    pthread_attr_t attr;
    if( pthread_attr_init( &attr ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    size_t stack_size;
    if( pthread_attr_getstacksize( &attr, &stack_size ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    /* TODO: provide some way for the user to pass in a stack */
    void *new_stack = malloc( stack_size );
    if( NULL == new_stack )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    __charcoal_activity_t *act_info = (__charcoal_activity_t *)new_stack;
    act_info->f = f;
    act_info->args = args;
    if( __charcoal_sem_init( &act_info->can_run, 0, 0 ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    /* effective base of stack  */
    size_t actual_activity_sz = sizeof( act_info ) /* XXX: + return type size */;
    new_stack += actual_activity_sz;
    if( pthread_attr_setstack( &attr, new_stack, stack_size - actual_activity_sz ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }

    /* XXX: What do these do? */
    // int pthread_attr_setdetachstate ( &attr, int detachstate );
    // int pthread_attr_setguardsize(pthread_attr_t *, size_t );
    // int pthread_attr_setinheritsched( &attr, int inheritsched);
    // int pthread_attr_setschedparam  ( &attr, const struct sched_param *restrict param);
    // int pthread_attr_setschedpolicy ( &attr, int policy);
    // int pthread_attr_setscope       ( &attr, int contentionscope);

    if( pthread_create( &act_info->self, &attr, __charcoal_start_activity, act_info ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    if( pthread_attr_destroy( &attr ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }

    __charcoal_switch_to( act_info );
    return act_info;
}

static void __charcoal_timer_handler( int sig, siginfo_t *siginfo, void *context )
{
    printf ("Sending PID: %ld, UID: %ld\n",
            (long)siginfo->si_pid, (long)siginfo->si_uid);
}
 

void __charcoal_main( void )
{
    // sigaction sigact;
    // sigact.sa_sigaction = __charcoal_timer_handler;
    // sigact.sa_flags = SA_SIGINFO;
    // assert( 0 == sigemptyset( &sigact.sa_mask ) );
    // assert( 0 == sigaction( SIGALRM, &sigact, NULL );
}
