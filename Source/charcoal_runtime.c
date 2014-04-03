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
#include <charcoal_runtime_io_commands.h>

/* Scheduler stuff */

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

static CRCL(thread_t) *CRCL(threads);

#if 0

int CRCL(choose_next_activity)( __charcoal_activity_t **p )
{
    CRCL(activity_t) *to_run, *first, *self = __charcoal_activity_self();
    CRCL(thread_t) *thd = self->container;
    uv_mutex_lock( &thd->thd_management_mtx );
    first = to_run = thd->activities;
    do {
        if( !( to_run->flags & __CRCL_ACTF_BLOCKED )
            && ( to_run != self ) )
        {
            first = NULL;
            break;
        }
        to_run = to_run->next;
    } while( to_run != first );
    /* XXX What is this sv nonsense about??? */
    int rv = -1, sv = uv_sem_trywait( &self->can_run );
    if( !sv )
        uv_sem_post( &self->can_run );
    
    if( first )
    {
        rv = uv_sem_trywait( &to_run->can_run );
        if( !rv )
            uv_sem_post( &to_run->can_run );
    }
    //commented this out for testing purposes, TODO: put back
    //printf( "SWITCH from: %p(%i)  to: %p(%i)\n",
    //        self, sv, to_run, rv );
    if( p )
    {
        *p = to_run;
    }
    uv_mutex_unlock( &thd->thd_management_mtx );
    return 0;
}

static pthread_key_t __charcoal_self_key;

__charcoal_activity_t *__charcoal_activity_self( void )
{
    return (__charcoal_activity_t *)pthread_getspecific( __charcoal_self_key );
}

void __charcoal_unyielding_enter()
{
    __charcoal_activity_t *self = __charcoal_activity_self();
    __charcoal_atomic_incr_int( &self->container->unyielding );
}

void __charcoal_unyielding_exit()
{
    __charcoal_activity_t *self = __charcoal_activity_self();
    __charcoal_atomic_decr_int( &self->container->unyielding );
    /* TODO: Maybe call yield here? */
}

void timeout_signal_handler(){
    __charcoal_atomic_store_int(&(__charcoal_activity_self()->container->timeout), 1);
    //printf("signal handler went off\n");
}

/* This should be called just before an activity starts or resumes from
 * yield/wait. */
static void __charcoal_activity_start_resume( __charcoal_activity_t *self )
{
    /* XXX: enqueue command */
    ABORT_ON_FAIL( uv_async_send( &CRCL(io_cmd) ) );

}

static int __charcoal_yield_try_switch( __charcoal_activity_t *self )
{
    __charcoal_activity_t *to;
    /* XXX error check */
    __charcoal_choose_next_activity( &to );
    if( to )
    {
        uv_sem_post( &to->can_run );
        uv_sem_wait( &self->can_run );
        __charcoal_activity_start_resume( self );
    }
    return 0;
}

