#ifndef __CHARCOAL_SEMAPHORE
#define __CHARCOAL_SEMAPHORE

/*
 * A simple little semaphore library.  Anonymous POSIX semaphores are
 * not supported in OS-X, which is why this library exists.  In the
 * future it should be changed to trivial wrappers for the system's
 * anonymous semaphores, if they exist.
 */

/* This struct is defined concretely so that client code can get the
 * proper size for allocation.  Should be treated as abstract. */
typedef struct
{
    int             value;
    pthread_mutex_t m;
    pthread_cond_t  c;
    unsigned int    waiters;
} CRCL(sem_t);

/* pshared == Process-shared.  Not supported for now. */
int CRCL(sem_init)     ( CRCL(sem_t) *, int pshared, unsigned int init );
int CRCL(sem_destroy)  ( CRCL(sem_t) * );
int CRCL(sem_get_value)( CRCL(sem_t) *__restrict, int *__restrict );
int CRCL(sem_incr)     ( CRCL(sem_t) * );
int CRCL(sem_decr)     ( CRCL(sem_t) * );
int CRCL(sem_try_decr) ( CRCL(sem_t) * );

#endif /* __CHARCOAL_SEMAPHORE */
