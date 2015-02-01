/*
 * The Charcoal Runtime System
 */

#include <charcoal_base.h>
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h> /* XXX remove dep eventually */
#include <stdio.h> /* XXX remove dep eventually */
#include <errno.h>
#include <limits.h>
#include <charcoal_runtime.h>
#include <charcoal_runtime_io_commands.h>
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

/* Scheduler stuff */

#define ABORT_ON_FAIL( e ) \
    do { \
        int __abort_on_fail_rc = e; \
        if( __abort_on_fail_rc ) \
            exit( __abort_on_fail_rc ); \
    } while( 0 )

static crcl(thread_t) *crcl(threads);
static pthread_key_t crcl(self_key);
// XXX static timer_t crcl(heartbeat_timer);
static int crcl(heartbeat_timer);

activity_t *crcl(get_self_activity)( void )
{
    return (activity_t *)pthread_getspecific( crcl(self_key) );
}

static int crcl(start_stop_heartbeat)( int start )
{
    /* XXX I think 'its' can be stack alloc'ed.  Check this. */
    // struct itimerspec its;
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
    // its.it_value.tv_sec     = freq_nanosecs / 1000000000;
    // its.it_value.tv_nsec    = freq_nanosecs % 1000000000;
    // its.it_interval.tv_sec  = its.it_value.tv_sec;
    // its.it_interval.tv_nsec = its.it_value.tv_nsec;

    // RET_IF_ERROR( timer_settime( crcl(heartbeat_timer), 0, &its, NULL ) );
    return 0;
}

int crcl(choose_next_activity)( activity_t **p )
{
    activity_t *to_run, *first, *self = crcl(get_self_activity)();
    crcl(thread_t) *thd = self->container;
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    first = to_run = thd->activities;
    do {
        if( !( to_run->flags & CRCL(ACTF_BLOCKED) )
            && ( to_run != self ) )
        {
            first = NULL;
            break;
        }
        to_run = to_run->next;
    } while( to_run != first );
    /* XXX What is this sv nonsense about??? */
    int rv = -1, sv = crcl(sem_try_decr)( &self->can_run );
    if( !sv )
        crcl(sem_incr)( &self->can_run );

    if( first )
    {
        rv = crcl(sem_try_decr)( &to_run->can_run );
        if( !rv )
            crcl(sem_incr)( &to_run->can_run );
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

/* unyielding */ void crcl(unyielding_enter)( crcl(frame_p) frame )
{
    crcl(atomic_incr_int)( &frame->activity->thread->unyield_depth );
}

/* unyielding */ void crcl(unyielding_exit)( crcl(frame_p) frame )
{
    crcl(atomic_decr_int)( &frame->activity->thread->unyield_depth );
    /* TODO: Maybe call yield here?  Probably not.  unyielding -/> loop */
}

/* This should be called just before an activity starts or resumes from
 * yield/wait. */
static void crcl(activity_start_resume)( activity_p activity )
{
    /* XXX: enqueue command */
    /* XXX: start heartbeat if runnable > 1 */
    ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    assert( !pthread_setspecific( crcl(self_key), activity ) );
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) self->container->max_time);
    // crcl(atomic_store_int)(&self->container->timeout, 0);
    activity->yield_attempts = 0;
    /* XXX: Lots to fix here. */
}

typedef void (*crcl(switch_listener))(activity_t *from, activity_t *to, void *ctx);

void crcl(push_yield_switch_listener)( crcl(switch_listener) pre, void *pre_ctx, crcl(switch_listener) post, void *post_ctx )
{
}

void crcl(pop_yield_switch_listener)()
{
}

/* XXX must fix.  Still assuming activities implemented as threads */
static int crcl(yield_try_switch)( activity_t *self )
{
    activity_t *to;
    /* XXX error check */
    crcl(choose_next_activity)( &to );
    if( to )
    {
        crcl(sem_incr)( &to->can_run );
        crcl(sem_decr)( &self->can_run );
        crcl(activity_start_resume)( self );
    }
    return 0;
}

