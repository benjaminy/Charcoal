/*
 * The Charcoal Runtime System
 */

#include<signal.h>
#include<stdlib.h>
#include<string.h> /* XXX remove dep eventually */
#include<stdio.h> /* XXX remove dep eventually */
#include<errno.h>
#include<limits.h>
#include<charcoal_runtime.h>

int __charcoal_sem_init( __charcoal_sem_t *s, int pshared, unsigned int value )
{
    /* assert( 0 == pshared ) */
    int rc;
    if( ( rc = pthread_mutex_init( &s->m, NULL ) ) )
    {
        return rc;
    }
    if( ( rc = pthread_cond_init( &s->c, NULL ) ) )
    {
        return rc;
    }
    s->value = value;
    s->waiters = 0;
    return 0;
}

int __charcoal_sem_destroy(  __charcoal_sem_t *s )
{
    int rc;
    if( ( rc = pthread_mutex_destroy( &s->m ) ) )
    {
        return rc;
    }
    if( ( rc = pthread_cond_destroy( &s->c ) ) )
    {
        return rc;
    }
    if( s->waiters )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    return 0;
}

int __charcoal_sem_getvalue( __charcoal_sem_t * __restrict s, int * __restrict vp )
{
    if( !s || !vp )
    {
        return EINVAL;
    }
    int rc;
    if( ( rc = pthread_mutex_lock( &s->m ) ) )
    {
        return rc;
    }
    *vp = s->value;
    if( ( rc = pthread_mutex_unlock( &s->m ) ) )
    {
        return rc;
    }
    return 0;
}

int __charcoal_sem_post( __charcoal_sem_t *s )
{
    if( !s )
    {
        return EINVAL;
    }
    unsigned waiters = 0;
    int rc;
    if ( ( rc = pthread_mutex_lock( &s->m ) ) )
    {
        return rc;
    }
    unsigned old_val = s->value;
    ++s->value;
    if( old_val >= s->value )
    {
        if( ( rc = pthread_mutex_unlock( &s->m ) ) )
        {
            return rc;
        }
        return EOVERFLOW;
    }
    waiters = s->waiters;
    if( ( rc = pthread_mutex_unlock( &s->m ) ) )
    {
        return rc;
    }
    if( waiters > 0 )
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
        return EINVAL;
    }
    if( pthread_mutex_lock( &s->m ) )
    {
        return 1; /* XXX */
    }
    while( s->value < 1 )
    {
        ++s->waiters; /* XXX OVERFLOW */
        if( pthread_cond_wait( &s->c, &s->m ) )
        {
            return 1; /* XXX */
        }
        --s->waiters;
    }
    --s->value;
    if( pthread_mutex_unlock( &s->m ) )
    {
        return 1; /* XXX */
    }
    return 0;
}

/* Scheduler stuff */

/* If the size of t's activities store is either greater than its
 * capacity or less than 1/4, make the appropriate adjustment. */
int __charcoal_adjust_activity_cap( __charcoal_thread_t *t )
{
    /* Begin weird edge case handling */
    if( !t->activities_sz )
        return 0;
    if( !t->activities_cap )
    {
        if( t->activities )
        {
            return EINVAL;
        }
        t->activities = malloc( t->activities_sz * sizeof( t->activities[0] ) );
        if( !t->activities )
        {
            return ENOMEM;
        }
        t->activities_cap = t->activities_sz;
        return 0;
    }
    /* End weird edge case handling */

    while( t->activities_sz > t->activities_cap )
    {
        t->activities_cap *= 2;
        void *tmp = realloc(
            t->activities, t->activities_cap * sizeof( t->activities[0] ) );
        if( !tmp )
        {
            /* XXX memory leak? */
            return ENOMEM;
        }
        t->activities = tmp;
    }
    while( ( t->activities_cap / 4 ) > t->activities_sz )
    {
        t->activities_cap /= 2;
        void *tmp = realloc(
            t->activities, t->activities_cap * sizeof( t->activities[0] ) );
        if( !tmp )
        {
            /* XXX memory leak? */
            return ENOMEM;
        }
        t->activities = tmp;
    }
    return 0;
}

