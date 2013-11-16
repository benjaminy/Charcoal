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
    thread_info_t  next;
    PIN_SEMAPHORE  sem;
    PIN_MUTEX      mtx;
};

static unsigned int event_threads_not_blocked, event_count;
static TLS_KEY thread_info;
static FILE * trace;
static PIN_MUTEX threads_mtx;

static THREADID       watchdog_tid = INVALID_THREADID;
static PIN_THREAD_UID watchdog_utid = 0;
static PIN_SEMAPHORE  watchdog_sem;

/* TQ = Thread Queue */
namespace TQ
{
    static thread_info_t thread_queue_front, thread_queue_back;

    void init( void )
    {
        thread_queue_front = NULL;
        thread_queue_back = NULL;
    }

    bool is_empty( void )
    {
        assert_ns( !thread_queue_front == !thread_queue_back );
        return !thread_queue_front;
    }

    thread_info_t peek_front( void )
    {
        return thread_queue_front;
    }

    thread_info_t peek_back( void )
    {
        return thread_queue_back;
    }

    void enqueue( thread_info_t t )
    {
        assert_ns( ( thread_queue_front && thread_queue_back )
                   || ( !thread_queue_front && !thread_queue_back ) );
        assert_ns( !t->next );
        if( thread_queue_front )
        {
            thread_queue_back->next = t;
            thread_queue_back = t;
        }
        else
        {
            thread_queue_front = t;
            thread_queue_back = t;
        }
    }

    void dequeue( thread_info_t *t )
    {
        assert_ns( ( thread_queue_front && thread_queue_back )
                   || ( !thread_queue_front && !thread_queue_back ) );
        if( t )
        {
            *t = NULL;
        }
        if( thread_queue_front )
        {
            thread_info_t temp = thread_queue_front;
            thread_queue_front = thread_queue_front->next;
            if( !thread_queue_front )
            {
                thread_queue_back = NULL;
            }
            temp->next = NULL;
            if( t )
            {
                *t = temp;
            }
        }
    }
}

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

/* XXX: Not at all portable */
bool is_thread_create(
    CONTEXT *ctxt,
    SYSCALL_STANDARD std )
{
    return
        SYS_clone == PIN_GetSyscallNumber( ctxt, std )
        && ( CLONE_THREAD & PIN_GetSyscallArgument( ctxt, std, 1 ) );
}

VOID handle_syscall_entry(
    THREADID threadIndex,
    CONTEXT *ctxt,
    SYSCALL_STANDARD std,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
        thread_info, PIN_ThreadId() ) ) );
    if( is_thread_create( ctxt, std ) )
    {
        self->flags |= FLAG_THREAD_CREATE;
    }
    else if( !( FLAG_EVENT_HANDLER_STYLE & self->flags ) )
    {
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    self->flags |= FLAG_SYSCALL;
    if( !TQ::is_empty() && TQ::peek_front() == self )
    {
        TQ::dequeue( NULL );
        if( !TQ::is_empty() )
        {
            PIN_SemaphoreSet( &TQ::peek_front()->sem );
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
    /* check for -1 */
}

void analyze_post_syscall( void )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    assert_ns( !self->next );
    if( !( self->flags & FLAG_SYSCALL ) )
    {
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    self->flags &= ~FLAG_SYSCALL;
    ++event_threads_not_blocked;
    //fprintf( trace, "Thread %3u post-syscall (sem: %i) (%u)\n", self->tid,
    //         PIN_SemaphoreIsSet( &self->sem ), event_threads_not_blocked );
    if( 1 < event_threads_not_blocked )
    {
        fflush( trace );
        assert_ns( !TQ::is_empty() || ( 2 == event_threads_not_blocked ) );
        TQ::enqueue( self );
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->sem );
        PIN_SemaphoreClear( &self->sem );
        //fprintf( trace, "Thread %3u post-syscall & released (%i) (%u)\n", self->tid,
        //         PIN_SemaphoreIsSet( &self->sem ), event_threads_not_blocked );
        // fflush( trace );
    }
    else
    {
        PIN_MutexUnlock( &threads_mtx );
    }
}

void handle_thread_start(
    THREADID threadIndex,
    CONTEXT *ctxt,
    INT32    flags,
    VOID    *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)assert_ns( malloc( sizeof( self[0] ) ) );
    self->os_tid  = PIN_GetTid();
    self->os_ptid = PIN_GetParentTid();
    self->tid     = PIN_ThreadId();
    self->utid    = PIN_ThreadUid();
    self->flags   = FLAG_EVENT_HANDLER_STYLE | FLAG_THREAD_ENTRY;
    self->next    = NULL;
    assert_ns( PIN_SemaphoreInit( &self->sem ) );
    assert_ns( PIN_MutexInit( &self->mtx ) );

    assert_ns( PIN_SetThreadData(
                thread_info, self, PIN_ThreadId() ) );
    PIN_MutexUnlock( &threads_mtx );
}

