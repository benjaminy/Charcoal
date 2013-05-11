/*
 * TetraStak Concurrency Library
 */

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

typedef struct
{
    channel
} semaphore;
