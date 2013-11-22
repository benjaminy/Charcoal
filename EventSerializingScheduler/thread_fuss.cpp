/* Copyright Benjamin Ylvisaker */
/*
 *  This file contains an ISA-portable PIN tool for tracing system calls
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#if defined(TARGET_MAC)
#include <sys/syscall.h>
#elif !defined(TARGET_WINDOWS)
#include <syscall.h>
#endif

#include "pin.H"

/* assert_ns means "assert, no shortcut".  The trouble with regular
 * assert is that it can be compiled away entirely, which is bad if
 * there are import effects to evaluating "e".  This version of assert
 * also has the fanciness that it returns the parameter. */
#define assert_ns( e ) \
    ({ typeof( e ) x = e; \
       assert( x ); \
       x; })

#define safe_dec( e ) \
    ({ typeof( e ) x = e; \
       --e; \
       assert_ns( e < x ); \
       e; })

#define safe_inc( e ) \
    ({ typeof( e ) x = e; \
       ++e; \
       assert_ns( e > x ); \
       e; })

#define SET_FLAG( x, f )   ({ x |= f; })
#define UNSET_FLAG( x, f ) ({ x &= ~f; })

#define FLAG_EVENT_HANDLER_STYLE 1
#define FLAG_SYSCALL        2
#define FLAG_THREAD_ENTRY   4
#define FLAG_THREAD_CREATE  8

typedef struct _thread_info_t _thread_info_t, *thread_info_t;
struct _thread_info_t
{
    unsigned int   flags;
    /* So many IDs ... */
    OS_THREAD_ID   os_tid, os_ptid;
    THREADID       tid;
    PIN_THREAD_UID utid;
    /* Maybe a little premature optimization. Each thread is in two
     * lists: the one for all threads and either the one for event
     * threads or the one for computation threads.  The latter are
     * mutually exclusive, so we use just one pointer. */
    thread_info_t  next_all, next_ev_comp;
    thread_info_t  parent;
    PIN_SEMAPHORE  sem;
    PIN_MUTEX      mtx;
};

static bool threads_stopped, stop_requested;
static unsigned int event_threads_not_blocked, event_count;
static TLS_KEY thread_info;
static FILE * trace;
static PIN_MUTEX threads_mtx;
static thread_info_t need_resume;

static THREADID       watchdog_tid = INVALID_THREADID;
static PIN_THREAD_UID watchdog_utid = 0;
static PIN_SEMAPHORE  watchdog_sem;

static thread_info_t get_self( void )
{
    return (thread_info_t)(assert_ns( PIN_GetThreadData(
        thread_info, PIN_ThreadId() ) ) );
}

/* TQ = Thread Queue */
enum thread_kind
{
    ALL_THREAD,
    EV_THREAD,
    COMP_THREAD,
};

namespace TQ
{
    static thread_info_t all_fr, all_bk, ev_fr, ev_bk, comp_fr, comp_bk;

    void init( void )
    {
        all_fr  = NULL;
        all_bk  = NULL;
        ev_fr   = NULL;
        ev_bk   = NULL;
        comp_fr = NULL;
        comp_bk = NULL;
    }

    void front_back( thread_kind k, thread_info_t **f, thread_info_t **b )
    {
        assert_ns( f && b );
        switch( k )
        {
        case ALL_THREAD:
            *f = &all_fr;
            *b = &all_bk;
            break;
        case EV_THREAD:
            *f = &ev_fr;
            *b = &ev_bk;
            break;
        case COMP_THREAD:
            *f = &comp_fr;
            *b = &comp_bk;
            break;
        default:
            assert_ns( 0 );
            break;
        }
    }

#define DECL_FRONT_BACK( k ) \
    thread_info_t *fr, *bk; \
    front_back( k, &fr, &bk );

    thread_info_t *thr_next( thread_kind k, thread_info_t t )
    {
        switch( k )
        {
        case ALL_THREAD:
            return &t->next_all;
        case EV_THREAD:
        case COMP_THREAD:
            return &t->next_ev_comp;
        default:
            break;
        }
        assert_ns( 0 );
        return NULL;
    }

#define THR_NEXT( k, t ) ( *thr_next( k, t ) )

    bool is_empty( thread_kind k )
    {
        DECL_FRONT_BACK( k );
        assert_ns( !(*fr) == !(*bk) );
        return !(*fr);
    }

    thread_info_t peek_front( thread_kind k )
    {
        DECL_FRONT_BACK( k );
        return *fr;
    }

    thread_info_t peek_back( thread_kind k )
    {
        DECL_FRONT_BACK( k );
        return *bk;
    }

