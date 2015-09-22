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

activity_p crcl(get_self_activity)( void )
{
    return (activity_p)pthread_getspecific( crcl(self_key) );
}

void crcl(stack_monster)( crcl(frame_p) the_frame )
{
    while( the_frame )
    {
        the_frame = the_frame->fn( the_frame );
    }
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

int crcl(choose_next_activity)( activity_p *p )
{
    activity_p to_run, first, self = crcl(get_self_activity)();
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

void crcl(unyielding_enter)()
{
    activity_p self = crcl(get_self_activity)();
    crcl(atomic_incr_int)( &self->container->unyield_depth );
}

void crcl(unyielding_exit)()
{
    activity_p self = crcl(get_self_activity)();
    crcl(atomic_decr_int)( &self->container->unyield_depth );
    /* TODO: Maybe call yield here? */
}

void timeout_signal_handler(){
    crcl(atomic_store_int)(&(crcl(get_self_activity)()->container->timeout), 1);
    //printf("signal handler went off\n");
}

/* This should be called just before an activity starts or resumes from
 * yield/wait. */
static void crcl(activity_start_resume)( activity_p self )
{
    /* XXX: enqueue command */
    /* XXX: start heartbeat if runnable > 1 */
    ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    assert( !pthread_setspecific( crcl(self_key), self ) );
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) self->container->max_time);
    // crcl(atomic_store_int)(&self->container->timeout, 0);
    self->yield_attempts = 0;
    /* XXX: Lots to fix here. */
}

typedef void (*crcl(switch_listener))(activity_p from, activity_p to, void *ctx);

void crcl(push_yield_switch_listener)( crcl(switch_listener) pre, void *pre_ctx, crcl(switch_listener) post, void *post_ctx )
{
}

void crcl(pop_yield_switch_listener)()
{
}

