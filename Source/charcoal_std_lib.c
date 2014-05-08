#include <charcoal_base.h>
#include <charcoal_std_lib.h>
#include <errno.h>
#include <stdlib.h>

/*
 * TetraStak
 */
#if 0
struct _TET_MACHINE
{
};

struct _TET_PROCESS
{
    TET_MACHINE container;
};

struct _TET_THREAD
{
    TET_PROCESS container;
};

struct _TET_TASK
{
    TET_THREAD container;
};

pthread_key_t crcl(current_activity);

activity_t activity_self( void )
{
    (activity_t)pthread_getspecific( crcl(current_activity) );
}

typedef enum
{
    TET_CHANNEL_UNI,
    TET_CHANNEL_BI,
} TET_CHANNEL_KIND;

struct _TET_CH_PORT
{
    /* NULL=unattached */
    TET_TASK task;
    size_t buffer_capacity, currently_buffered;
}

struct _TET_CHANNEL
{
    TET_CHANNEL_KIND kind;
    /* For uni-directional channels: left=sender; right=receiver */
    _TET_CH_PORT left, right;
};

static int pseudo_multithreaded_check(
    __crcl_atomic_int *iptr, __crcl_atomic_int *ptr, void *thread )
{
    int ifoo = __crcl_atomic_fetch_and_incr_int( iptr );
    void *foo = __crcl_atomic_compare_exchange_ptr( ptr, thread, self->thread );
    if( foo != self->thread )
    {
    }
}

void send( channel c, void *data )
{
    int foo = __crcl_atomic_fetch_and_incr_int( &c->working_int );
    void *foo = __crcl_atomic_compare_exchange_ptr( &c->cas_ptr, c->thread, self->thread );
    {
        /* multithreaded channel!!! */
    }
    else
    {
    }
    if( c->buffered && !c->full )
    {
    }
    else if( c->partner_waiting )
    {
        switch_to();
    }
    else
    {
        c->partner_waiting = true;
        if( c->single_receiver )
        {
            switch_to( c->receiver );
        }
        block_self();
        invoke_scheduler();
    }
}

void receive( channel c )
{
}

/* activity version */
int acquire( mutex m )
{
    /* XXX add thread stuff */
    if( m->held > 0 )
    {
        if( m->owner == self )
        {
            ++m->held;
            return;
        }
        else
        {
            char buffer;
            buffered_channel ch, *next;
            channel_init( ch, &buffer );
            next = m->waiter;
            m->waiter = &ch;
            recv( &ch );
            m->waiter = next;
        }
    }
    m->held = 1;
    m->owner = self;
    return OK;
}

int release( mutex m )
{
    if( m->held < 1 || m->owner != self )
    {
        /* ERROR */
    }
    else
    {
        --m->held;
        if( m->held < 1 )
        {
            if( m->waiter )
            {
                send( m->waiter, () );
            }
        }
    }
}
#endif
/* XXX the semaphore implementation is not thread-safe yet.  Fix
 * eventually. */

int semaphore_open( semaphore_t *s, unsigned i )
{
    if( !s )
    {
        return EINVAL;
    }
    s->value = i;
    s->waiters = NULL;
    return 0;
}

int semaphore_close( semaphore_t *s )
{
    if( !s )
    {
        return EINVAL;
    }
    if( s->waiters )
    {
        /* XXX error? */
    }
    return 0;
}

int semaphore_incr( semaphore_t *s )
{
    if( !s )
    {
        return EINVAL;
    }
    ++s->value;
    if( s->waiters )
    {
        activity_t *a = crcl(pop_special_queue)(
            CRCL(ACTF_BLOCKED), NULL, &s->waiters );
        crcl(push_special_queue)(
            CRCL(ACTF_READY_QUEUE), a, a->container, NULL );
    }
    return 0;
}

int semaphore_decr( semaphore_t *s )
{
    if( !s )
    {
        return EINVAL;
    }
    activity_t *self = crcl(get_self_activity)();
    while( s->value < 1 )
    {
        int rc;
        crcl(push_special_queue)(CRCL(ACTF_BLOCKED),
            self, NULL, &s->waiters );
        if( ( rc = crcl(activity_blocked)( self ) ) )
        {
            return rc;
        }
    }
    --s->value;
    /* XXX maybe this isn't necessary?  */
    if( s->value > 0 && s->waiters )
    {
        activity_t *a = crcl(pop_special_queue)(
            CRCL(ACTF_BLOCKED), NULL, &s->waiters );
        crcl(push_special_queue)(
            CRCL(ACTF_READY_QUEUE), a, a->container, NULL );
    }
    return 0;
}