    void enqueue( thread_kind k, thread_info_t t )
    {
        DECL_FRONT_BACK( k );
        assert_ns( ( *fr && *bk ) || ( !(*fr) && !(*bk) ) );
        assert_ns( !THR_NEXT( k, t ) );
        if( *fr )
        {
            THR_NEXT( k, *bk ) = t;
            *bk = t;
        }
        else
        {
            *fr = *bk = t;
        }
    }

    void enqueue_front( thread_kind k, thread_info_t t )
    {
        DECL_FRONT_BACK( k );
        assert_ns( ( *fr && *bk ) || ( !(*fr) && !(*bk) ) );
        assert_ns( !THR_NEXT( k, t ) );
        if( *fr )
        {
            THR_NEXT( k, t ) = *fr;
            *fr = t;
        }
        else
        {
            *fr = *bk = t;
        }
    }

    void dequeue( thread_kind k, thread_info_t *t )
    {
        DECL_FRONT_BACK( k );
        assert_ns( ( *fr && *bk ) || ( !(*fr) && !(*bk) ) );
        if( t )
        {
            *t = NULL;
        }
        if( fr )
        {
            thread_info_t temp = *fr;
            *fr = THR_NEXT( k, *fr );
            if( !(*fr) )
            {
                *bk = NULL;
            }
            THR_NEXT( k, temp ) = NULL;
            if( t )
            {
                *t = temp;
            }
        }
    }

#undef THR_NEXT
} /* namespace TQ */

#define THR_NEXT( k, t ) ( *TQ::thr_next( k, t ) )

/* XXX: Not really sure what to do with ctx changes yet */
void handle_ctx_change(
    THREADID              threadIndex,
    CONTEXT_CHANGE_REASON reason,
    const CONTEXT        *from,
    CONTEXT              *to,
    INT32                 info,
    VOID                 *v )
{
    fprintf( trace, "idx: %i  CONTEXT CHANGE\n", threadIndex );
    fflush( trace );
}

static bool __attribute__((unused)) is_ev_thread( thread_info_t t )
{
    return t->flags & FLAG_EVENT_HANDLER_STYLE;
}

static bool is_comp_thread( thread_info_t t )
{
    return !( t->flags & FLAG_EVENT_HANDLER_STYLE );
}

/* XXX: Not at all portable */
bool is_thread_create(
    CONTEXT *ctxt,
    SYSCALL_STANDARD std )
{
    return
        SYS_clone == PIN_GetSyscallNumber( ctxt, std )
        && ( CLONE_THREAD & PIN_GetSyscallArgument( ctxt, std, 1 ) );
}

static void pause_other_threads( thread_info_t self )
{
    fprintf( trace, "tid: %4x pause_other\n", self->tid ); fflush( trace );
    /* TODO: In the future, be more polite about stopping other threads */
    assert_ns( PIN_StopApplicationThreads( self->tid ) );
    threads_stopped = true;
}

/* Assumptions:
 * - This procedure is only called from EV threads. */
static void resume_other_threads( thread_info_t self )
{
    fprintf( trace, "tid: %4x resume  stopped:%i\n", self->tid, threads_stopped );
    fflush( trace );
    if( threads_stopped )
    {
        PIN_ResumeApplicationThreads( self->tid );
        for( thread_info_t t = TQ::peek_front( COMP_THREAD );
             t;
             t = THR_NEXT( COMP_THREAD, t ) )
        {
            PIN_SemaphoreSet( &t->sem );
        }
    }
    threads_stopped = false;
}

