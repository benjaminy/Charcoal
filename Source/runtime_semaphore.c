/* XXX Unresolved: use errno or return value for error code? */

#include<core.h>
#include<charcoal_semaphore.h>
#include<stdlib.h>
#include<errno.h>

int crcl(sem_init)( crcl(sem_t) *s, int pshared, unsigned int value )
{
    if( pshared || !s )
    {
        return EINVAL;
    }
    int rc;
    if( ( rc = uv_mutex_init( &s->m ) ) )
    {
        return rc;
    }
    if( ( rc = uv_cond_init( &s->c ) ) )
    {
        return rc;
    }
    s->value = value;
    s->waiters = 0;
    return 0;
}

int crcl(sem_destroy)( crcl(sem_t) *s )
{
    if( !s )
    {
        return EINVAL;
    }
    uv_mutex_destroy( &s->m );
    uv_cond_destroy( &s->c );
    if( s->waiters )
    {
        /* Destroying a semaphore while someone is still waiting on
         * it. */
        /* XXX Improve error handling */
        exit( 1 );
    }
    return 0;
}

int crcl(sem_get_value)( crcl(sem_t) * __restrict s, int * __restrict vp )
{
    if( !s || !vp )
    {
        return EINVAL;
    }
    uv_mutex_lock( &s->m );
    *vp = s->value;
    uv_mutex_unlock( &s->m );
    return 0;
}

int crcl(sem_incr)( crcl(sem_t) *s )
{
    if( !s )
    {
        return EINVAL;
    }
    unsigned waiters = 0;
    uv_mutex_lock( &s->m );
    unsigned old_val = s->value;
    ++s->value;
    if( old_val >= s->value )
    {
        uv_mutex_unlock( &s->m );
        return EOVERFLOW;
    }
    waiters = s->waiters;
    uv_mutex_unlock( &s->m );
    if( waiters > 0 )
    {
        uv_cond_signal( &s->c );
    }
    return 0;
}

int crcl(sem_try_decr)( crcl(sem_t) *s )
{
    if( !s )
    {
        return EINVAL;
    }
    int rv;
    uv_mutex_lock( &s->m );
    if( s->value > 0 )
    {
        --s->value;
        rv = 0;
    }
    else
    {
        rv = EAGAIN;
    }
    uv_mutex_unlock( &s->m );
    return rv;
}

int crcl(sem_decr)( crcl(sem_t) *s )
{
    if( !s )
    {
        return EINVAL;
    }
    uv_mutex_lock( &s->m );
    while( s->value < 1 )
    {
        ++s->waiters; /* XXX OVERFLOW */
        uv_cond_wait( &s->c, &s->m );
        --s->waiters;
    }
    --s->value;
    uv_mutex_unlock( &s->m );
    return 0;
}