static void crcl(print_special_queue)( activity_t **q )
{
    assert( q );
    activity_t *a = *q, *first = a;
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
activity_t *crcl(pop_special_queue)(
    unsigned queue_flag, crcl(thread_t) *t, activity_t **qp )
{
    // assert( t || qp );
    // assert( !( t && qp ) );
    activity_t **q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_BLOCKED):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-pop pre " ); */
    /* crcl(print_special_queue)( q ); */
    if( *q )
    {
        activity_t *a = *q;
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
    /* crcl(print_special_queue)( q ); */
}

static activity_p crcl(pop_ready_queue)( thread_p t )
{
    return activity_t *to = crcl(pop_special_queue)(
        CRCL(ACTF_READY_QUEUE), thd, NULL );
}

/* Precondition: The thread mgmt mutex is held. */
void crcl(push_special_queue)(
    unsigned queue_flag, activity_t *a,
    crcl(thread_t) *t, activity_t **qp )
{
    assert( t || qp );
    assert( !( t && qp ) );
    if( a->flags & queue_flag )
    {
        /* printf( "Already in queue\n" ); */
        return;
    }
    a->flags |= queue_flag;
    activity_t **q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_BLOCKED):
        q = qp;
        break;
    default:
        exit( queue_flag );
    }
    /* printf( "S-push pre q:%p %p ", q, a ); */
    /* crcl(print_special_queue)( q ); */
    if( *q )
    {
        activity_t *front = *q, *rear = front->sprev;
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
    /* crcl(print_special_queue)( q ); */
}

/* activity_blocked should be called when an activity has nothing to do
 * right now.  Another activity or thread might switch back to it later.
 * If another activity is ready, run it.  Otherwise wait for a condition
 * variable signal from another thread (typically the I/O thread). */
crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame )
{
    // XXX --thd->runnable_activities;
    /* printf( "Actvity blocked\n" ); */
    frame_p    rv       = NULL;
    activity_p activity = frame->activity;
    thread_p   thd      = activity->thread;
    assert( !pthread_mutex_lock( &thd->thd_management_mtx ) );
    if( activity->flags & CRCL(ACTF_BLOCKED) )
    {
        activity->top = frame;
        if( thd->ready )
        {
            activity_p to = crcl(pop_ready_queue)( thd );
            crcl(activity_start_resume)( to );
            rv = to->top;
        }
    }
    else
    {
        rv = frame->caller;
    }
    assert( !pthread_mutex_unlock( &thd->thd_management_mtx ) );
    return rv;
}

void crcl(activity_set_return_value)( activity_t *a, void *ret_val_ptr )
{
    memcpy( a->return_value, ret_val_ptr, a->ret_size );
}

void crcl(activity_get_return_value)( activity_t *a, void **ret_val_ptr )
{
    memcpy( ret_val_ptr, a->return_value, a->ret_size );
}

void crcl(switch_from_to)( activity_t *from, activity_t *to )
{
    int rc;
    /* XXX assert from is the currently running activity? */
    crcl(thread_t) *thd = from->container;
    assert( thd );
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        printf( "ERRR from: %p thd: %p %s\n", from, thd, strerror( rc ) ); fflush( stdout );
        exit( rc );
    }
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), from, thd, NULL );
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    if( _setjmp( from->jmp ) == 0 )
    {
        crcl(activity_start_resume)( to );
        _longjmp( to->jmp, 1 );
    }
    //printf("Set timeout value to 0 in charcoal_switch_from_to\n");
    /* check if anybody should be deallocated (int sem_destroy(sem_t *);) */
}

void crcl(switch_to)( activity_t *act )
{
    crcl(switch_from_to)( crcl(get_self_activity)(), act );
}

crcl(frame_p)void crcl(yield_impl)( crcl(frame_p) self_frame ){
    activity_p self_activity      = self_frame->self;
    size_t     current_yield_tick = crcl(atomic_load_size_t)( crcl(yield_ticker) );
    ssize_t    diff               = self_activity->tick - current_yield_tick;
    if( diff < 0 )
    {
        ...
    }
#if 0
    // XXX new idea: Counters
    //
    // The other idea requires performing a thread local read, _then_ an
    // atomic read on the pointer that comes back from that.  This idea
    // has a thread-local read and an atomic read as well, but they're
    // in parallel.  Might make a big difference.  Or it might not.
    // Maybe do an experiment some day.

    
#endif
    activity_t *self = crcl(get_self_activity)();

    /* XXX DEBUG foo->yield_attempts++; */
    int unyield_depth = crcl(atomic_load_int)( &(self->container->unyield_depth) );
    if( unyield_depth == 0 )
    {
        crcl(thread_t) *thd = self->container;
        /* XXX handle locking errors? */
        pthread_mutex_lock( &thd->thd_management_mtx );
        if( !thd->ready )
        {
            pthread_mutex_unlock( &thd->thd_management_mtx );
            return;
        }
        activity_t *to = crcl(pop_special_queue)(
            CRCL(ACTF_READY_QUEUE), thd, NULL );
        pthread_mutex_unlock( &thd->thd_management_mtx );
        crcl(switch_from_to)( self, to );
    }
}

