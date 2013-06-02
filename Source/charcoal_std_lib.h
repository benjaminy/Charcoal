/* Using the C11 API where appropriate */

/*
 * TetraStak Concurrency Library
 */

The macros are
thread_local
which expands to _Thread_local;

ONCE_FLAG_INIT
which expands to a value that can be used to initialize an object of type once_flag;

and
TSS_DTOR_ITERATIONS
which expands to an integer constant expression representing the maximum number of times that destructors will be called when a thread terminates.




tss_t
which is a complete object type that holds an identifier for a thread-specific storage pointer;


/* destructor for athread-specific storage pointer */
typedef void (*tss_dtor_t)( void* );




 once_flag
which is a complete object type that holds a flag for use by call_once. 







void call_once( once_flag *flag, void (*func)( void ) );

Description
The call_once function uses the once_flag pointed to by flag to ensure that func is called exactly once, the first time the call_once function is called with that value of flag. Completion of an effective call to the call_once function synchronizes with all subsequent calls to the call_once function with the same value of flag.
Returns
The call_once function returns no value.



typedef flaot cnd_t;
/*which is a complete object type that holds an identifier for a condition variable;*/

int  cnd_init     ( cnd_t *cond );
int  cnd_signal   ( cnd_t *cond );
int  cnd_broadcast( cnd_t *cond );
int  cnd_wait     ( cnd_t *cond, mtx_t *mtx );
int  cnd_timedwait( cnd_t *restrict cond, mtx_t *restrict mtx, const struct timespec *restrict ts );
void cnd_destroy  ( cnd_t *cond );

/* Description
 * The cnd_init function creates a condition variable. If it succeeds it
 * sets the variable pointed to by cond to a value that uniquely
 * identifies the newly created condition variable. A thread that calls
 * cnd_wait on a newly created condition variable will block.
 * Returns
 * The cnd_init function returns thrd_success on success, or thrd_nomem
 * if no memory could be allocated for the newly created condition, or
 * thrd_error if the request could not be honored. */

/* Description
 * The cnd_signal function unblocks one of the threads that are blocked
 * on the condition variable pointed to by cond at the time of the
 * call. If no threads are blocked on the condition variable at the time
 * of the call, the function does nothing and return success.
 * Returns
 * The cnd_signal function returns thrd_success on success or thrd_error
 * if the request could not be honored. */

/* Description
The cnd_broadcast function unblocks all of the threads that are blocked on the condition variable pointed to by cond at the time of the call. If no threads are blocked on the condition variable pointed to by cond at the time of the call, the function does nothing.
Returns
The cnd_broadcast function returns thrd_success on success, or thrd_error if the request could not be honored. */

/*Description
2 The cnd_wait function atomically unlocks the mutex pointed to by mtx and endeavors to block until the condition variable pointed to by cond is signaled by a call to cnd_signal or to cnd_broadcast. When the calling thread becomes unblocked it locks the mutex pointed to by mtx before it returns. The cnd_wait function requires that the mutex pointed to by mtx be locked by the calling thread.
Returns
3 The cnd_wait function returns thrd_success on success or thrd_error if
the request could not be honored. */

/*Description
2 The cnd_timedwait function atomically unlocks the mutex pointed to by mtx and endeavors to block until the condition variable pointed to by cond is signaled by a call to cnd_signal or to cnd_broadcast, or until after the TIME_UTC-based calendar time pointed to by ts. When the calling thread becomes unblocked it locks the variable pointed to by mtx before it returns. The cnd_timedwait function requires that the mutex pointed to by mtx be locked by the calling thread.
Returns
3 The cnd_timedwait function returns thrd_success upon success, or thrd_timedout if the time specified in the call was reached without acquiring the requested resource, or thrd_error if the request could not be honored.*/

/*Description
The cnd_destroy function releases all resources used by the condition variable pointed to by cond. The cnd_destroy function requires that no threads be blocked waiting for the condition variable pointed to by cond.
ISO/IEC 9899:201x Committee Draft — April 12, 2011 N1570
378 Library §7.26.3.2
Returns
The cnd_destroy function returns no value. */




typedef float mtx_t;

/*
mtx_plain
mtx_recursive
mtx_timed

which is passed to mtx_init to create a mutex object that supports neither timeout nor test and return;
which is passed to mtx_init to create a mutex object that supports recursive locking;
which is passed to mtx_init to create a mutex object that supports timeout;
*/

int  mtx_init     ( mtx_t *mtx, int type );
int  mtx_lock     ( mtx_t *mtx );
int  mtx_timedlock( mtx_t *restrict mtx, const struct timespec *restrict ts );
int  mtx_trylock  ( mtx_t *mtx );
int  mtx_unlock   ( mtx_t *mtx );
void mtx_destroy  ( mtx_t *mtx );