int __charcoal_yield(){
    int rv = 0;
    __charcoal_activity_t *foo = __charcoal_activity_self();

    foo->yield_attempts++;
    int unyielding = __charcoal_atomic_load_int( &(foo->container->unyielding) );
    int timeout = __charcoal_atomic_load_int(&foo->container->timeout);
    if( (unyielding == 0) && (timeout==1) )
    {
        return __charcoal_yield_try_switch( foo );
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
    uv_sem_post( &to->can_run );
    /* This will block until we switch back to "from" */
    uv_sem_wait( &from->can_run );

    alarm((int) to->container->max_time);
    __charcoal_atomic_store_int(&(__charcoal_activity_self()->container->timeout), 0);
    from->yield_attempts = 0;
    //printf("Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

void CRCL(switch_to)( CRCL(activity_t) *act )
{
    CRCL(switch_from_to)( CRCL(activity_self)(), act );
}

static void CRCL(remove_activity_from_thread)( CRCL(activity_t) *a, CRCL(thread_t) *t)
{
    if( a == a->next )
    {
        /* XXX Trying to remove the last activity. */
        exit( -1 );
    }
    if( t->activities == a )
    {
        t->activities = a->next;
    }
    a->next->prev = a->prev;
    a->prev->next = a->next;
}

static void CRCL(insert_activity_into_thread)( CRCL(activity_t) *a, CRCL(thread_t) *t)
{
    CRCL(activity_t) *prev = t->activities->prev;
    prev->next = a;
    a->prev = prev;
    a->next = t->activities;
    t->activities->prev = a;
}

int CRCL(activity_join)( CRCL(activity_t) *a )
{
    if( !a )
    {
        return EINVAL;
    }
    int rc;
    CRCL(activity_t) *self = CRCL(activity_self)();
    CRCL(thread_t) *thd = self->container;
    self->flags |= __CRCL_ACTF_BLOCKED;
    CRCL(activity_t) *to;
    ABORT_ON_FAIL( CRCL(choose_next_activity)( &to ) );
    if( to )
    {
        uv_sem_post( &to->can_run );
    }

    --thd->runnable_activities;
    if( ( rc = pthread_join( a->self, NULL ) ) )
    {
        return rc;
    }
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    /* I'm pretty sure we don't want this free anymore */
    free( a );
    /* assert( thd->activities_sz > 0 ) */
    CRCL(remove_activity_from_thread)( a, thd );
    /* XXX ! go if noone is going.  Wait otherwise */
    self->flags &= ~__CRCL_ACTF_BLOCKED;
    // int wait = !!( thd->flags & __CRCL_THDF_ANY_RUNNING );
    int wait = thd->runnable_activities > 0;
    ++thd->runnable_activities;
    // thd->flags |= __CRCL_THDF_ANY_RUNNING;
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    if( wait )
    {
        uv_sem_wait( &self->can_run );
    }
    return 0;
}

int CRCL(activity_detach)( CRCL(activity_t) *a )
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

static void CRCL(thread_entry)( void *p )
{
}

/* This is the entry procedure for new activities */
static void CRCL(activity_entry)( void *p )
{
    CRCL(activity_t) *next, *_selfp, _self;
    _selfp = (CRCL(activity_t) *)p;
    /* copy to local memory */
    _self = *_selfp;
    //printf( "Starting activity %p\n", _self );
    ABORT_ON_FAIL( pthread_setspecific( CRCL(self_key), &_self ) );

    CRCL(thread_t) *thd = _self.container;
    uv_mutex_lock( &thd->thd_management_mtx );
    _selfp->f = NULL;
    _selfp->args = &_self;
    uv_mutex_unlock( &thd->thd_management_mtx );


    uv_sem_wait( &_self.can_run );
    CRCL(activity_start_resume)( &_self );
    /* Do the actual activity */
    _self.f( _self.args );
    //printf( "Finishing activity %p\n", _self );
    uv_mutex_lock( &thd->thd_management_mtx );
    if( _self.flags & __CRCL_ACTF_DETACHED )
    {
        /* deallocate activity */
    }
    else
    {
        /* blah */
    }
    _self.flags |= __CRCL_ACTF_BLOCKED;
    /* Who knows if another activity is joining? */

    ABORT_ON_FAIL( CRCL(choose_next_activity)( &next ) );
    if( next )
    {
        uv_sem_post( &next->can_run );
    }
    else
    {
        // _self.container->flags &= ~__CRCL_THDF_ANY_RUNNING;
    }
    --_self.container->runnable_activities;
    uv_mutex_unlock( &thd->thd_management_mtx );
}

#endif



/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "__charcoal_replace_main".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */


typedef struct CRCL(thread_launch_ctx) CRCL(thread_launch_ctx);

struct CRCL(thread_launch_ctx)
{
    void (*entry)( void * );
    void *p;
    CRCL(activity_t) *act;
};


CRCL(activity_t) *CRCL(activate_in_thread)(
    CRCL(thread_t) *thd, CRCL(activity_t) *act, CRCL(entry_t) f, void *args )
{
    // int rc;
    /* TODO: provide some way for the user to pass in a stack */
    // void *new_stack = malloc( stack_size );
    // if( !new_stack )
    // {
    //     exit( ENOMEM );
    // }
    // __charcoal_activity_t *act_info = (__charcoal_activity_t *)new_stack;
    CRCL(thread_launch_ctx) ctx;
    ctx.entry = f;
    ctx.p = args;
    ctx.act = act;
    act->flags = 0;
    act->container = thd;
    act->yield_attempts = 0;
    alarm((int) act->container->max_time);
    CRCL(atomic_store_int)(&(act->container->timeout), 0);
    //printf("Setting timeout to 0 in charcoal_activate");
    uv_mutex_lock( &act->container->thd_management_mtx );
    CRCL(insert_activity_into_thread)( act, act->container );
    ABORT_ON_FAIL( uv_sem_init( &act->can_run, 0 ) );
    /* effective base of stack  */
    // size_t actual_activity_sz = sizeof( act_info_local ) /* XXX: + return type size */;
    /* round up to next multiple of PTHREAD_STACK_MIN */
    // actual_activity_sz -= 1;
    // actual_activity_sz /= PTHREAD_STACK_MIN;
    // actual_activity_sz += 1;
    // actual_activity_sz *= PTHREAD_STACK_MIN;
    // new_stack += actual_activity_sz;
    // if( ( rc = pthread_attr_setstack( &attr, new_stack, stack_size - actual_activity_sz ) ) )
    // {
    //     printf( "pthread_attr_setstack failed! code:%i  err:%s  stk:%p  sz:%i\n",
    //             rc, strerror( rc ), new_stack, (int)(stack_size - actual_activity_sz) );
    //     exit( rc );
    // }


    ++act_info_local.container->runnable_activities;

    ABORT_ON_FAIL( pthread_create(
        &act_info_local.self, CRCL(activity_entry), &act_info_local ) );
    // ABORT_ON_FAIL( pthread_attr_destroy( &attr ) );

    
    CRCL(activity_t) *act_info;
    while( act_info_local.f )
    {
        uv_cond_wait( &act_info_local.container->thd_management_cond,
                      &act_info_local.container->thd_management_mtx );
        act_info = (CRCL(activity_t)*)act_info_local.args;
    }

    /* Whether the newly created activities goes first should probably
     * be controllable. */
    uv_mutex_unlock( &act_info_local.container->thd_management_mtx );
    CRCL(switch_to)( &act_info_local );
    return act_info;
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
CRCL(activity_t) *CRCL(activate)( CRCL(activity_t) *act, CRCL(entry_t) f, void *args )
{
    return CRCL(activate_in_thread)(
        CRCL(activity_self)()->container, act, f, args );
}

static void CRCL(add_to_threads)( CRCL(thread_t) *thd )
{
    CRCL(thread_t) *last = CRCL(threads)->prev;
    last->next = thd;
    CRCL(threads)->prev = thd;
    thd->next = CRCL(threads);
    thd->prev = last;
}

static void *CRCL(thread_entry)( void *p )
{
    CRCL(thread_launch_ctx) ctx = *((CRCL(thread_launch_ctx)*)p);
    
    CRCL(activate_in_thread)( ctx.act->container, ctx.act, ctx.entry, ctx.p );
    /* XXX Wait for all activities in this thread to be done.
     * Maybe do the reaping here. */
    while( 0 )
    {
    }
    return NULL;
}

/* NOTE: Launching a new thread needs storage for thread stuff and
 * activity stuff. */
static int CRCL(create_charcoal_thread)(
    CRCL(thread_t) *thd, CRCL(activity_t) *act,
    void (*entry_fn)( void *formal_param ), void *actual_param )
{
    assert( thd );
    CRCL(add_to_threads)( thd );
    CRCL(atomic_store_int)( &thd->unyielding, 0 );
    CRCL(atomic_store_int)( &thd->timeout, 0 );
    thd->timer_req.data = thd;
    /* XXX Does timer_init have to be called from the I/O thread? */
    uv_timer_init( CRCL(io_loop), &thd->timer_req );
    thd->start_time = 0.0;
    thd->max_time = 0.0;

    ABORT_ON_FAIL( uv_mutex_init( &thd->thd_management_mtx ) );
    ABORT_ON_FAIL(  uv_cond_init( &thd->thd_management_cond ) );
    // thd->flags = __CRCL_THDF_ANY_RUNNING;
    thd->flags = 0;
    thd->runnable_activities = 1;
    act->container = thd;

    // pthread_attr_t attr;
    // ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    // size_t stack_size;
    // ABORT_ON_FAIL( pthread_attr_getstacksize( &attr, &stack_size ) );

    /* XXX: What do these do? */
    // int pthread_attr_setdetachstate ( &attr, int detachstate );
    // int pthread_attr_setguardsize   ( &attr, size_t );
    // int pthread_attr_setinheritsched( &attr, int inheritsched);
    // int pthread_attr_setschedparam  ( &attr, const struct sched_param *restrict param);
    // int pthread_attr_setschedpolicy ( &attr, int policy);
    // int pthread_attr_setscope       ( &attr, int contentionscope);


    /* Maybe make a fixed pool of these ctxs to avoid malloc */
    CRCL(thread_launch_ctx) *ctx = (CRCL(thread_launch_ctx) *)malloc( sizeof( ctx[0] ) );
    if( !ctx )
    {
        return ENOMEM;
    }
    ctx->entry = entry_fn;
    ctx->p     = actual_param;
    ctx->act   = act;

    /* XXX make the stack much smaller.  The inital activity in any
     * charcoal thread just sits around waiting for all the other
     * activities to finish.  Its stack does not need to be very big. */
    ABORT_ON_FAIL( pthread_create(
                       &thd->self, NULL /*XXX*/, CRCL(thread_entry), ctx ) );

    return 0;
}


/*
 * From this point on in this file is stuff related to the main
 * procedure.
 *
 * The initial thread that exists when a Charcoal program starts will
 * be the I/O thread.  Before it starts its I/O duties it launches
 * another thread that will call the application's main procedure.
 */
int CRCL(replace_main)( int argc, char **argv, char **env );

typedef struct
{
    int argc;
    char **argv, **env;
} CRCL(main_params);

static int CRCL(process_return_value);

static void CRCL(main_thread_entry)( void *p )
{
    struct CRCL(main_params) main_params = *((struct CRCL(main_params) *)p);
    int    argc = main_params.argc;
    char **argv = main_params.argv;
    char **env  = main_params.env;
    CRCL(process_return_value) = CRCL(replace_main)( argc, argv, env );
}

int main( int argc, char **argv, char **env )
{
    /* Okay to stack-allocate these here because the I/O thread should
     * always be the last thing running in a Charcoal process. */
    CRCL(thread_t)   io_thread;
    CRCL(activity_t) io_activity;
    CRCL(threads) = &io_thread;

    ABORT_ON_FAIL( pthread_key_create( &CRCL(self_key), NULL /* XXX destructor */ ) );

    /* Most of the thread and activity fields are not relevant to the I/O thread */
    io_thread.activities     = &io_activity;
    io_thread.next           = CRCL(threads);
    io_thread.prev           = CRCL(threads);
    /* XXX Not sure about the types of self: */
    io_thread.self           = pthread_self();
    io_activity.container    = &io_thread;

    CRCL(io_loop) = uv_default_loop();
    ABORT_ON_FAIL(
        uv_async_init( CRCL(io_loop), &CRCL(io_cmd), CRCL(io_cmd_cb) ) );

    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    CRCL(thread_t) thd = (CRCL(thread_t)*)malloc( sizeof( thd[0] ) );
    if( !thd )
    {
        return ENOMEM;
    }
    CRCL(activity_t) act = (CRCL(activity_t)*)malloc( sizeof( act[0] ) );
    if( !act )
    {
        return ENOMEM;
    }

    /* It's slightly wasteful to stack-allocate these, because they only
     * need to exist until the application's main is actually called.
     * But really, it's only a handful of bytes. */
    CRCL(main_params) params;
    params.argc = argc;
    params.argv = argv;
    params.env  = env;

    CRCL(create_charcoal_thread)( thd, act, CRCL(main_thread_entry), &params );

    /* XXX Someone's going to have to tell the loop to stop at some
     * point. */
    int rc = uv_run( CRCL(io_loop), UV_RUN_DEFAULT );
    if( rc )
    {
        printf( "%i", rc );
        exit( rc );
    }
    return CRCL(process_return_value);
}
