/*
 * A simple little semaphore library.  Anonymous POSIX semaphores are
 * not supported in OS-X, which is why this file exists.  In the future
 * it should be changed to trivial wrappers for the system's anonymous
 * semaphores, if they exist.
 */

/* XXX Unresolved: use errno or return value for error code? */

#include<stdlib.h>
#include<errno.h>
#include<charcoal_runtime.h>

/* pshared == Process-shared.  Not supported for now. */
int __charcoal_sem_init( __charcoal_sem_t *s, int pshared, unsigned int value )
{
    if( pshared || !s )
    {
        return EINVAL;
    }
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
    if( !s )
    {
        return EINVAL;
    }
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
        /* Destroying a semaphore while someone is still waiting on
         * it. */
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
    if( ( rc = pthread_mutex_lock( &s->m ) ) )
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
    if( !s )
    {
        return EINVAL;
    }
    int rv;
    pthread_mutex_lock( &s->m );
    if( s->value > 0 )
    {
        --s->value;
        rv = 0;
    }
    else
    {
        rv = EAGAIN;
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
    int rc;
    if( ( rc = pthread_mutex_lock( &s->m ) ) )
    {
        return rc;
    }
    while( s->value < 1 )
    {
        ++s->waiters; /* XXX OVERFLOW */
        if( ( rc = pthread_cond_wait( &s->c, &s->m ) ) )
        {
            return rc;
        }
        --s->waiters;
    }
    --s->value;
    if( ( pthread_mutex_unlock( &s->m ) ) )
    {
        return rc;
    }
    return 0;
}