void analyze_entry( void )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    if( !( self->flags & FLAG_THREAD_ENTRY ) )
    {
        /* Weird case where the thread entry code executes "again". */
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    self->flags &= ~FLAG_THREAD_ENTRY;
    ++event_threads_not_blocked;
    // fprintf( trace, "N C ready %u  %u\n", self->tid, event_threads_not_blocked ); fflush( trace );
    //fprintf( trace, "idx: %i  thread START  %u\n", self->tid,
    //         event_threads_not_blocked );
    //fflush( trace );
    if( 1 < event_threads_not_blocked )
    {
        assert_ns( !TQ::is_empty() || 2 == event_threads_not_blocked );
        /* The following block is probably not necessary.  It's
         * purpose is to ensure that a newly created thread gets a
         * chance to run before its parent.  It doesn't always
         * accomplish that, but it does sometimes. */
        if( !TQ::is_empty() && self->os_ptid == TQ::peek_back()->os_tid )
        {
            thread_info_t temp;
            while( self->os_ptid != TQ::peek_front()->os_tid )
            {
                TQ::dequeue( &temp );
                // fprintf( trace, "A. stupid parent thing %u\n", temp->tid ); fflush( trace );
                TQ::enqueue( temp );
            }
            // fprintf( trace, "B. stupid parent thing %u\n", temp->tid ); fflush( trace );
            TQ::enqueue( self );
            TQ::dequeue( &temp ); /* should be the parent */
            // fprintf( trace, "C. stupid parent thing %u\n", temp->tid ); fflush( trace );
            TQ::enqueue( temp );
        }
        else
        {
            // fprintf( trace, "enqueue 5\n" ); fflush( trace );
            TQ::enqueue( self );
        }
        //fprintf( trace, "Thread %3u starting & waiting (%i)\n", self->tid,
        //         PIN_SemaphoreIsSet( &self->sem ) );
        //fflush( trace );
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->sem );
        PIN_SemaphoreClear( &self->sem );
        PIN_MutexLock( &threads_mtx );
        //fprintf( trace, "Thread %3u starting & released (%i)\n", self->tid,
        //         PIN_SemaphoreIsSet( &self->sem ) );
        //fflush( trace );
        PIN_MutexUnlock( &threads_mtx );
    }
    else
    {
        PIN_MutexUnlock( &threads_mtx );
    }
}

void instrument_trace( TRACE trace, VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    if( self->flags & FLAG_THREAD_ENTRY )
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_entry, IARG_END );
    if( self->flags & FLAG_SYSCALL )
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_post_syscall, IARG_END );
    PIN_MutexUnlock( &threads_mtx );
}

void handle_thread_fini(
    THREADID threadIndex,
    const CONTEXT *ctxt,
    INT32 code,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    // fprintf( trace, "idx: %i  thread FIN\n", threadIndex );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
        thread_info, PIN_ThreadId() ) ) );
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

void watchdog_entry( void *a )
{
    while( 1 )
    {
        if( PIN_IsProcessExiting() )
        {
            break;
        } 
        PIN_MutexLock( &threads_mtx );
        if( event_threads_not_blocked < 1 )
        {
            /* No event threads are ready to run.  Wait on the
             * semaphore.  We need a timeout to avoiding hanging on
             * process termination. */
            PIN_MutexUnlock( &threads_mtx );
            BOOL is_set = PIN_SemaphoreTimedWait(
                &watchdog_sem, 10/*millisecond*/ );
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

    PIN_AddThreadStartFunction  ( handle_thread_start,  NULL );
    PIN_AddThreadFiniFunction   ( handle_thread_fini,   NULL );
    PIN_AddSyscallEntryFunction ( handle_syscall_entry, NULL );
    PIN_AddSyscallExitFunction  ( handle_syscall_exit,  NULL );
    PIN_AddContextChangeFunction( handle_ctx_change,    NULL );
    TRACE_AddInstrumentFunction ( instrument_trace,     NULL );
    PIN_AddFiniFunction         (Fini, 0);

    assert_ns( PIN_SemaphoreInit( &watchdog_sem ) );
    watchdog_tid = PIN_SpawnInternalThread(
        watchdog_entry, NULL, 0, &watchdog_utid );
    if( watchdog_tid == INVALID_THREADID )
    {
        /* XXX die */
    }

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