int __charcoal_choose_next_activity( __charcoal_activity_t **p )
{
    unsigned i;
    __charcoal_activity_t *to_run = NULL, *self = __charcoal_activity_self();
    __charcoal_thread_t *thd = self->container;
    int rc;
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    for( i = 0; i < thd->activities_sz; ++i )
    {
        to_run = thd->activities[ i ];
        if( !( to_run->flags & __CRCL_ACTF_BLOCKED )
            && ( to_run != self ) )
        {
            break;
        }
        to_run = NULL;
    }
    int sv = -1, rv = -1;
    __charcoal_sem_getvalue( &self->can_run, &sv );
    if( to_run )
        __charcoal_sem_getvalue( &to_run->can_run, &rv );
    printf( "SWITCH from: %p(%i)  to: %p(%i)\n",
            self, sv, to_run, rv );
    if( p )
    {
        *p = to_run;
    }
    if( ( rc = pthread_mutex_unlock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    return 0;
}

/* */

static pthread_key_t __charcoal_self_key;

__charcoal_activity_t *__charcoal_activity_self( void )
{
    return (__charcoal_activity_t *)pthread_getspecific( __charcoal_self_key );
}

void __charcoal_unyielding_enter()
{
}

void __charcoal_unyielding_exit()
{
}

int __charcoal_yield()
{
    int rv = 0;
    __charcoal_activity_t *foo = __charcoal_activity_self();
    // int unyield_depth = OPA_load_int( &foo->_opa_const );
    if( 1 /* == unyield_depth */ )
    {
        __charcoal_activity_t *to;
        /* XXX error check */
        __charcoal_choose_next_activity( &to );
        if( to )
        {
            int rc;
            if( ( rc = __charcoal_sem_post( &to->can_run ) ) )
            {
                return rc;
            }
            return __charcoal_sem_wait( &foo->can_run );
        }
        else
            return 0;
    }
    return rv;
}

void __charcoal_activity_set_return_value( void *ret_val_ptr )
{
}

void __charcoal_activity_get_return_value( __charcoal_activity_t *act, void *ret_val_ptr )
{
}

void __charcoal_switch_from_to( __charcoal_activity_t *from, __charcoal_activity_t *to )
{
    /* XXX assert from is the currently running activity? */
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
    __charcoal_switch_from_to( __charcoal_activity_self(), act );
}

/* This is the entry procedure for new activities */
static void *__charcoal_activity_entry_point( void *p )
{
    __charcoal_activity_t *next, *_self = (__charcoal_activity_t *)p;
    printf( "Starting activity %p\n", _self );
    if( pthread_setspecific( __charcoal_self_key, _self ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    if( __charcoal_sem_wait( &_self->can_run ) )
    {
        /* XXX Improve error handling */
        exit( 1 );
    }
    _self->f( _self->args );
    printf( "Finishing activity %p\n", _self );
    if( _self->flags & __CRCL_ACTF_DETACHED )
    {
        /* deallocate activity */
    }
    else
    {
        /* blah */
    }
    _self->flags |= __CRCL_ACTF_BLOCKED;
    /* Who knows if another activity is joining? */

    if( __charcoal_choose_next_activity( &next ) )
    {
        exit( 1 );
    }
    if( next )
    {
        if( __charcoal_sem_post( &next->can_run ) )
        {
            /* XXX Improve error handling */
            exit( 1 );
        }
    }
    else
    {
        _self->container->flags &=
            ~__CRCL_THDF_ANY_RUNNING;
    }
    return NULL; /* XXX */
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
__charcoal_activity_t *__charcoal_activate( void (*f)( void *args ), void *args )
{
    pthread_attr_t attr;
    int rc;
    if( ( rc = pthread_attr_init( &attr ) ) )
    {
        exit( rc );
    }
    size_t stack_size;
    if( ( rc = pthread_attr_getstacksize( &attr, &stack_size ) ) )
    {
        exit( rc );
    }
    /* TODO: provide some way for the user to pass in a stack */
    void *new_stack = malloc( stack_size );
    if( !new_stack )
    {
        exit( ENOMEM );
    }
    __charcoal_activity_t *act_info = (__charcoal_activity_t *)new_stack;
    act_info->f = f;
    act_info->args = args;
    act_info->flags = 0;
    act_info->container = __charcoal_activity_self()->container;
    if( ( rc = pthread_mutex_lock( &act_info->container->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    ++act_info->container->activities_sz;
    if( ( rc = __charcoal_adjust_activity_cap( act_info->container ) ) )
    {
        exit( rc );
    }
    act_info->container->activities[ act_info->container->activities_sz - 1 ] = act_info;
    if( ( rc = __charcoal_sem_init( &act_info->can_run, 0, 0 ) ) )
    {
        exit( rc );
    }
    /* effective base of stack  */
    size_t actual_activity_sz = sizeof( act_info ) /* XXX: + return type size */;
    /* round up to next multiple of PTHREAD_STACK_MIN */
    actual_activity_sz -= 1;
    actual_activity_sz /= PTHREAD_STACK_MIN;
    actual_activity_sz += 1;
    actual_activity_sz *= PTHREAD_STACK_MIN;
    new_stack += actual_activity_sz;
    if( ( rc = pthread_attr_setstack( &attr, new_stack, stack_size - actual_activity_sz ) ) )
    {
        printf( "pthread_attr_setstack failed! code:%i  err:%s  stk:%p  sz:%i\n",
                rc, strerror( rc ), new_stack, (int)(stack_size - actual_activity_sz) );
        exit( rc );
    }

    /* XXX: What do these do? */
    // int pthread_attr_setdetachstate ( &attr, int detachstate );
    // int pthread_attr_setguardsize   ( &attr, size_t );
    // int pthread_attr_setinheritsched( &attr, int inheritsched);
    // int pthread_attr_setschedparam  ( &attr, const struct sched_param *restrict param);
    // int pthread_attr_setschedpolicy ( &attr, int policy);
    // int pthread_attr_setscope       ( &attr, int contentionscope);

    if( ( rc = pthread_create(
              &act_info->self, &attr, __charcoal_activity_entry_point, act_info ) ) )
    {
        exit( rc );
    }
    if( ( rc = pthread_attr_destroy( &attr ) ) )
    {
        exit( rc );
    }

    /* Whether the newly created activities goes first should probably
     * be controllable. */
    if( ( rc = pthread_mutex_unlock( &act_info->container->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    __charcoal_switch_to( act_info );
    return act_info;
}

int __charcoal_activity_join( __charcoal_activity_t *a )
{
    if( !a )
    {
        return EINVAL;
    }
    int rc;
    __charcoal_activity_t *self = __charcoal_activity_self();
    __charcoal_thread_t *thd = self->container;
    self->flags |= __CRCL_ACTF_BLOCKED;
    __charcoal_activity_t *to;
    if( ( rc = __charcoal_choose_next_activity( &to ) ) )
    {
        exit( rc );
    }
    if( to )
    {
        if( ( rc = __charcoal_sem_post( &to->can_run ) ) )
        {
            exit( rc );
        }
    }

    if( ( rc = pthread_join( a->self, NULL ) ) )
    {
        return rc;
    }
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    free( a );
    /* assert( thd->activities_sz > 0 ) */
    unsigned i, shifting = 0;
    for( i = 0; i < thd->activities_sz; ++i )
    {
        if( shifting )
        {
            thd->activities[ i - 1 ] = thd->activities[ i ];
        }
        if( thd->activities[ i ] == a )
        {
            shifting = 1;
        }
    }
    --thd->activities_sz;
    if( ( rc = __charcoal_adjust_activity_cap( thd ) ) )
    {
        exit( rc );
    }
    /* XXX ! go if noone is going.  Wait otherwise */
    self->flags &= ~__CRCL_ACTF_BLOCKED;
    int wait = !!( thd->flags & __CRCL_THDF_ANY_RUNNING );
    thd->flags |= __CRCL_THDF_ANY_RUNNING;
    if( ( rc = pthread_mutex_unlock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    if( wait && ( rc = __charcoal_sem_wait( &self->can_run ) ) )
    {
        exit( rc );
    }
    return 0;
}

int __charcoal_activity_detach( __charcoal_activity_t *a )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & __CRCL_ACTF_DETACHED )
    {
        /* XXX error? */
    }
    a->flags |= __CRCL_ACTF_DETACHED;
    return 0;
}

static void __charcoal_timer_handler( int sig, siginfo_t *siginfo, void *context )
{
    printf ("Sending PID: %ld, UID: %ld\n",
            (long)siginfo->si_pid, (long)siginfo->si_uid);
}

/* The Charcoal preprocessor will replace all instances of "main" in
 * Charcoal code with "__charcoal_replace_main".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */

int __charcoal_replace_main( int argc, char **argv );

int main( int argc, char **argv )
{
    __charcoal_activity_t __charcoal_main_activity;
    __charcoal_thread_t   __charcoal_main_thread;
    int rc;
    if( ( rc = pthread_key_create( &__charcoal_self_key, NULL /* destructor */ ) ) )
    {
        exit( rc );
    }
    __charcoal_main_activity.f         = (void (*)( void * ))__charcoal_replace_main;
    __charcoal_main_activity.args      = NULL;
    __charcoal_main_activity.flags     = 0;
    __charcoal_main_activity.self      = pthread_self();
    __charcoal_main_activity.container = &__charcoal_main_thread;
    if( ( rc = __charcoal_sem_init( &__charcoal_main_activity.can_run, 0, 0 ) ) )
    {
        exit( rc );
    }
    if( ( rc = pthread_setspecific( __charcoal_self_key, &__charcoal_main_activity ) ) )
    {
        exit( rc );
    }
    __charcoal_main_thread.unyield_depth  = 1;
    __charcoal_main_thread.activities_sz  = 1;
    __charcoal_main_thread.activities_cap = 1;
    __charcoal_main_thread.activities = malloc( sizeof( __charcoal_main_thread.activities[0] ) );
    if( !__charcoal_main_thread.activities )
    {
        exit( ENOMEM );
    }
    __charcoal_main_thread.activities[0] = &__charcoal_main_activity;
    if( ( rc = pthread_mutex_init( &__charcoal_main_thread.thd_management_mtx, NULL ) ) )
    {
        exit( rc );
    }
    __charcoal_main_thread.flags = __CRCL_THDF_ANY_RUNNING;

    // sigaction sigact;
    // sigact.sa_sigaction = __charcoal_timer_handler;
    // sigact.sa_flags = SA_SIGINFO;
    // assert( 0 == sigemptyset( &sigact.sa_mask ) );
    // assert( 0 == sigaction( SIGALRM, &sigact, NULL );

    /* Call the "real" main */
    *((int *)(&__charcoal_main_activity.return_value)) =
        __charcoal_replace_main( argc, argv );

    /* XXX Wait until all activities are done? */

    return *((int *)(&__charcoal_main_activity.return_value));
}