/*Description
The mtx_init function creates a mutex object with properties indicated by type, which must have one of the six values:
mtx_plain for a simple non-recursive mutex,
mtx_timed for a non-recursive mutex that supports timeout, ∗ mtx_plain | mtx_recursive for a simple recursive mutex, or
mtx_timed | mtx_recursive for a recursive mutex that supports timeout.
If the mtx_init function succeeds, it sets the mutex pointed to by mtx to a value that
uniquely identifies the newly created mutex.
Returns
The mtx_init function returns thrd_success on success, or thrd_error if the request could not be honored.*/

/*Description
The mtx_lock function blocks until it locks the mutex pointed to by mtx. If the mutex is non-recursive, it shall not be locked by the calling thread. Prior calls to mtx_unlock on the same mutex shall synchronize with this operation.
Returns
The mtx_lock function returns thrd_success on success, or thrd_error if the ∗ request could not be honored.*/

/*Description
2 The mtx_trylock function endeavors to lock the mutex pointed to by mtx. If the ∗ mutex is already locked, the function returns without blocking. If the operation succeeds, prior calls to mtx_unlock on the same mutex shall synchronize with this operation.
Returns
3 The mtx_trylock function returns thrd_success on success, or thrd_busy if the resource requested is already in use, or thrd_error if the request could not be honored.*/

/*Description
2 The mtx_timedlock function endeavors to block until it locks the mutex pointed to by mtx or until after the TIME_UTC-based calendar time pointed to by ts. The specified mutex shall support timeout. If the operation succeeds, prior calls to mtx_unlock on the same mutex shall synchronize with this operation.
Returns
3 The mtx_timedlock function returns thrd_success on success, or thrd_timedout if the time specified was reached without acquiring the requested resource, or thrd_error if the request could not be honored.*/

/*Description
2 The mtx_unlock function unlocks the mutex pointed to by mtx. The mutex pointed to by mtx shall be locked by the calling thread.
Returns
3 The mtx_unlock function returns thrd_success on success or thrd_error if the request could not be honored.*/

/*Description
2 The mtx_destroy function releases any resources used by the mutex pointed to by mtx. No threads can be blocked waiting for the mutex pointed to by mtx.
Returns
3 The mtx_destroy function returns no value.*/


typedef float thrd_t;
/* which is a complete object type that holds an identifier for a thread; */

/* thread start function. Charcoal?  I guess the first activity */
typedef int (*thrd_start_t)( void* );


         thrd_timedout
which is returned by a timed wait function to indicate that the time specified in the call was reached without acquiring the requested resource;

         thrd_success
which is returned by a function to indicate that the requested operation succeeded;

thrd_busy
which is returned by a function to indicate that the requested operation failed because a resource requested by a test and return function is already in use;

thrd_error
which is returned by a function to indicate that the requested operation failed; and

thrd_nomem
which is returned by a function to indicate that the requested operation failed because it was unable to allocate memory.
Forward references: date and time (7.27).

int            thrd_create ( thrd_t *thr, thrd_start_t func, void *arg );
thrd_t         thrd_current( void );
int            thrd_detach ( thrd_t thr );
int            thrd_equal  ( thrd_t thr0, thrd_t thr1 );
int            thrd_sleep  ( const struct timespec *duration, struct timespec *remaining );
void           thrd_yield  ( void );
_Noreturn void thrd_exit   ( int res );
int            thrd_join   ( thrd_t thr, int *res );


/* Description
The thrd_create function creates a new thread executing func(arg). If the thrd_create function succeeds, it sets the object pointed to by thr to the identifier of the newly created thread. (A thread’s identifier may be reused for a different thread once the original thread has exited and either been detached or joined to another thread.) The completion of the thrd_create function synchronizes with the beginning of the execution of the new thread.
Returns
The thrd_create function returns thrd_success on success, or thrd_nomem if no memory could be allocated for the thread requested, or thrd_error if the request could not be honored.*/

/*Description
The thrd_current function identifies the thread that called it.
Returns
The thrd_current function returns the identifier of the thread that called it. */

/*Description
The thrd_detach function tells the operating system to dispose of any resources allocated to the thread identified by thr when that thread terminates. The thread identified by thr shall not have been previously detached or joined with another thread.
Returns
The thrd_detach function returns thrd_success on success or thrd_error if the request could not be honored.*/

/*Description
The thrd_equal function will determine whether the thread identified by thr0 refers to the thread identified by thr1.
Returns
The thrd_equal function returns zero if the thread thr0 and the thread thr1 refer to different threads. Otherwise the thrd_equal function returns a nonzero value.*/

