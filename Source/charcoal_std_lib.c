/*
 * TetraStak
 */

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

pthread_key_t __charcoal_current_activity;

activity_t activity_self( void )
{
    (activity_t)pthread_getspecific( __charcoal_current_activity );
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
    OPA_int_t *iptr, OPA_ptr_t *ptr, void *thread )
{
    int ifoo = OPA_fetch_and_incr_int( iptr );
    void *foo = OPA_cas_ptr( ptr, thread, self->thread );
    if( foo != self->thread )
    {
    }
}

void send( channel c, void *data )
{
    int foo = OPA_fetch_and_incr_int( &c->working_int );
    void *foo = OPA_cas_ptr( &c->cas_ptr, c->thread, self->thread );
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

void sem_inc( semaphore *s )
{
    if( pseudo_multithreaded_check(
            OPA_int_t *iptr, OPA_ptr_t *ptr, void *thread ) )
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
            OPA_int_t *iptr, OPA_ptr_t *ptr, void *thread ) )
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