static void crcl(remove_activity_from_thread)(
    activity_t *a, crcl(thread_t) *t)
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

static void crcl(insert_activity_into_thread)(
    activity_t *a, crcl(thread_t) *t )
{
    /* printf( "Insert activity %p  %p\n", a, t ); */
    if( t->activities )
    {
        activity_t *rear = t->activities->prev;
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
int crcl(activity_join)( activity_t *a, void *p )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & CRCL(ACTF_DONE) )
    {
        return 0;
    }
    activity_t *self = crcl(get_self_activity)();
    crcl(push_blocked_queue)( CRCL(ACTF_BLOCKED), self, NULL, &a->joining );
    RET_IF_ERROR( crcl(activity_blocked)( self ) );
    return 0;
}

int crcl(activity_detach)( activity_t *a )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & CRCL(ACTF_DETACHED) )
    {
        /* XXX error? */
    }
    a->flags |= CRCL(ACTF_DETACHED);
    return 0;
}

/* Never returns */
static void crcl(activity_finished)( activity_t *a )
{
    int rc;
    crcl(thread_t) *thd = a->container;
    /* printf( "Finishing %p\n", a ); */
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    activity_t *reaper_act = a->container->activities;
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), reaper_act, thd, NULL );
    crcl(push_special_queue)( CRCL(ACTF_REAP_QUEUE), a, thd, NULL );
    activity_t *next = crcl(pop_special_queue)(
        CRCL(ACTF_READY_QUEUE), thd, NULL );
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    /* printf( "Jumping to %p\n", next ); */
    crcl(activity_start_resume)( next );
    _longjmp( next->jmp, 1 );
    exit( 1 );
}

/* XXX The compiler could generate this code to avoid too many layers
 * of wrappers. */
/* This is the entry procedure for new activities */
static void crcl(activity_entry)( void *p )
{
    crcl(thread_launch_ctx) ctx = *(crcl(thread_launch_ctx) *)p;
    /* printf( "Starting activity %p(thd:%p)\n", ctx.act, ctx.act->container ); */

    /* XXX activity migration between threads will cause problems */

    /* Wacky code alert!  We swapcontext back to the activity that
     * created this one, after doing a setjmp here.  I'm not 100% sure
     * why it's important to do this swapcontext when "normal"
     * activity switching can be done with longjmp, but it seems to
     * be. */
    /* XXX Maybe try to avoid malloc.  ucontext_t is huge. */
    ucontext_t *tmp = (ucontext_t *)malloc( sizeof( tmp[0] ) );;
    if( _setjmp( ctx.act->jmp ) == 0 )
    {
        swapcontext( tmp, ctx.prv );
    }
    free( tmp );
    /* LOG: Actual activity starting. */
    ctx.entry( ctx.p );
    /* LOG: Actual activity complete. */
    assert( ctx.act == crcl(get_self_activity)() );
    crcl(activity_finished)( ctx.act );
    exit( -1 );
}

int crcl(activate_in_thread)(
    thread_p thd,
    activity_p act,
    crcl(entry_t) f,
    void *args )
{
    act->flags = 0;
    act->container = thd;
    act->yield_attempts = 0;
    act->snext = act->sprev = NULL;
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) act->container->max_time);
    crcl(atomic_store_int)(&(act->container->timeout), 0);
    pthread_mutex_lock( &thd->thd_management_mtx );
    crcl(insert_activity_into_thread)( act, thd );

    /* XXX mask signals??? */

    ++thd->runnable_activities;

    /* XXX Maybe try to avoid malloc.  ucontext_t is huge. */
    ucontext_t *tmp = (ucontext_t *)malloc( sizeof( tmp[0] ) );
    crcl(thread_launch_ctx) ctx;
    ctx.entry = f;
    ctx.p     = args;
    ctx.act   = act;

    activity_t *real_self = crcl(get_self_activity)();
    crcl(activity_start_resume)( act );
    crcl(activity_start_resume)( real_self );
    pthread_mutex_unlock( &thd->thd_management_mtx );
    /* Whether the newly created activities goes first should probably
     * be controllable. */
    free( tmp );
    crcl(switch_to)( act );
    return 0;
}