/*Description
2 The thrd_sleep function suspends execution of the calling thread until either the interval specified by duration has elapsed or a signal which is not being ignored is received. If interrupted by a signal and the remaining argument is not null, the amount of time remaining (the requested interval minus the time actually slept) is stored in the interval it points to. The duration and remaining arguments may point to the same object.
3 The suspension time may be longer than requested because the interval is rounded up to an integer multiple of the sleep resolution or because of the scheduling of other activity by the system. But, except for the case of being interrupted by a signal, the suspension time shall not be less than that specified, as measured by the system clock TIME_UTC.
Returns
4 The thrd_sleep function returns zero if the requested time has elapsed, -1 if it has been interrupted by a signal, or a negative value if it fails.*/

/*Description
2 The thrd_yield function endeavors to permit other threads to run, even if the current thread would ordinarily continue to run.
Returns
3 The thrd_yield function returns no value. */

/*Description
The thrd_exit function terminates execution of the calling thread and sets its result code to res.
The program shall terminate normally after the last thread has been terminated. The behavior shall be as if the program called the exit function with the status EXIT_SUCCESS at thread termination time.
Returns
The thrd_exit function returns no value. */

/*Description
The thrd_join function joins the thread identified by thr with the current thread by blocking until the other thread has terminated. If the parameter res is not a null pointer, it stores the thread’s result code in the integer pointed to by res. The termination of the
other thread synchronizes with the completion of the thrd_join function. The thread identified by thr shall not have been previously detached or joined with another thread.
Returns
3 The thrd_join function returns thrd_success on success or thrd_error
if the request could not be honored. */











     int tss_create(tss_t *key, tss_dtor_t dtor);
Description
The tss_create function creates a thread-specific storage pointer with destructor dtor, which may be null.
Returns
If the tss_create function is successful, it sets the thread-specific storage pointed to by key to a value that uniquely identifies the newly created pointer and returns thrd_success; otherwise, thrd_error is returned and the thread-specific storage pointed to by key is set to an undefined value.
7.26.6.2 Thetss_deletefunction Synopsis
     #include <threads.h>
     void tss_delete(tss_t key);
Description
The tss_delete function releases any resources used by the thread-specific storage identified by key.
Returns
The tss_delete function returns no value. 7.26.6.3 Thetss_getfunction Synopsis
     #include <threads.h>
     void *tss_get(tss_t key);
Description
The tss_get function returns the value for the current thread held in the thread-specific storage identified by key.
Returns
The tss_get function returns the value for the current thread if successful, or zero if unsuccessful.
ISO/IEC 9899:201x Committee Draft — April 12, 2011 N1570
386 Library §7.26.6.3
1
2
3
7.26.6.4 Thetss_setfunction Synopsis
     #include <threads.h>
     int tss_set(tss_t key, void *val);
Description
The tss_set function sets the value for the current thread held in the thread-specific storage identified by key to val.
Returns
The tss_set function returns thrd_success on success or thrd_error if the request could not be honored. ∗


typedef struct _TET_MACHINE _TET_MACHINE, *TET_MACHINE;
typedef struct _TET_PROCESS _TET_PROCESS, *TET_PROCESS;
typedef struct _TET_THREAD  _TET_THREAD,  *TET_THREAD;
typedef struct _TET_TASK    _TET_TASK,    *TET_TASK;
/* uni- and bi-directional channels.  All channels must be attached
 * to a task in order to be used. */
typedef struct _TET_CHANNEL _TET_CHANNEL, *TET_CHANNEL;
/* A channel port ("ch_port") is one end of a channel.  All sending
 * and receiving actually happens on ports, not channels. */
typedef struct _TET_CH_PORT _TET_CH_PORT, *TET_CH_PORT;
/* Channel types are an interesting area for further exploration */

typedef struct

activity_t activity_self( void );

typedef enum
{
    TETRC_SUCCESS,
} TET_RESULT_CODE;

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

/* While executing inside a task atomic block, a task will never
 * switch control to another task.  This provides no synchronization
 * with respect to other threads, processes or machines. */
TET_RESULT_CODE tet_task_atomic_enter();
TET_RESULT_CODE tet_task_atomic_leave();
#define TET_TASK_ATOMIC( STMT ) \
    do { \
        tet_task_atomic_enter(); \
        STMT; \
        tet_task_atomic_leave(); \
    } while( 0 )

typedef struct _TET_MUTEX_MACHINE _TET_MUTEX_MACHINE, *TET_MUTEX_MACHINE;
typedef struct _TET_MUTEX_PROCESS _TET_MUTEX_PROCESS, *TET_MUTEX_PROCESS;
typedef struct _TET_MUTEX_THREAD  _TET_MUTEX_THREAD,  *TET_MUTEX_THREAD;
typedef struct _TET_MUTEX_TASK    _TET_MUTEX_TASK,    *TET_MUTEX_TASK;

/* Charcoal primitives like mutexes, semaphores, barriers work with
 * activities and threads, not processes or machines */

/* These structs should be treated as opaque.  They're defined here so
 * that user code can see what size they are. */
struct mutex_t
{
    int x;
};

struct semaphore_t
{
    channel
};

struct barrier_t
{
};