/* XXX must fix.  Still assuming activities implemented as threads */
static int crcl(yield_try_switch)( activity_p self )
{
    activity_p to;
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

static void crcl(print_special_queue)( activity_p *q )
{
    assert( q );
    activity_p a = *q, first = a;
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
activity_p crcl(pop_special_queue)(
    unsigned queue_flag, crcl(thread_t) *t, activity_p *qp )
{
    // assert( t || qp );
    // assert( !( t && qp ) );
    activity_p *q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_REAP_QUEUE):
        q = &t->to_be_reaped;
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
        activity_p a = *q;
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

/* Precondition: The thread mgmt mutex is held. */
void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a,
    crcl(thread_t) *t, activity_p *qp )
{
    assert( t || qp );
    assert( !( t && qp ) );
    if( a->flags & queue_flag )
    {
        /* printf( "Already in queue\n" ); */
        return;
    }
    a->flags |= queue_flag;
    activity_p *q = NULL;
    switch( queue_flag )
    {
    case CRCL(ACTF_READY_QUEUE):
        q = &t->ready;
        break;
    case CRCL(ACTF_REAP_QUEUE):
        q = &t->to_be_reaped;
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
        activity_p front = *q, rear = front->sprev;
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
 * variable signal from another thread (typically the I/O thread).
 *
 * Should the calling code call _setjmp, or should that happen in
 * here? */
int crcl(activity_blocked)( activity_p self )
{
    // XXX --thd->runnable_activities;
    /* printf( "Actvity blocked\n" ); */
    crcl(thread_t) *thd = self->container;
    RET_IF_ERROR( pthread_mutex_lock( &thd->thd_management_mtx ) );
    while( !thd->ready )
    {
        RET_IF_ERROR( pthread_cond_wait( &thd->thd_management_cond,
                                         &thd->thd_management_mtx ) );
    }
    activity_p to = crcl(pop_special_queue)(
        CRCL(ACTF_READY_QUEUE), thd, NULL );
    RET_IF_ERROR( pthread_mutex_unlock( &thd->thd_management_mtx ) );
#if __CHARCOAL_ACTIVITY_IMPL == __CHARCOAL_IMPL_SETCONTEXT
    if( !setjmp( self->jmp ) )
    {
        crcl(activity_start_resume)( to );
        _longjmp( to->jmp, 1 );
    }
#elif __CHARCOAL_ACTIVITY_IMPL == __CHARCOAL_IMPL_COROUTINE
    sdf
#else
        sdf
#endif
    return 0;
}

void crcl(activity_set_return_value)( activity_p a, void *ret_val_ptr )
{
    memcpy( a->return_value, ret_val_ptr, a->ret_size );
}

void crcl(activity_get_return_value)( activity_p a, void **ret_val_ptr )
{
    memcpy( ret_val_ptr, a->return_value, a->ret_size );
}

void crcl(switch_from_to)( activity_p from, activity_p to )
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

void crcl(switch_to)( activity_p act )
{
    crcl(switch_from_to)( crcl(get_self_activity)(), act );
}

void yield(){
#if 0
    // XXX new idea: Counters
    //
    // The other idea requires performing a thread local read, _then_ an
    // atomic read on the pointer that comes back from that.  This idea
    // has a thread-local read and an atomic read as well, but they're
    // in parallel.  Might make a big difference.  Or it might not.
    // Maybe do an experiment some day.

    activity_p self = crcl(get_self_activity)();
    size_t current_yield_tick = crcl(atomic_load_size_t)( crcl(yield_ticker) );
    ssize_t diff = self->interrupt_tick - current_yield_tick;
    if( diff < 0 )
    {
        ...
    }
    
#endif
    activity_p self = crcl(get_self_activity)();

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
        activity_p to = crcl(pop_special_queue)(
            CRCL(ACTF_READY_QUEUE), thd, NULL );
        pthread_mutex_unlock( &thd->thd_management_mtx );
        crcl(switch_from_to)( self, to );
    }
}

static void crcl(remove_activity_from_thread)(
    activity_p a, crcl(thread_t) *t)
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
    activity_p a, crcl(thread_t) *t )
{
    /* printf( "Insert activity %p  %p\n", a, t ); */
    if( t->activities )
    {
        activity_p rear = t->activities->prev;
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
int crcl(activity_join)( activity_p a, void *p )
{
    if( !a )
    {
        return EINVAL;
    }
    if( a->flags & CRCL(ACTF_DONE) )
    {
        return 0;
    }
    activity_p self = crcl(get_self_activity)();
    /* printf( "joining.  push %p on %p\n", self, a ); */
    crcl(push_special_queue)( CRCL(ACTF_BLOCKED), self, NULL, &a->joining );
    RET_IF_ERROR( crcl(activity_blocked)( self ) );
    return 0;
}

int crcl(activity_detach)( activity_p a )
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

typedef struct crcl(thread_launch_ctx) crcl(thread_launch_ctx);

struct crcl(thread_launch_ctx)
{
    void            (*entry)( void * );
    void             *p;
    activity_p act;
    ucontext_t       *prv;
};

/* Never returns */
static void crcl(activity_finished)( activity_p a )
{
    int rc;
    crcl(thread_t) *thd = a->container;
    /* printf( "Finishing %p\n", a ); */
    if( ( rc = pthread_mutex_lock( &thd->thd_management_mtx ) ) )
    {
        exit( rc );
    }
    activity_p reaper_act = a->container->activities;
    crcl(push_special_queue)( CRCL(ACTF_READY_QUEUE), reaper_act, thd, NULL );
    crcl(push_special_queue)( CRCL(ACTF_REAP_QUEUE), a, thd, NULL );
    activity_p next = crcl(pop_special_queue)(
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

int crcl(activate_in_thread)(crcl(thread_t) *thd, activity_p act,
        crcl(entry_t) f, void *args )
{
    /* TODO: provide some way for the user to pass in a stack */
    act->flags = 0;
    act->container = thd;
    act->yield_attempts = 0;
    act->snext = act->sprev = act->joining = NULL;
    // XXX don't think we're using alarm anymore
    // XXX alarm((int) act->container->max_time);
    crcl(atomic_store_int)(&(act->container->timeout), 0);
    pthread_mutex_lock( &thd->thd_management_mtx );
    crcl(insert_activity_into_thread)( act, thd );

    ABORT_ON_FAIL( getcontext( &act->ctx ) );
    /* I hope the minimum stack size is actually bigger, but this is
     * fine for now. */
    act->ctx.uc_stack.ss_size = 16 * MINSIGSTKSZ;
    act->ctx.uc_stack.ss_sp = malloc( act->ctx.uc_stack.ss_size );
    if( !act->ctx.uc_stack.ss_sp )
    {
        return( ENOMEM );
    }
    act->ctx.uc_stack.ss_flags = 0; /* SA_DISABLE and/or SA_ONSTACK */
    act->ctx.uc_link = NULL; /* XXX fix. */
    /* XXX mask signals??? */
    sigemptyset( &act->ctx.uc_sigmask );

    ++thd->runnable_activities;

    /* XXX Maybe try to avoid malloc.  ucontext_t is huge. */
    ucontext_t *tmp = (ucontext_t *)malloc( sizeof( tmp[0] ) );
    crcl(thread_launch_ctx) ctx;
    ctx.prv   = tmp;
    ctx.entry = f;
    ctx.p     = args;
    ctx.act   = act;

    makecontext( &act->ctx, (void(*)(void))crcl(activity_entry), 1, &ctx );
    activity_p real_self = crcl(get_self_activity)();
    crcl(activity_start_resume)( act );
    swapcontext( tmp, &act->ctx );
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
int crcl(activate)( activity_p act, crcl(entry_t) f, void *args )
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
    /* printf( "Joining thread!!!\n" ); */
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

static void crcl(report_thread_done)( activity_p a )
{
    /* printf( "XXX Thread is done!!!\n" ); */
}

static void crcl(reap_activities)( activity_p a )
{
    crcl(thread_t) *thd = a->container;
    assert( thd->activities == a );
    ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    /* XXX More efficient to manage a pool of stacks, rather than
     * allocating and freeing every time?  Maybe not.  malloc is okay */
    while( 1 )
    {
        /* printf( "Reaping (reaper:%p) %p\n", a, thd->to_be_reaped ); */
        activity_p r;
        while( ( r = crcl(pop_special_queue)( CRCL(ACTF_REAP_QUEUE), thd, NULL ) ) )
        {
            /* XXX */
            /* printf( "Reaping activity %p j:%p\n", r, r->joining ); */
            r->flags |= CRCL(ACTF_DONE);
            crcl(remove_activity_from_thread)( r, thd );
            free( r->ctx.uc_stack.ss_sp );
            while( r->joining )
            {
                activity_p ready = crcl(pop_special_queue)(
                    CRCL(ACTF_BLOCKED), NULL, &r->joining );
                crcl(push_special_queue)(
                    CRCL(ACTF_READY_QUEUE), ready, ready->container, NULL );
            }
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
        crcl(activity_blocked)( a );
        /* printf( "The reaper is back!!! %p\n", a ); */
        ABORT_ON_FAIL( pthread_mutex_lock( &thd->thd_management_mtx ) );
    }
    ABORT_ON_FAIL( pthread_mutex_unlock( &thd->thd_management_mtx ) );
    crcl(report_thread_done)( a );
}

static void *crcl(thread_entry)( void *p )
{
    crcl(thread_launch_ctx) ctx = *((crcl(thread_launch_ctx)*)p);
    printf( "\n\nXXXXXXXXXX\n\n" ); fflush( stdout );
    free( p );
    printf( "\n\nYYYYYYYYYY\n\n" ); fflush( stdout );
    activity_p client_act = ctx.act;
    crcl(thread_t) *thd = client_act->container;
    /* printf( "CLIENT act %p\n", client_act ); */
    activity_t reaper_act;
    reaper_act.yield_attempts = 0;
    assert( thd );
    reaper_act.container = thd;
    /* printf( "reaper: %p  container: %p\n", &reaper_act, thd ); fflush( stdout ); */
    /* XXX  init can-run */
    reaper_act.flags = 0;
    reaper_act.next = &reaper_act;
    reaper_act.prev = &reaper_act;
    reaper_act.joining = NULL;
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

    crcl(activity_start_resume)( &reaper_act );
    crcl(activate_in_thread)( thd, client_act, ctx.entry, ctx.p );
    crcl(reap_activities)( &reaper_act );
    assert( thd->activities->next == &reaper_act );
    crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    cmd->command = CRCL(IO_CMD_JOIN_THREAD);
    cmd->_.thread = thd;
    enqueue( cmd );
    ABORT_ON_FAIL( uv_async_send( &crcl(io_cmd) ) );
    /* printf( "After!!!\n" ); */
    /* XXX a ha! we're getting here too soon! */
    return NULL;
}

/* NOTE: Launching a new thread needs storage for thread stuff and
 * activity stuff. */
static int crcl(create_thread)(
    crcl(thread_t) *thd, activity_p act, crcl(entry_t) entry_fn, void *actuals )
{
    int rc;
    assert( thd );
    assert( act );
    crcl(add_to_threads)( thd );

    crcl(atomic_store_int)( &thd->unyield_depth, 0 );
    crcl(atomic_store_int)( &thd->timeout, 0 );
    thd->timer_req.data = thd;
    /* XXX Does timer_init have to be called from the I/O thread? */
    uv_timer_init( crcl(io_loop), &thd->timer_req );
    // XXX thd->start_time = 0.0;
    // XXX thd->max_time = 0.0;

    RET_IF_ERROR( pthread_mutex_init( &thd->thd_management_mtx, NULL /* XXX attrs? */ ) );
    RET_IF_ERROR( pthread_cond_init( &thd->thd_management_cond, NULL /* XXX attrs? */ ) );

    thd->flags               = 0; // CRCL(THDF_ANY_RUNNING)
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
    ABORT_ON_FAIL( pthread_attr_setstacksize( &attr, PTHREAD_STACK_MIN ) );

    /* XXX: pthread attributes */
    // detachstate guardsize inheritsched schedparam schedpolicy scope

    /* Maybe make a fixed pool of these ctxs to avoid malloc */
    crcl(thread_launch_ctx) *ctx = (crcl(thread_launch_ctx) *)malloc( sizeof( ctx[0] ) );
    if( !ctx )
    {
        return ENOMEM;
    }
    ctx->entry = entry_fn;
    ctx->p     = actuals;
    ctx->act   = act;

    if( ( rc = pthread_create(
              &thd->sys, &attr, crcl(thread_entry), ctx ) ) )
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

static int crcl(init_io_loop)( crcl(thread_t) *t, activity_p a )
{
    crcl(threads) = t;

    RET_IF_ERROR(
        pthread_key_create( &crcl(self_key), NULL /* XXX destructor */ ) );

    /* Most of the thread and activity fields are not relevant to the I/O thread */
    t->activities     = a;
    t->ready          = NULL;
    t->to_be_reaped   = NULL;
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
int crcl(application_main)( int argc, char **argv, char **env );

typedef struct
{
    int argc;
    char **argv, **env;
} crcl(main_params);

static int crcl(process_return_value);

static void crcl(main_activity_entry)( void *p )
{
    crcl(main_params) main_params = *((crcl(main_params) *)p);
    int    argc = main_params.argc;
    char **argv = main_params.argv;
    char **env  = main_params.env;
    crcl(process_return_value) = crcl(application_main)( argc, argv, env );
    /* XXX Maybe free the initial thread and activity??? */
}

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
    printf( "PTHREAD_STACK_MIN: %d\n", PTHREAD_STACK_MIN );
    printf( "MINSIGSTKSZ: %d\n", MINSIGSTKSZ );
    /* Okay to stack-allocate these here because the I/O thread should
     * always be the last thing running in a Charcoal process. */
    crcl(thread_t)   io_thread;
    activity_t io_activity;
    RET_IF_ERROR( crcl(init_io_loop)( &io_thread, &io_activity ) );

    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    crcl(thread_t) *thd = (crcl(thread_t)*)malloc( sizeof( thd[0] ) );
    if( !thd )
    {
        return ENOMEM;
    }
    activity_p act = (activity_p)malloc( sizeof( act[0] ) );
    if( !act )
    {
        return ENOMEM;
    }

    /* It's slightly wasteful to stack-allocate these, because they only
     * need to exist until the application's main is actually called.
     * But really, it's only a handful of bytes. */
    crcl(main_params) params;
    params.argc = argc;
    params.argv = argv;
    params.env  = env;

    RET_IF_ERROR( crcl(init_yield_heartbeat)() );
    crcl(activity_start_resume)( &io_activity );
    crcl(create_thread)( thd, act, crcl(main_activity_entry), &params );

    int rc = uv_run( crcl(io_loop), UV_RUN_DEFAULT );
    printf( "Charcoal program finished!!! return code:%d (err?%d)\n",
            crcl(process_return_value), rc );

    if( rc )
    {
        printf( "%i", rc );
        exit( rc );
    }
    return crcl(process_return_value);
}