static void handle_syscall_entry(
    THREADID threadIndex,
    CONTEXT *ctxt,
    SYSCALL_STANDARD std,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = get_self();
    fprintf( trace, "tid: %4x syscall_entry  q_empty?%i\n", threadIndex,
             TQ::is_empty( EV_THREAD ) );
    fflush( trace );
    /* TODO: maybe only do this if it's a blocking syscall. */
    if( threads_stopped )
    {
        need_resume = self;
        PIN_SemaphoreSet( &watchdog_sem );
    }
    SET_FLAG( self->flags, FLAG_SYSCALL );
    if( is_thread_create( ctxt, std ) )
    {
        SET_FLAG( self->flags, FLAG_THREAD_CREATE );
        PIN_SemaphoreClear( &self->sem );
    }
    else if( is_comp_thread( self ) )
    {
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    else /* if syscall is blocking */
    {
        assert_ns( TQ::peek_front( EV_THREAD ) == self );
        TQ::dequeue( EV_THREAD, NULL );
        thread_info_t next = TQ::peek_front( EV_THREAD );
        if( next )
        {
            PIN_SemaphoreSet( &next->sem );
        }
    }
    PIN_MutexUnlock( &threads_mtx );
}

VOID handle_syscall_exit(
    THREADID threadIndex,
    CONTEXT *ctxt,
    SYSCALL_STANDARD std,
    VOID *v)
{
    PIN_MutexLock( &threads_mtx );
    fprintf( trace, "tid: %4x syscall_exit\n", threadIndex ); fflush( trace );
    if( is_thread_create( ctxt, std )
        && ( 0 > PIN_GetSyscallReturn( ctxt, std ) ) )
    {
        thread_info_t self = get_self();
        UNSET_FLAG( self->flags, FLAG_THREAD_CREATE );
    }
    PIN_MutexUnlock( &threads_mtx );
}

void analyze_post_syscall( UINT32 sz )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = get_self();
    fprintf( trace, "tid: %4x post_syscall\n", self->tid ); fflush( trace );
    assert_ns( !THR_NEXT( EV_THREAD, self ) );
    if( !( self->flags & FLAG_SYSCALL ) )
    {
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    UNSET_FLAG( self->flags, FLAG_SYSCALL );
    if( self->flags & FLAG_THREAD_CREATE )
    {
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->sem );
        PIN_MutexLock( &threads_mtx );
    }
    if( is_comp_thread( self ) )
    {
        /* check for self-suspending? */
    }
    else if( TQ::is_empty( EV_THREAD ) )
    {
        /* Nobody else is trying to go.  Just do it. */
        TQ::enqueue( EV_THREAD, self );
        pause_other_threads( self );
    }
    else if( TQ::peek_front( EV_THREAD ) == self )
    {
        /* Decided to not switch for syscall */
        pause_other_threads( self );
    }
    else
    {
        TQ::enqueue( EV_THREAD, self );
        PIN_SemaphoreClear( &self->sem );
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->sem );
    }
    PIN_MutexUnlock( &threads_mtx );
    // ++event_threads_not_blocked;
}

static thread_info_t find_parent( thread_info_t self )
{
    thread_info_t t = TQ::peek_front( ALL_THREAD );
    if( !t )
    {
        /* This must be the original thread */
        return NULL;
    }
    /* Could definitely do better than linear, but this shouldn't be
     * called often, so it probably doesn't matter. */
    while( t )
    {
        if( self->os_ptid == t->os_tid )
        {
            return t;
        }
        t = THR_NEXT( ALL_THREAD, t );
    }
    assert_ns( false );
    return NULL;
}

static void handle_thread_start(
    THREADID threadIndex,
    CONTEXT *ctxt,
    INT32    flags,
    VOID    *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)assert_ns( malloc( sizeof( self[0] ) ) );
    fprintf( trace, "tid: %4x thread_start\n", threadIndex ); fflush( trace );
    self->os_tid       = PIN_GetTid();
    self->os_ptid      = PIN_GetParentTid();
    self->tid          = PIN_ThreadId();
    self->utid         = PIN_ThreadUid();
    self->flags        = FLAG_EVENT_HANDLER_STYLE | FLAG_THREAD_ENTRY;
    self->next_all     = NULL;
    self->next_ev_comp = NULL;
    self->parent       = find_parent( self );
    assert_ns( PIN_SemaphoreInit( &self->sem ) );
    assert_ns( PIN_MutexInit( &self->mtx ) );

    TQ::enqueue( ALL_THREAD, self );
    assert_ns( PIN_SetThreadData(
                thread_info, self, PIN_ThreadId() ) );
    PIN_MutexUnlock( &threads_mtx );
}

void analyze_thread_entry( UINT32 sz )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = get_self();
    fprintf( trace, "tid: %4x thread_entry\n", self->tid ); fflush( trace );
    if( !( self->flags & FLAG_THREAD_ENTRY ) )
    {
        /* Weird case where a thread entry point executes "again". */
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    pause_other_threads( self );
    UNSET_FLAG( self->flags, FLAG_THREAD_ENTRY );
    // ++event_threads_not_blocked;
    TQ::enqueue_front( EV_THREAD, self );
    fprintf( trace, "tid: %4x   q_empty?%i\n", self->tid, TQ::is_empty( EV_THREAD ) );
    fflush( trace );
    PIN_MutexUnlock( &threads_mtx );
}

struct {
    int buffer_space1[20];
    int the_key;
    int buffer_space2[20];
} foo;

int *key = &foo.the_key;

static ADDRINT trace_if( void )
{
    return ATOMIC::OPS::Load( key );
}

//    ATOMIC::OPS::Store( key, 0 );

