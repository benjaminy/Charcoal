/*
 * The Charcoal Runtime System
 */

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h> /* XXX remove dep eventually */
#include <stdio.h> /* XXX remove dep eventually */
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <charcoal_runtime.h>

/* Scheduler stuff */

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

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
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
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
    //commented this out for testing purposes, TODO: put back
    //printf( "SWITCH from: %p(%i)  to: %p(%i)\n",
    //        self, sv, to_run, rv );
    if( p )
    {
        *p = to_run;
    }
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    thd->start_time = time(&(thd->timer));
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

void timeout_signal_handler(){
    OPA_store_int(&(__charcoal_activity_self()->container->timeout), 1);
    //printf("signal handler went off\n");
}

int __charcoal_yield(){
    int rv = 0;
    __charcoal_activity_t *foo = __charcoal_activity_self();

    foo->yield_attempts++;
    int unyielding = OPA_load_int( &(foo->container->unyielding) );
    int timeout = OPA_load_int(&foo->container->timeout);
    if( (unyielding == 0) && (timeout==1) )
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

int __charcoal_timeout_yield()
{
    int rv = 0;
    __charcoal_activity_t *foo = __charcoal_activity_self();
    int unyielding = OPA_load_int( &(foo->container->unyielding) );
    assert(unyielding >= 0);
    //double time_difference = (double)(time(&(foo->container->timer)) - foo->container->start_time);
    //printf("Start time: %f\n", (double) foo->container->start_time);
    //printf("Current time: %f\n", (double) time(&(foo->container->timer)));
    //printf("Time difference: %f\n", time_difference);
    int timed_out = (double)(time(&(foo->container->timer)) -
            foo->container->start_time) > foo->container->max_time;
    signal(SIGALRM, timeout_signal_handler);
    //int received_signal = 0; // TODO why exactly do we need signals?
    //printf("Unyield depth: %d\n", unyielding);
    if( unyielding == 0 && timed_out )
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
    /*if (unyielding > 0)
    {
        OPA_decr_int(&foo->container->unyielding);
    }*/
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
    ABORT_ON_FAIL( __charcoal_sem_post( &to->can_run ) );
    ABORT_ON_FAIL( __charcoal_sem_wait( &from->can_run ) );
    from->container->start_time = time(&(from->container->timer));

    alarm((int) to->container->max_time);
    OPA_store_int(&(__charcoal_activity_self()->container->timeout), 0);
    from->yield_attempts = 0;
    //printf("Set timeout value to 0 in charcoal_switch_from_to\n");
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
    //printf( "Starting activity %p\n", _self );
    //REGISTER SIGNAL HANDLER (important)
    signal(SIGALRM, timeout_signal_handler);
    _self->container->start_time = time(&(_self->container->timer));
    ABORT_ON_FAIL( pthread_setspecific( __charcoal_self_key, _self ) );
    ABORT_ON_FAIL( __charcoal_sem_wait( &_self->can_run ) );
    _self->f( _self->args );
    //printf( "Finishing activity %p\n", _self );
    __charcoal_thread_t *thd = _self->container;
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
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

    ABORT_ON_FAIL( __charcoal_choose_next_activity( &next ) );
    if( next )
    {
        ABORT_ON_FAIL( __charcoal_sem_post( &next->can_run ) );
    }
    else
    {
        // _self->container->flags &= ~__CRCL_THDF_ANY_RUNNING;
    }
    --_self->container->runnable_activities;
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    return NULL; /* XXX */
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
__charcoal_activity_t *__charcoal_activate( void (*f)( void *args ), void *args )
{
    pthread_attr_t attr;
    int rc;
    ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    size_t stack_size;
    ABORT_ON_FAIL( pthread_attr_getstacksize( &attr, &stack_size ) );
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
    act_info->yield_attempts = 0;
    alarm((int) act_info->container->max_time);
    OPA_store_int(&(act_info->container->timeout), 0);
    //printf("Setting timeout to 0 in charcoal_activate");
    ABORT_ON_FAIL( pthread_mutex_lock( &act_info->container->thd_management_mtx ) );
    ++act_info->container->activities_sz;
    ABORT_ON_FAIL( __charcoal_adjust_activity_cap( act_info->container ) );
    act_info->container->activities[ act_info->container->activities_sz - 1 ] = act_info;
    ABORT_ON_FAIL( __charcoal_sem_init( &act_info->can_run, 0, 0 ) );
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

    ++act_info->container->runnable_activities;

    ABORT_ON_FAIL( pthread_create(
        &act_info->self, &attr, __charcoal_activity_entry_point, act_info ) );
    ABORT_ON_FAIL( pthread_attr_destroy( &attr ) );

    /* Whether the newly created activities goes first should probably
     * be controllable. */
    ABORT_ON_FAIL( pthread_mutex_unlock( &act_info->container->thd_management_mtx ) );
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
    ABORT_ON_FAIL( __charcoal_choose_next_activity( &to ) );
    if( to )
    {
        ABORT_ON_FAIL( __charcoal_sem_post( &to->can_run ) );
    }

    --thd->runnable_activities;
    if( ( rc = pthread_join( a->self, NULL ) ) )
    {
        return rc;
    }
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
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
    ABORT_ON_FAIL( __charcoal_adjust_activity_cap( thd ) );
    /* XXX ! go if noone is going.  Wait otherwise */
    self->flags &= ~__CRCL_ACTF_BLOCKED;
    // int wait = !!( thd->flags & __CRCL_THDF_ANY_RUNNING );
    int wait = thd->runnable_activities > 0;
    ++thd->runnable_activities;
    // thd->flags |= __CRCL_THDF_ANY_RUNNING;
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
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

//timer that delivers to program periodically
//Signal handler should deliver this to yield
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
    ABORT_ON_FAIL( pthread_key_create( &__charcoal_self_key, NULL /* destructor */ ) );
    __charcoal_main_activity.f         = (void (*)( void * ))__charcoal_replace_main;
    __charcoal_main_activity.args      = NULL;
    __charcoal_main_activity.flags     = 0;
    __charcoal_main_activity.self      = pthread_self();
    __charcoal_main_activity.container = &__charcoal_main_thread;
    ABORT_ON_FAIL( __charcoal_sem_init( &__charcoal_main_activity.can_run, 0, 0 ) );
    ABORT_ON_FAIL( pthread_setspecific( __charcoal_self_key, &__charcoal_main_activity ) );
    
    OPA_store_int(&(__charcoal_main_thread.unyielding), 0);
    __charcoal_main_thread.activities_sz  = 1;
    __charcoal_main_thread.activities_cap = 1;
    __charcoal_main_thread.activities = malloc( sizeof( __charcoal_main_thread.activities[0] ) );
    if( !__charcoal_main_thread.activities )
    {
        exit( ENOMEM );
    }
    __charcoal_main_thread.activities[0] = &__charcoal_main_activity;

    pthread_mutexattr_t attr;
    ABORT_ON_FAIL( pthread_mutexattr_init( &attr ) );
    ABORT_ON_FAIL( pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ) );
    ABORT_ON_FAIL( pthread_mutex_init( &__charcoal_main_thread.thd_management_mtx, &attr ) );
    ABORT_ON_FAIL( pthread_mutexattr_destroy( &attr ) );
    // __charcoal_main_thread.flags = __CRCL_THDF_ANY_RUNNING;
    __charcoal_main_thread.flags = 0;
    __charcoal_main_thread.runnable_activities = 1;

    // sigaction sigact;
    // sigact.sa_sigaction = __charcoal_timer_handler;
    // sigact.sa_flags = SA_SIGINFO;
    // assert( 0 == sigemptyset( &sigact.sa_mask ) );
    // assert( 0 == sigaction( SIGALRM, &sigact, NULL );

    /* Call the "real" main */
    *((int *)(&__charcoal_main_activity.return_value)) =
        __charcoal_replace_main( argc, argv );

    /* XXX Wait until all activities are done? */
    free( __charcoal_main_thread.activities );
    return *((int *)(&__charcoal_main_activity.return_value));
}