/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
int crcl(activate)( activity_t *act, crcl(entry_t) f, void *args )
{
    return crcl(activate_in_thread)(
        crcl(get_self_activity)()->container, act, f, args );
}

static void crcl(add_to_threads)( crcl(thread_t) *thd )
{
    crcl(thread_t) *last = crcl(threads)->prev;
    last->next = thd;
    crcl(threads)->prev = thd;
    thd->next = crcl(threads);
    thd->prev = last;
}

int crcl(join_thread)( crcl(thread_t) *t )
{
    ABORT_ON_FAIL( pthread_join( t->sys, NULL ) );
    if( t == t->next )
    {
        /* XXX Trying to remove the last thread. */
        exit( 1 );
    }
    if( t == crcl(threads) )
    {
        /* XXX Trying to remove the I/O thread. */
        exit( 1 );
    }
    t->next->prev = t->prev;
    t->prev->next = t->next;
    return crcl(threads) == crcl(threads)->next;
}

static void crcl(report_thread_done)( activity_t *a )
{
    /* printf( "XXX Thread is done!!!\n" ); */
}

struct crcl(thread_entry_params)
{
    crcl(sem_t) s;
    thread_p   *t;
    void       *options;
}

static void *crcl(thread_entry)( void *p )
{
    assert( p );
    struct crcl(thread_entry_params) *params = (struct crcl(thread_entry_params) *)p;
    thread_t thd;
    *params->t = &thd;
    /* XXX  init can-run */

    crcl(add_to_threads)( &thd );
    crcl(atomic_store_int)( &thd.unyield_depth, 0 );
    crcl(atomic_store_int)( &thd.timeout, 0 );
    crcl(atomic_store_int)( &thd.keep_going, 1 );
    thd.timer_req.data = &thd;
    /* XXX Does timer_init have to be called from the I/O thread? */
    uv_timer_init( crcl(io_loop), &thd.timer_req );
    // XXX thd->start_time = 0.0;
    // XXX thd->max_time = 0.0;

    assert( !pthread_mutex_init( &thd.thd_management_mtx, NULL /* XXX attrs? */ ) );
    assert( !pthread_cond_init( &thd.thd_management_cond, NULL /* XXX attrs? */ ) );

    thd.flags               = 0; // CRCL(THDF_ANY_RUNNING)
    thd.runnable_activities = 0;
    thd.activities          = NULL;
    thd.ready               = NULL;

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    assert( !crcl(sem_incr)( &params->s ) );

    crcl(atomic_int) *keep_going = &thd->keep_going;
    while( crcl(atomic_load_int)( keep_going ) )
    {
        int rc = pthread_mutex_lock( &thd->thd_management_mtx );
        assert( !rc );
        while( crcl(atomic_load_int)( keep_going ) && !thd->ready )
        {
            rc = pthread_cond_wait( &thd->thd_management_cond,
                                    &thd->thd_management_mtx );
            assert( !rc );
        }
        if( !crcl(atomic_load_int)( keep_going ) )
        {
            /* TODO: Check reason for interruption */
            rc = pthread_mutex_unlock( &thd->thd_management_mtx );
            assert( !rc );
            break;
        }
        activity_p a = crcl(pop_ready_queue)( thd );
        crcl(frame_p) = a->top;
        rc = pthread_mutex_unlock( &thd->thd_management_mtx );
        assert( !rc );
        do
        {
            the_frame = the_frame->fn( the_frame );
        } while( the_frame && crcl(atomic_load_int)( keep_going ) );
    }

    crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    cmd->command = CRCL(IO_CMD_JOIN_THREAD);
    cmd->_.thread = thd;
    enqueue( cmd );
    assert( !uv_async_send( &crcl(io_cmd) ) );
    /* printf( "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
    return NULL;
}

/* NOTE: Launching a new thread needs storage for thread stuff and
 * activity stuff. */
int thread_start( crcl(thread_p) *thd, void *options )
{
    int rc;

    pthread_attr_t attr;
    ABORT_ON_FAIL( pthread_attr_init( &attr ) );
    /* Because yielding call frames are heap-allocated, the thread's
     * stack only needs to be big enough for the maximum chain of
     * unyielding calls, which shouldn't be very long.  However, TODO:
     * this should probably be configurable. */
    ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN ) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    /* Maybe make a fixed pool of these ctxs to avoid malloc */
    struct crcl(thread_entry_params) params;
    assert( !crcl(sem_init)( &params.s, 0, 0 ) );
    params.t       = thd;
    params.options = options;

    if( ( rc = pthread_create(
              &thd->sys, &attr, crcl(thread_entry), &params ) ) )
    {
        return rc;
    }
    crcl(sem_decr)( &params.s );
    assert( !crcl(sem_destroy)( &params.s ) );
    assert( !pthread_attr_destroy( &attr ) );

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
static void crcl(yield_heartbeat)( int sig, siginfo_t *info, void *uc )
{
    if( sig != 0 /*XXX SIG*/ )
    {
        /* XXX Very weird */
        exit( sig );
    }

    void *p = info->si_value.sival_ptr;
    if( p == &crcl(threads) )
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

static int crcl(init_yield_heartbeat)()
{
    /* Establish handler for timer signal */
    struct sigaction sa;
    /* printf("Establishing handler for signal %d\n", SIG); */
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = crcl(yield_heartbeat);
    sigemptyset( &sa.sa_mask );
    RET_IF_ERROR( sigaction( 0 /*SIG*/, &sa, NULL ) );

    /* Create the timer */
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = 0 /*XXX SIG*/;
    sev.sigev_value.sival_ptr = &crcl(threads);
    RET_IF_ERROR( timer_create( 0 /*CLOCKID*/, &sev, &crcl(heartbeat_timer) ) );

    /* printf( "timer ID is 0x%lx\n", (long) crcl(heartbeat_timer) ); */
    return 0;
}

static int crcl(init_io_loop)( crcl(thread_t) *t, activity_t *a )
{
    crcl(threads) = t;

    RET_IF_ERROR(
        pthread_key_create( &crcl(self_key), NULL /* XXX destructor */ ) );

    /* Most of the thread and activity fields are not relevant to the I/O thread */
    t->activities     = a;
    t->ready          = NULL;
    t->next           = crcl(threads);
    t->prev           = crcl(threads);
    /* XXX Not sure about the types of self: */
    t->sys            = pthread_self();
    a->container      = t;

    crcl(io_loop) = uv_default_loop();
    RET_IF_ERROR(
        uv_async_init( crcl(io_loop), &crcl(io_cmd), crcl(io_cmd_cb) ) );
    return 0;
}

/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "crcl(replace_main)".  The "real" main is
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
crcl(frame_p) crcl(fn_main_init)( int argc, char **argv, char **env );

typedef struct
{
    int argc;
    char **argv, **env;
} crcl(main_params);

/* Architecture note: For the time being (as of early 2014, at least),
 * we're using libuv to handle asynchronous I/O stuff.  It would be
 * lovely if we could "embed" libuv's event loop in the yield logic.
 * Unfortunately, libuv embedding is not totally solidified yet.  So
 * we're going to have a separate thread for running the I/O event
 * loop.  Eventually we should be able to get rid of that.
 * ... On the other hand, having a separate thread for the event loop
 * means that we can do the heartbeat timer there and not have to worry
 * about signal handling junk.  Seems like that might be a nicer way to
 * go for many systems.  In the long term, probably should support
 * both. */
int main( int argc, char **argv, char **env )
{
    /* Okay to stack-allocate these here because the I/O thread should
     * always be the last thing running in a Charcoal process. */
    crcl(thread_t)   io_thread;
    activity_t io_activity;
    RET_IF_ERROR( crcl(init_io_loop)( &io_thread, &io_activity ) );

    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    thread_p thd = (thread_p)malloc( sizeof( thd[0] ) );
    if( !thd )
    {
        return ENOMEM;
    }
    activity_p act = (activity_p)malloc( sizeof( act[0] ) );
    if( !act )
    {
        return ENOMEM;
    }
    crcl(frame_p) main_entry = crcl(fn_main_init)( argc, argv, env );
    if( !main_entry )
    {
        return ENOMEM;
    }

    RET_IF_ERROR( crcl(init_yield_heartbeat)() );
    crcl(activity_start_resume)( &io_activity );
    crcl(create_thread)( thd, act, crcl(main_activity_entry), &params );

    int rc = uv_run( crcl(io_loop), UV_RUN_DEFAULT );

    if( rc )
    {
        printf( "Error running the I/O loop: %i", rc );
        exit( rc );
    }

    int process_return_value = *((int *)&main_entry.locals);
    printf( "Charcoal program finished!!! return code:%d\n", process_return_value );
    return process_return_value;
}