static void trace_then( UINT32 sz )
{
    PIN_MutexLock( &threads_mtx );
    ADDRINT run_mode = ATOMIC::OPS::Load( key );
    if( !run_mode )
    {
        /* Very unlikely, but maybe possible */
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    thread_info_t self = get_self();
    if( self == running_event_thread )
    {
        /* check time? */
    }
    else
    {
        if( run_mode == 1 )
        {
            /* safe point yield */
        }
        else if( run_mode == 2 )
        {
            /* Yield immediately */
        }
        else
        {
            assert_ns( false );
        }
    }
    PIN_MutexUnlock( &threads_mtx );
}

static void instrument_trace( TRACE trace, VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = get_self();
    int entry   = !!( self->flags & FLAG_THREAD_ENTRY ),
        syscall = !!( self->flags & FLAG_SYSCALL );
    assert_ns( !( entry && syscall ) );
    UINT32 sz = (UINT32)TRACE_Size( trace );
    if( entry )
        TRACE_InsertCall( trace, IPOINT_BEFORE, (AFUNPTR)analyze_thread_entry,
                          IARG_UINT32, sz, IARG_END );
    else if( syscall )
        TRACE_InsertCall( trace, IPOINT_BEFORE, (AFUNPTR)analyze_post_syscall,
                          IARG_UINT32, sz, IARG_END );
    else
    {
        /* Common case.  Use if/then instrumentation for better performance. */
        TRACE_InsertIfCall  ( trace, IPOINT_BEFORE, (AFUNPTR)trace_if,   IARG_END );
        TRACE_InsertThenCall( trace, IPOINT_BEFORE, (AFUNPTR)trace_then,
                              IARG_UINT32, sz, IARG_END );


    }
    PIN_MutexUnlock( &threads_mtx );
}

void handle_thread_fini(
    THREADID threadIndex,
    const CONTEXT *ctxt,
    INT32 code,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    fprintf( trace, "tid: %x thread_fini\n", threadIndex ); fflush( trace );
    thread_info_t self = get_self();
    PIN_SemaphoreFini( &self->sem );
    free( self );
    // unsigned int temp = event_threads_not_blocked;
    /* XXX hm... do threads need to make a syscall to fini */
    // --event_threads_not_blocked;
    // fprintf( trace, "N D ready %u  %u\n", self->tid, event_threads_not_blocked ); fflush( trace );
    // assert_ns( event_threads_not_blocked < temp );
    assert_ns( PIN_SetThreadData( thread_info, NULL, PIN_ThreadId() ) );
    PIN_MutexUnlock( &threads_mtx );
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace,"#eof\n");
    fclose(trace);
    assert_ns( PIN_DeleteThreadDataKey( thread_info ) );
    PIN_MutexFini( &threads_mtx );
}


/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This tool does some thread shit" 
                + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

void watchdog( void *a )
{
    while( 1 )
    {
        if( PIN_IsProcessExiting() )
        {
            break;
        }
        PIN_MutexLock( &threads_mtx );
        // fprintf( trace, "watchdog loop\n" ); fflush( trace );
        if( need_resume )
        {
            resume_other_threads( need_resume );
            need_resume = NULL;
        }
        else if( TQ::is_empty( EV_THREAD ) )
        {
            /* No event threads are ready to run.  Wait on the
             * semaphore.  We need a timeout to avoiding hanging on
             * process termination. */
            PIN_SemaphoreClear( &watchdog_sem );
            PIN_MutexUnlock( &threads_mtx );
            BOOL is_set = PIN_SemaphoreTimedWait(
                &watchdog_sem, 100/*millisecond*/ );
            if( is_set )
                PIN_SemaphoreClear( &watchdog_sem );
        }
        else
        {
            struct timespec sleep_time;
            sleep_time.tv_sec  = 0;
            sleep_time.tv_nsec = 100000;
            unsigned int old_event_count = event_count;
            PIN_MutexUnlock( &threads_mtx );
            while( nanosleep( &sleep_time, &sleep_time ) ) { }
            PIN_MutexLock( &threads_mtx );
            if( old_event_count == event_count )
            {
                /* The currently executing thread is not an event
                 * thread. */
            }
            PIN_MutexUnlock( &threads_mtx );
        }
    }
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if( PIN_Init( argc, argv ) ) return Usage();

    trace = fopen( "strace.out", "w" );
    thread_info = PIN_CreateThreadDataKey( NULL );
    assert_ns( PIN_MutexInit( &threads_mtx ) );
    event_threads_not_blocked = 0;
    TQ::init();
    threads_stopped = false;
    stop_requested  = false;
    need_resume = NULL;

    PIN_AddThreadStartFunction  ( handle_thread_start,  NULL );
    PIN_AddThreadFiniFunction   ( handle_thread_fini,   NULL );
    PIN_AddSyscallEntryFunction ( handle_syscall_entry, NULL );
    PIN_AddSyscallExitFunction  ( handle_syscall_exit,  NULL );
    PIN_AddContextChangeFunction( handle_ctx_change,    NULL );
    TRACE_AddInstrumentFunction ( instrument_trace,     NULL );
    PIN_AddFiniFunction         (Fini, 0);

    assert_ns( PIN_SemaphoreInit( &watchdog_sem ) );
    watchdog_tid = PIN_SpawnInternalThread(
        watchdog, NULL, 0, &watchdog_utid );
    assert_ns( watchdog_tid != INVALID_THREADID );

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
