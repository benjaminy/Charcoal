#ifndef __CHARCOAL_STD_LIB_H
#define __CHARCOAL_STD_LIB_H

#include <charcoal_runtime.h>
#include <netdb.h>

/* Using the C11 API where appropriate */

#if 0
/* Thread and activity types */
typedef struct *thread_t;
typedef struct *activity_t;

typedef int (*thrd_start_t)(void*)

int     thrd_create( thrd_t *thr, thrd_start_t func, void *arg );
int     thrd_equal(     thrd_t lhs,     thrd_t rhs );
int activity_equal( activity_t lhs, activity_t rhs );
thrd_t         thrd_current();
activity_t activity_current();

int     thrd_sleep( const struct timespec* time_point, struct timespec* remaining );
int activity_sleep( const struct timespec* time_point, struct timespec* remaining );
void thrd_yield();
_Noreturn void thrd_exit( int res );
int thrd_detach( thrd_t thr );
int thrd_join( thrd_t thr, int *res );
#endif
typedef struct semaphore_t semaphore_t;

struct semaphore_t
{
    unsigned value;
    CRCL(activity_t) *waiters;
};

int semaphore_open    ( semaphore_t *s, unsigned i );
int semaphore_close   ( semaphore_t *s );
int semaphore_incr    ( semaphore_t *s );
int semaphore_decr    ( semaphore_t *s );
int semaphore_try_decr( semaphore_t *s );

int getaddrinfo_crcl(
    const char *node,
    const char *service,
    const struct addrinfo *hints,
    struct addrinfo **res );

#if 0
enum{
thrd_success
thrd_timedout
thrd_busy
thrd_nomem
thrd_error
}


typedef struct *mtx_t;

int mtx_init( mtx_t* mutex, int type );
int mtx_lock( mtx_t* mutex );
int mtx_timedlock( mtx_t *restrict mutex, const struct timespec *restrict time_point );
int mtx_trylock( mtx_t *mutex );
int mtx_unlock( mtx_t *mutex );
void mtx_destroy( mtx_t *mutex );

enum {
    mtx_plain = /* unspecified */,
    mtx_recursive = /* unspecified */,
    mtx_timed = /* unspecified */
};

#define ONCE_FLAG_INIT /* unspecified */
typedef once_flag;

void call_once( once_flag* flag, void (*func)(void) );

typedef cnd_t;

int cnd_init( cnd_t* cond );
int cnd_signal( cnd_t *cond );
int cnd_broadcast( cnd_t *cond );
int cnd_wait( cnd_t* cond, mtx_t* mutex );
int cnd_timedwait( cnd_t* restrict cond, mtx_t* restrict mutex,
                   const struct timespec* restrict time_point );
void cnd_destroy( cnd_t* cond );
  
#define thread_local _Thread_local

typedef tss_t;
typedef void (*tss_dtor_t)(void*);
int tss_create( tss_t* tss_id, tss_dtor_t destructor );

#define TSS_DTOR_ITERATIONS /* unspecified */

void *tss_get( tss_t tss_id );
int   tss_set( tss_t tss_id, void *val );
void  tss_delete( tss_t tss_id );


/* Okay, filling in some holes in C11 */

typedef float barrier_t;

int barrier_init( barrier_t *restrict barrier, unsigned count); 
int barrier_wait( barrier_t *restrict barrier );
int barrier_destroy( barrier_t *barrier );


TET_RESULT_CODE tet_spawn( void *(*f)(void *), void *param );
TET_RESULT_CODE tet_spawn_in_thread(
    void *(*f)(void *), void *param, TET_THREAD thread );
TET_RESULT_CODE tet_spawn_in_process(
    void *(*f)(void *), void *param, TET_PROCESS process );
TET_RESULT_CODE tet_spawn_in_machine(
    void *(*f)(void *), void *param, TET_MACHINE machine );

TET_RESULT_CODE tet_coroutine_init(
    void *(*f)(void *),
    void *param,
    TET_BCHANNEL channel );

TET_RESULT_CODE tet_coroutine_call(
    TET_BCHANNEL channel, ... );

TET_RESULT_CODE tet_coroutine_call_async(
    TET_BCHANNEL channel, TET_EVENT event, ... );

/* channels */

/* The simplest send receive primitives. */
TET_RESULT_CODE tet_send( TET_CH_PORT port, void *data, size_t sz );
TET_RESULT_CODE tet_recv( TET_CH_PORT port, void *data, size_t *sz );
TET_RESULT_CODE tet_send_fancy(
    TET_CH_PORT port, ... );
TET_RESULT_CODE tet_recv_fancy(
    TET_CH_PORT port, ... );



TET_RESULT_CODE tet_yield( void );
TET_RESULT_CODE tet_yield_value( void *val );
TET_RESULT_CODE tet_yield_value_to_chan( void *val, TET_CHANNEL channel );
TET_RESULT_CODE tet_set_coroutine_channel(
    TET_CHANNEL channel, TET_CHANNEL *prev_channel );

/* task, thread, process, machine? */
TET_RESULT_CODE tet_async_call( void *(*f)( void *), promise? );

/* synchronization primitives */
#endif
#endif  /* __CHARCOAL_STD_LIB_H */