int semaphore_try_decr( semaphore_t *s )
{
    if( !s )
    {
        return EINVAL;
    }
    if( s->value < 1 )
    {
        return EAGAIN;
    }
    else
    {
        --s->value;
        return 0;
    }
}

static int crcl(send_io_cmd)( crcl(io_cmd_t) *cmd, activity_t *a )
{
    enqueue( cmd );
    a->flags |= CRCL(ACTF_BLOCKED);
    /* XXX Race between the next two lines??? */
    RET_IF_ERROR( uv_async_send( &crcl(io_cmd) ) );
    RET_IF_ERROR( crcl(activity_blocked)( a ) );
    return 0;
}

int getaddrinfo_crcl(
    const char *node,
    const char *service,
    const struct addrinfo *hints,
    struct addrinfo **res )
{
    /* XXX if unyielding, just call directly */
    uv_getaddrinfo_t resolver;
    activity_t *self = crcl(get_self_activity)();
    resolver.data = self;
    crcl(io_cmd_t) *cmd = (crcl(io_cmd_t) *)malloc( sizeof( cmd[0] ) );
    if( !cmd )
    {
        return ENOMEM;
    }
    cmd->command = CRCL(IO_CMD_GETADDRINFO);
    cmd->_.addrinfo.resolver = &resolver;
    cmd->_.addrinfo.node     = node;
    cmd->_.addrinfo.service  = service;
    cmd->_.addrinfo.hints    = hints;
    RET_IF_ERROR( crcl(send_io_cmd)( cmd, self ) );
    *res = self->io_response.addrinfo.info;
    return self->io_response.addrinfo.rc;
}

#if 0
void sem_inc( semaphore *s )
{
    if( pseudo_multithreaded_check(
            __crcl_atomic_int *iptr, __crcl_atomic_ptr *ptr, void *thread ) )
    {
        system_semaphore_inc();
    }
    else
    {
        ++s->count;
        if( s->waiters )
        {
            /* XXX */
        }
    }
}

void sem_dec()
{
    if( pseudo_multithreaded_check(
            __crcl_atomic_int *iptr, __crcl_atomic_ptr *ptr, void *thread ) )
    {
        system_semaphore_inc();
    }
    else
    {
        if( s->count > 0 )
        {
            --s->count;
        }
        else
        {
            if( s->front )
        }
    }
}

void yield_debug( current_time )
{
    yield_delta = current_time - last_yield_time_stamp;
    if( yield_delta < epsilon )
    {
        ++short_yield_deltas;
    }
    else if( yield_delta > time_resolution )
    {
        ++long_yield_deltas;
    }
    else
    {
        ++good_yield_deltas;
    }
    last_yield_time_stamp = current_time;
}

inline void yield( void )
{
    current_time = magic_machine_specific_thing;
    if( debug )
    {
        yield_debug( current_time )
    }
    time_slice_not_expired = current_time < end_of_time_slice;
    if( ( no_yield_depth > 0 )
        || ( no_external_interrupt_pending
             && time_slice_not_expired ) )
    {
        /* This path should be followed on the vast majority of yield
         * invocations in most programs. */
        return;
    }
    activity_scheduler();
}

void wait( event ev )
{
}

void try_wait( event ev )
{
}

typedef struct
{
    enum{ ONCE_NOT_STARTED, ONCE_IN_PROGRESS, ONCE_DONE } status;
    void *condition;
    void *res;
} once_t;

/* unnecessarily complicated */
void *once_activity( once_t *o, void *(*proc)( void * ), void *arg )
{
    if( o->status == ONCE_NOT_STARTED )
    {
        o->status = ONCE_IN_PROGRESS;
        o->res = proc( arg );
        o->status = ONCE_DONE;
        send( done_channel, NULL );
    }
    else if( o->status == ONCE_IN_PROGRESS )
    {
        send
        wait( o->condition );
    }
    return o->res;
}

void *once( once_t *o, void *(*proc)( void ) )
{
    acquire( o->m );
    if( o->inited )
        return o->val;
    o->val = proc();
    o->inited = true;
    release( o->m );
    return o->val;
}

/* If you _really_ don't want to acq/rel */
void *once( once_t *o, void *(*proc)( void ) )
{
    if( o->inited )
        return o->val;
    acquire( o->m );
    if( o->inited )
        return o->val;
    o->val = proc();
    yield;
    o->inited = true;
    release( o->m );
    return o->val;
}

#endif
