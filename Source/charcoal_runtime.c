/*
 * The Charcoal Runtime System
 */

#include <charcoal_base.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h> /* XXX remove dep eventually */
#include <stdio.h> /* XXX remove dep eventually */
#include <errno.h>
#include <limits.h>
#include <charcoal_runtime.h>
#include <charcoal_runtime_io_commands.h>

/* Scheduler stuff */

#define RET_IF_ERROR(cmd) \
    do { int rc; if( ( rc = cmd ) ) { return rc; } } while( 0 )

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

static CRCL(thread_t) *CRCL(threads);
static pthread_key_t CRCL(self_key);
static timer_t CRCL(heartbeat_timer);

CRCL(activity_t) *CRCL(get_self_activity)( void )
{
    return (CRCL(activity_t) *)pthread_getspecific( CRCL(self_key) );
}

static int CRCL(start_stop_heartbeat)( int start )
{
    /* XXX I think 'its' can be stack alloc'ed.  Check this. */
    struct itimerspec its;
    /* XXX Think about what the interval should be. */
    long long freq_nanosecs;
    if( start )
    {
        freq_nanosecs = 10*1000*1000;
    }
    else
    {
        freq_nanosecs = 0;
    }
    its.it_value.tv_sec     = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec    = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec  = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    RET_IF_ERROR( timer_settime( CRCL(heartbeat_timer), 0, &its, NULL ) );
    return 0;
}

int CRCL(choose_next_activity)( __charcoal_activity_t **p )
{
    CRCL(activity_t) *to_run, *first, *self = CRCL(get_self_activity)();
    CRCL(thread_t) *thd = self->container;
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
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
    int rv = -1, sv = CRCL(sem_try_decr)( &self->can_run );
    if( !sv )
        CRCL(sem_incr)( &self->can_run );

    if( first )
    {
        rv = CRCL(sem_try_decr)( &to_run->can_run );
        if( !rv )
            CRCL(sem_incr)( &to_run->can_run );
    }
    //commented this out for testing purposes, TODO: put back
    //printf( "SWITCH from: %p(%i)  to: %p(%i)\n",
    //        self, sv, to_run, rv );
    if( p )
    {
        *p = to_run;
    }
    pthread_mutex_unlock( &thd->thd_management_mtx );
    return 0;
}

void CRCL(unyielding_enter)()
{
    CRCL(activity_t) *self = CRCL(get_self_activity)();
    CRCL(atomic_incr_int)( &self->container->unyielding );
}

void CRCL(unyielding_exit)()
{
    CRCL(activity_t) *self = CRCL(get_self_activity)();
    CRCL(atomic_decr_int)( &self->container->unyielding );
    /* TODO: Maybe call yield here? */
}

void timeout_signal_handler(){
    __charcoal_atomic_store_int(&(CRCL(get_self_activity)()->container->timeout), 1);
    //printf("signal handler went off\n");
}

/* This should be called just before an activity starts or resumes from
 * yield/wait. */
static void CRCL(activity_start_resume)( CRCL(activity_t) *self )
{
    /* XXX: enqueue command */
    /* XXX: start heartbeat if runnable > 1 */
    ABORT_ON_FAIL( uv_async_send( &CRCL(io_cmd) ) );
    assert( !pthread_setspecific( CRCL(self_key), self ) );
    alarm((int) self->container->max_time);
    CRCL(atomic_store_int)(&self->container->timeout, 0);
    self->yield_attempts = 0;
    /* XXX: Lots to fix here. */
}

typedef void (*CRCL(switch_listener))(CRCL(activity_t) *from, CRCL(activity_t) *to, void *ctx);

void CRCL(push_yield_switch_listener)( CRCL(switch_listener) pre, void *pre_ctx, CRCL(switch_listener) post, void *post_ctx )
{
}

void CRCL(pop_yield_switch_listener)()
{
}

