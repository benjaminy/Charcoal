#ifndef __CHARCOAL_SEMAPHORE
#define __CHARCOAL_SEMAPHORE

#include <charcoal_runtime_common.h>

/*
 * A simple little semaphore library.  Anonymous POSIX semaphores are
 * not supported in OS-X, which is why this library exists.  In the
 * future it should be changed to trivial wrappers for the system's
 * anonymous semaphores, if they exist.
 *
 * This is different from the standard library semaphore because that
 * deals with activities in a smart way.  This is just a plain
 * multithreading seamphore.
 */

/* This struct is defined concretely so that client code can get the
 * proper size for allocation.  Should be treated as abstract. */
typedef struct
{
    int          value;
    uv_mutex_t   m;
    uv_cond_t    c;
    unsigned int waiters;
} crcl(sem_t);

extern int foo;

/* pshared == Process-shared.  Not supported for now. */
int crcl(sem_init)     ( crcl(sem_t) *, int pshared, unsigned int init );
int crcl(sem_destroy)  ( crcl(sem_t) * );
int crcl(sem_get_value)( crcl(sem_t) *__restrict, int *__restrict );
int crcl(sem_incr)     ( crcl(sem_t) * );
int crcl(sem_decr)     ( crcl(sem_t) * );
int crcl(sem_try_decr) ( crcl(sem_t) * );

#endif /* __CHARCOAL_SEMAPHORE */