/* XXX must fix.  Still assuming activities implemented as threads */
static int CRCL(yield_try_switch)( CRCL(activity_t) *self )
{
    CRCL(activity_t) *to;
    /* XXX error check */
    CRCL(choose_next_activity)( &to );
    if( to )
    {
        CRCL(sem_incr)( &to->can_run );
        CRCL(sem_decr)( &self->can_run );
        CRCL(activity_start_resume)( self );
    }
    return 0;
}

void CRCL(yield)(){
    __charcoal_activity_t *foo = CRCL(get_self_activity)();

    /* DEBUG foo->yield_attempts++; */
    int unyielding = __charcoal_atomic_load_int( &(foo->container->unyielding) );
    if(unyielding == 0 )
    {
        /* ) && (timeout==1) */
        __charcoal_yield_try_switch( foo );
    }
}

static void CRCL(print_special_queue)( CRCL(activity_t) **q )
{
    assert( q );
    CRCL(activity_t) *a = *q, *first = a;
    if( a )
    {
        printf( "Special queue: " );
        do {
            printf( "%p  ", a );
            a = a->snext;
        } while( a != first );
        printf( "\n" );
    }
    else
    {
        printf( "Special queue empty\n" );
    }
}

/* Precondition: The thread mgmt mutex is held. */
static CRCL(activity_t) *CRCL(pop_special_queue)(
    unsigned queue_flag, CRCL(thread_t) *t )
{
    CRCL(activity_t) **q = NULL;
    switch( queue_flag )
    {
    case __CRCL_ACTF_IN_READY_QUEUE:
        q = &t->ready;
        break;
    case __CRCL_ACTF_IN_REAP_QUEUE:
        q = &t->to_be_reaped;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-pop pre " ); */
    /* CRCL(print_special_queue)( q ); */
    if( *q )
    {
        CRCL(activity_t) *a = *q;
        if( a->snext == a )
        {
            *q = NULL;
        }
        else
        {
            a->snext->sprev = a->sprev;
            a->sprev->snext = a->snext;
            *q = a->snext;
        }
        a->flags &= ~queue_flag;
        return a;
    }
    else
    {
        return NULL;
    }
    /* printf( "S-pop post " ); */
    /* CRCL(print_special_queue)( q ); */
}

/* Precondition: The thread mgmt mutex is held. */
static void CRCL(push_special_queue)(
    unsigned queue_flag, CRCL(thread_t) *t, CRCL(activity_t) *a )
{
    if( a->flags & queue_flag )
    {
        /* printf( "Already in queue\n" ); */
        return;
    }
    a->flags |= queue_flag;
    CRCL(activity_t) **q = NULL;
    switch( queue_flag )
    {
    case __CRCL_ACTF_IN_READY_QUEUE:
        q = &t->ready;
        break;
    case __CRCL_ACTF_IN_REAP_QUEUE:
        q = &t->to_be_reaped;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-push pre q:%p %p ", q, a ); */
    /* CRCL(print_special_queue)( q ); */
    if( *q )
    {
        CRCL(activity_t) *front = *q, *rear = front->sprev;
        /* printf( "Front: %p(n:%p p:%p)  Rear:%p(n:%p p:%p)\n", */
        /*         front, front->snext, front->sprev, */
        /*         rear , rear ->snext, rear ->sprev ); */
        rear->snext = a;
        front->sprev = a;
        a->snext = front;
        a->sprev = rear;
    }
    else
    {
        *q = a;
        a->snext = a;
        a->sprev = a;
    }
    /* printf( "S-push post q:%p %p ", q, a ); */
    /* CRCL(print_special_queue)( q ); */
}

/* activity_blocked should be called when an activity has nothing to do
 * right now.  Another activity or thread might switch back to it later.
 * If another activity is ready, run it.  Otherwise wait for a condition
 * variable signal from another thread (typically the I/O thread).
 *
 * Should the calling code call _setjmp, or should that happen in
 * here? */
static int CRCL(activity_blocked)( CRCL(activity_t) *self )
{
    int rc;
    /* printf( "Actvity blocked\n" ); */
    CRCL(thread_t) *thd = self->container;
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        return rc;
    }
    while( !thd->ready )
    {
        if( ( rc = pthread_cond_wait( &thd->thd_management_cond,
                                      &thd->thd_management_mtx ) ) )
        {
            return rc;
        }
    }
    CRCL(activity_t) *to = CRCL(pop_special_queue)(
        __CRCL_ACTF_IN_READY_QUEUE, thd );
    if( ( rc = pthread_mutex_unlock( &thd->thd_management_mtx ) ) )
    {
        return rc;
    }
    if( !setjmp( self->jmp ) )
    {
        CRCL(activity_start_resume)( to );
        _longjmp( to->jmp, 1 );
    }
    return 0;
}

void CRCL(activity_set_return_value)( CRCL(activity_t) *a, void *ret_val_ptr )
{
    memcpy( a->return_value, ret_val_ptr, a->ret_size );
}

void CRCL(activity_get_return_value)( CRCL(activity_t) *a, void **ret_val_ptr )
{
    memcpy( ret_val_ptr, a->return_value, a->ret_size );
}

void CRCL(switch_from_to)( CRCL(activity_t) *from, CRCL(activity_t) *to )
{
    int rc;
    /* XXX assert from is the currently running activity? */
    CRCL(thread_t) *thd = from->container;
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        printf( "ERRR from: %p thd: %p %s\n", from, thd, strerror( rc ) ); fflush( stdout );
        exit( rc );
    }
    CRCL(push_special_queue)( __CRCL_ACTF_IN_READY_QUEUE, thd, from );
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    if( _setjmp( from->jmp ) == 0 )
    {
        CRCL(activity_start_resume)( to );
        _longjmp( to->jmp, 1 );
    }
    //printf("Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

void CRCL(switch_to)( CRCL(activity_t) *act )
{
    CRCL(switch_from_to)( CRCL(get_self_activity)(), act );
}

static void CRCL(remove_activity_from_thread)(
    CRCL(activity_t) *a, CRCL(thread_t) *t)
{
    /* printf( "Remove activity %p  %p\n", a, t ); */
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
    // printf( "f:%p  r:%p\n", t->activities, t->activities->prev );
}

static void CRCL(insert_activity_into_thread)(
    CRCL(activity_t) *a, CRCL(thread_t) *t )
{
    /* printf( "Insert activity %p  %p\n", a, t ); */
    if( t->activities )
    {
        CRCL(activity_t) *rear = t->activities->prev;
        /* printf( "f:%p  r:%p\n", t->activities, rear ); */
        rear->next = a;
        t->activities->prev = a;
        a->prev = rear;
        a->next = t->activities;
    }
    else
    {
        t->activities = a;
    }
}

/* XXX remove problem!!! */
int CRCL(activity_join)( CRCL(activity_t) *a )
{
    if( !a )
    {
        return EINVAL;
    }
    /* XXX int rc; */
    CRCL(activity_t) *self = CRCL(get_self_activity)();
    CRCL(thread_t) *thd = self->container;
    self->flags |= __CRCL_ACTF_BLOCKED;
    CRCL(activity_t) *to;
    ABORT_ON_FAIL( CRCL(choose_next_activity)( &to ) );
    if( to )
    {
        CRCL(sem_incr)( &to->can_run );
    }

    --thd->runnable_activities;
    /* XXX FIXME */
    /* if( ( rc = pthread_join( a->self, NULL ) ) ) */
    /* { */
    /*     return rc; */
    /* } */
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
    CRCL(sem_decr)( &self->can_run );
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

typedef struct CRCL(thread_launch_ctx) CRCL(thread_launch_ctx);

struct CRCL(thread_launch_ctx)
{
    void            (*entry)( void * );
    void             *p;
    CRCL(activity_t) *act;
    ucontext_t       *prv;
};

/* Never returns */
static void CRCL(activity_finished)( CRCL(activity_t) *a )
{
    int rc;
    CRCL(thread_t) *thd = a->container;
    /* printf( "Finishing %p\n", a ); */
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    CRCL(activity_t) *reaper_act = a->container->activities;
    CRCL(push_special_queue)( __CRCL_ACTF_IN_READY_QUEUE, thd, reaper_act );
    CRCL(push_special_queue)( __CRCL_ACTF_IN_REAP_QUEUE, thd, a );
    CRCL(activity_t) *next = CRCL(pop_special_queue)(
        __CRCL_ACTF_IN_READY_QUEUE, thd );
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    /* printf( "Jumping to %p\n", next ); */
    CRCL(activity_start_resume)( next );
    _longjmp( next->jmp, 1 );
    exit( 1 );
}

/* This is the entry procedure for new activities */
static void CRCL(activity_entry)( void *p )
{
    CRCL(thread_launch_ctx) ctx = *(CRCL(thread_launch_ctx) *)p;
    /* printf( "Starting activity %p(thd:%p)\n", ctx.act, ctx.act->container ); */

    /* XXX activity migration between threads will cause problems */
    /* XXX CRCL(thread_t) *thd = ctx.act->container; */

    /* Wacky code alert!  We swapcontext back to the activity that
     * created this one, after doing a setjmp here.  I'm not 100% sure
     * why it's important to do this swapcontext when "normal"
     * activity switching can be done with longjmp, but it seems to
     * be. */
    if( _setjmp( ctx.act->jmp ) == 0 )
    {
        ucontext_t tmp;
        swapcontext( &tmp, ctx.prv );
    }
    /* LOG: Actual activity starting. */
    ctx.entry( ctx.p );
    /* LOG: Actual activity complete. */
    assert( ctx.act == CRCL(get_self_activity)() );
    CRCL(activity_finished)( ctx.act );
    exit( -1 );
}

/* static ucontext_t WTF; */

int CRCL(activate_in_thread)(CRCL(thread_t) *thd, CRCL(activity_t) *act,
        CRCL(entry_t) f, void *args )
{
    /* TODO: provide some way for the user to pass in a stack */
    act->flags = 0;
    act->container = thd;
    act->yield_attempts = 0;
    act->snext = act->sprev = NULL;
    alarm((int) act->container->max_time);
    CRCL(atomic_store_int)(&(act->container->timeout), 0);
    //printf("Setting timeout to 0 in charcoal_activate");
    pthread_mutex_lock( &thd->thd_management_mtx );
    CRCL(insert_activity_into_thread)( act, thd );

    ABORT_ON_FAIL( getcontext( &act->ctx ) );
    // ABORT_ON_FAIL( getcontext( &WTF ) );
    /* I hope the minimum stack size is actually bigger, but this is
     * fine for now. */
    act->ctx.uc_stack.ss_size = MINSIGSTKSZ*42;
    /* printf( "MER??? %d\n", (int)act->ctx.uc_stack.ss_size ); */
    act->ctx.uc_stack.ss_sp = malloc( act->ctx.uc_stack.ss_size );
    /* printf( "DER!\n" ); */
    if( !act->ctx.uc_stack.ss_sp )
    {
        return( ENOMEM );
    }
    act->ctx.uc_stack.ss_flags = 0; /* SA_DISABLE and/or SA_ONSTACK */
    act->ctx.uc_link = NULL; /* XXX fix. */
    // act->ctx.uc_sigmask = 0; /* XXX sigset_t */

    ++thd->runnable_activities;

    ucontext_t tmp;
    CRCL(thread_launch_ctx) ctx;
    ctx.prv   = &tmp;
    ctx.entry = f;
    ctx.p     = args;
    ctx.act   = act;

    CRCL(activity_t) *real_self = CRCL(get_self_activity)();

    /* printf( "A %p, %p  %p\n", real_self, real_self->container, act ); */

    void (*entry)(void) = (void(*)(void))CRCL(activity_entry);
    makecontext( &act->ctx, entry, 1, &ctx );

    /* printf( "B %p, %p\n", real_self, real_self->container ); */

    CRCL(activity_start_resume)( act );
    swapcontext( &tmp, &act->ctx );
    CRCL(activity_start_resume)( real_self );

    /* printf( "C %p, %p\n", real_self, real_self->container ); */

    pthread_mutex_unlock( &thd->thd_management_mtx );
    /* Whether the newly created activities goes first should probably
     * be controllable. */
    CRCL(switch_to)( act );
    return 0;
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
int CRCL(activate)( CRCL(activity_t) *act, CRCL(entry_t) f, void *args )
{
    return CRCL(activate_in_thread)(
        CRCL(get_self_activity)()->container, act, f, args );
}

static void CRCL(add_to_threads)( CRCL(thread_t) *thd )
{
    CRCL(thread_t) *last = CRCL(threads)->prev;
    last->next = thd;
    CRCL(threads)->prev = thd;
    thd->next = CRCL(threads);
    thd->prev = last;
}

int CRCL(join_thread)( CRCL(thread_t) *t )
{
    /* printf( "Joining thread!!!\n" ); */
    ABORT_ON_FAIL( pthread_join( t->sys, NULL ) );
    if( t == t->next )
    {
        /* XXX Trying to remove the last thread. */
        exit( 1 );
    }
    if( t == CRCL(threads) )
    {
        /* XXX Trying to remove the I/O thread. */
        exit( 1 );
    }
    t->next->prev = t->prev;
    t->prev->next = t->next;
    return CRCL(threads) == CRCL(threads)->next;
}

static void CRCL(report_thread_done)( CRCL(activity_t) *a )
{
    /* printf( "XXX Thread is done!!!\n" ); */
}

static void CRCL(reap_activities)( CRCL(activity_t) *a )
{
    CRCL(thread_t) *thd = a->container;
    assert( thd->activities == a );
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    /* XXX More efficient to manage a pool of stacks, rather than
     * allocating and freeing every time. */
    while( 1 )
    {
        /* printf( "Reaping (reaper:%p) %p\n", a, thd->to_be_reaped ); */
        while( thd->to_be_reaped )
        {
            CRCL(activity_t) *r = CRCL(pop_special_queue)(
                __CRCL_ACTF_IN_REAP_QUEUE, thd );
            /* XXX */
            /* printf( "Reaping activity %p\n", r ); */
            CRCL(remove_activity_from_thread)( r, thd );
            free( r->ctx.uc_stack.ss_sp );
        }

        if( thd->activities->next == thd->activities )
        {
            /* printf( "Break??? %p n:%p p:%p\n", thd->activities, */
            /*         thd->activities->next, thd->activities->prev  ); */
            break;
        }
        /* XXX Maybe just continue holding the lock and change "blocked" */
        /* ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) ); */
        if( pthread_mutex_unlock( &thd->thd_management_mtx ) )
        {
            printf( "HOLY SHITBALLS\n" );
            exit( 111 );
        }
        CRCL(activity_blocked)( a );
        /* printf( "The reaper is back!!! %p\n", a ); */
        ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    }
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    CRCL(report_thread_done)( a );
}

static void *CRCL(thread_entry)( void *p )
{
    CRCL(thread_launch_ctx) ctx = *((CRCL(thread_launch_ctx)*)p);
    free( p );
    CRCL(activity_t) *client_act = ctx.act;
    CRCL(thread_t) *thd = client_act->container;
    /* printf( "CLIENT act %p\n", client_act ); */
    CRCL(activity_t) reaper_act;
    reaper_act.yield_attempts = 0;
    assert( thd );
    reaper_act.container = thd;
    /* printf( "reaper: %p  container: %p\n", &reaper_act, thd ); fflush( stdout ); */
    /* XXX  init can-run */
    reaper_act.flags = 0;
    reaper_act.next = &reaper_act;
    reaper_act.prev = &reaper_act;
    reaper_act.snext = NULL;
    reaper_act.sprev = NULL;
    if( getcontext( &reaper_act.ctx ) )
    {
        ABORT_ON_FAIL( 43 );
    }

    pthread_mutex_lock( &thd->thd_management_mtx );
    thd->activities = &reaper_act;
    pthread_mutex_unlock( &thd->thd_management_mtx );
    pthread_cond_signal( &thd->thd_management_cond );

    CRCL(activity_start_resume)( &reaper_act );
    CRCL(activate_in_thread)( thd, client_act, ctx.entry, ctx.p );
    CRCL(reap_activities)( &reaper_act );
    assert( thd->activities->next == &reaper_act );
    CRCL(io_cmd_t) *cmd = (CRCL(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    cmd->command = __CRCL_IO_CMD_JOIN_THREAD;
    cmd->_.thread = thd;
    enqueue( cmd );
    ABORT_ON_FAIL( uv_async_send( &CRCL(io_cmd) ) );
    /* printf( "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
    return NULL;
}

/* NOTE: Launching a new thread needs storage for thread stuff and
 * activity stuff. */
static int CRCL(create_thread)(
    CRCL(thread_t) *thd, CRCL(activity_t) *act, CRCL(entry_t) entry_fn, void *actuals )
{
    int rc;
    assert( thd );
    assert( act );
    CRCL(add_to_threads)( thd );

    CRCL(atomic_store_int)( &thd->unyielding, 0 );
    CRCL(atomic_store_int)( &thd->timeout, 0 );
    thd->timer_req.data = thd;
    /* XXX Does timer_init have to be called from the I/O thread? */
    uv_timer_init( CRCL(io_loop), &thd->timer_req );
    thd->start_time = 0.0;
    thd->max_time = 0.0;

    RET_IF_ERROR( pthread_mutex_init( &thd->thd_management_mtx, NULL /* XXX attrs? */ ) );
    RET_IF_ERROR( pthread_cond_init( &thd->thd_management_cond, NULL /* XXX attrs? */ ) );

    thd->flags               = 0; // __CRCL_THDF_ANY_RUNNING
    thd->runnable_activities = 1;
    thd->activities          = NULL;
    thd->ready               = NULL;
    thd->to_be_reaped        = NULL;
    act->container           = thd;

    pthread_attr_t attr;
    ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    /* The inital activity in any charcoal thread just sits around
     * waiting for all the other activities to finish.  Its stack does
     * not need to be very big.  In fact, PTHREAD_STACK_MIN might be on
     * the big side... TODO using something even smaller. */
    ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN *42) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    /* Maybe make a fixed pool of these ctxs to avoid malloc */
    CRCL(thread_launch_ctx) *ctx = (CRCL(thread_launch_ctx) *)malloc( sizeof( ctx[0] ) );
    if( !ctx )
    {
        return ENOMEM;
    }
    ctx->entry = entry_fn;
    ctx->p     = actuals;
    ctx->act   = act;

    if( ( rc = pthread_create(
              &thd->sys, &attr, CRCL(thread_entry), ctx ) ) )
    {
        return rc;
    }
    if( ( rc = pthread_attr_destroy( &attr ) ) )
    {
        return rc;
    }

    /* Wait for the new thread to complete initialization. */
    pthread_mutex_lock( &thd->thd_management_mtx );
    while( !thd->activities )
    {
        if( ( rc = pthread_cond_wait( &thd->thd_management_cond,
                                      &thd->thd_management_mtx ) ) )
        {
            return rc;
        }
    }
    pthread_mutex_unlock( &thd->thd_management_mtx );
    return 0;
}

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

/* yield_heartbeat is the signal handler that should run every few
 * milliseconds (give or take) when any activity is running.  It
 * atomically modifies the activity state so that the next call to
 * yield will do a more thorough check to see if it should switch to
 * another activity.
 *
 * XXX Actually, this only really needs to be armed if there are any
 * threads with more than one ready (or running) activity. */
static void CRCL(yield_heartbeat)( int sig, siginfo_t *info, void *uc )
{
    if( sig != SIG )
    {
        /* XXX Very weird */
        exit( sig );
    }

    void *p = info->si_value.sival_ptr;
    if( p == &CRCL(threads) )
    {
        /* XXX Is it worth worrying about a weird address collision
         * here? */
        /* XXX disarm if no activities are running */
        /* XXX For all threads: check flags, decr unyielding */
    }
    else
    {
        /* XXX pass the signal on to the application? */
        exit( -sig );
    }
}

static int CRCL(init_yield_heartbeat)()
{
    /* Establish handler for timer signal */
    struct sigaction sa;
    /* printf("Establishing handler for signal %d\n", SIG); */
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = CRCL(yield_heartbeat);
    sigemptyset( &sa.sa_mask );
    RET_IF_ERROR( sigaction( SIG, &sa, NULL ) );

    /* Create the timer */
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIG;
    sev.sigev_value.sival_ptr = &CRCL(threads);
    RET_IF_ERROR( timer_create( CLOCKID, &sev, &CRCL(heartbeat_timer) ) );

    /* printf( "timer ID is 0x%lx\n", (long) CRCL(heartbeat_timer) ); */
    return 0;
}

static int CRCL(init_io_loop)( CRCL(thread_t) *t, CRCL(activity_t) *a )
{
    CRCL(threads) = t;

    RET_IF_ERROR(
        pthread_key_create( &CRCL(self_key), NULL /* XXX destructor */ ) );

    /* Most of the thread and activity fields are not relevant to the I/O thread */
    t->activities     = a;
    t->ready          = NULL;
    t->to_be_reaped   = NULL;
    t->next           = CRCL(threads);
    t->prev           = CRCL(threads);
    /* XXX Not sure about the types of self: */
    t->sys            = pthread_self();
    a->container      = t;

    CRCL(io_loop) = uv_default_loop();
    RET_IF_ERROR(
        uv_async_init( CRCL(io_loop), &CRCL(io_cmd), CRCL(io_cmd_cb) ) );
    return 0;
}

/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "__charcoal_replace_main".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */

/*
 * From here down is stuff related to the main procedure.
 *
 * The initial thread that exists when a Charcoal program starts will
 * be the I/O thread.  Before it starts its I/O duties it launches
 * another thread that will call the application's main procedure.
 */
int CRCL(application_main)( int argc, char **argv, char **env );

typedef struct
{
    int argc;
    char **argv, **env;
} CRCL(main_params);

static int CRCL(process_return_value);

static void CRCL(main_activity_entry)( void *p )
{
    CRCL(main_params) main_params = *((CRCL(main_params) *)p);
    int    argc = main_params.argc;
    char **argv = main_params.argv;
    char **env  = main_params.env;
    CRCL(process_return_value) = CRCL(application_main)( argc, argv, env );
    /* XXX Maybe free the initial thread and activity??? */
}

/* Architecture note: For the time being (as of early 2014, at least),
 * we're using libuv to handle asynchronous I/O stuff.  It would be
 * lovely if we could "embed" libuv's event loop in the yield logic.
 * Unfortunately, libuv embedding is not totally solidified yet.  So
 * we're going to have a separate thread for running the I/O event
 * loop.  Eventually we should be able to get rid of that. */
int main( int argc, char **argv, char **env )
{
    /* Okay to stack-allocate these here because the I/O thread should
     * always be the last thing running in a Charcoal process. */
    CRCL(thread_t)   io_thread;
    CRCL(activity_t) io_activity;
    RET_IF_ERROR( CRCL(init_io_loop)( &io_thread, &io_activity ) );

    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    CRCL(thread_t) *thd = (CRCL(thread_t)*)malloc( sizeof( thd[0] ) );
    if( !thd )
    {
        return ENOMEM;
    }
    CRCL(activity_t) *act = (CRCL(activity_t)*)malloc( sizeof( act[0] ) );
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

    RET_IF_ERROR( CRCL(init_yield_heartbeat)() );
    CRCL(activity_start_resume)( &io_activity );
    CRCL(create_thread)( thd, act, CRCL(main_activity_entry), &params );

    int rc = uv_run( CRCL(io_loop), UV_RUN_DEFAULT );
    printf( "Charcoal program finished!!! return code:%d (err?%d)\n",
            CRCL(process_return_value), rc );

    if( rc )
    {
        printf( "%i", rc );
        exit( rc );
    }
    return CRCL(process_return_value);
}
