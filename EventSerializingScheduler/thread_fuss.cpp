/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing system calls
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

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

#define FLAG_EVENT_HANDLER_STYLE 1
#define FLAG_POST_SYSCALL        2
#define FLAG_POST_SPAWN          4

typedef struct _thread_info_t _thread_info_t, *thread_info_t;

struct _thread_info_t
{
    OS_THREAD_ID   os_tid, os_ptid;
    THREADID       tid;
    PIN_THREAD_UID utid;
    unsigned int   flags;
    thread_info_t  next;
    PIN_SEMAPHORE  run_sem;
};

static unsigned int event_threads_not_blocked;
static TLS_KEY thread_info;
static FILE * trace;
static thread_info_t thread_queue_front, thread_queue_back;
static PIN_MUTEX threads_mtx;

void enqueue_thread( thread_info_t t )
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

void dequeue_thread( thread_info_t *t )
{
    assert_ns( t );
    assert_ns( ( thread_queue_front && thread_queue_back )
               || ( !thread_queue_front && !thread_queue_back ) );
    if( thread_queue_front )
    {
        *t = thread_queue_front;
        thread_queue_front = thread_queue_front->next;
        if( !thread_queue_front )
        {
            thread_queue_back = NULL;
        }
        (*t)->next = NULL;
    }
    else
    {
        *t = NULL;
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

VOID handle_syscall_entry(
    THREADID threadIndex,
    CONTEXT *ctxt,
    SYSCALL_STANDARD std,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
        thread_info, PIN_ThreadId() ) ) );
    self->flags |= FLAG_POST_SYSCALL;
    --event_threads_not_blocked;
    if( 0 /* is thread spawn */ )
    {
    }
    else
    {
        /* TODO: Figure out if this syscall might block or not. */
        thread_info_t next;
        dequeue_thread( &next );
        if( next )
        {
            PIN_SemaphoreSet( &next->run_sem );
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
}

void analyze_post_syscall( void )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    if( !( self->flags & FLAG_POST_SYSCALL ) )
    {
        /* Weird case where the program jumps to the instruction
         * directly after a syscall. */
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    self->flags &= ~FLAG_POST_SYSCALL;
    ++event_threads_not_blocked;
    if( 1 < event_threads_not_blocked )
    {
        fprintf( trace, "!!!!!! WOW 1 %u\n", event_threads_not_blocked );
        fflush( trace );
        assert_ns( thread_queue_back || ( 2 == event_threads_not_blocked ) );
        thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
        enqueue_thread( self );
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->run_sem );
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
    self->flags   = FLAG_EVENT_HANDLER_STYLE | FLAG_POST_SPAWN;
    self->next    = NULL;
    assert_ns( PIN_SemaphoreInit( &self->run_sem ) );

    assert_ns( PIN_SetThreadData(
                thread_info, self, PIN_ThreadId() ) );
    PIN_MutexUnlock( &threads_mtx );
}

void analyze_post_spawn( void )
{
    PIN_MutexLock( &threads_mtx );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    if( !( self->flags & FLAG_POST_SPAWN ) )
    {
        /* Weird case where the thread entry code executes "again". */
        PIN_MutexUnlock( &threads_mtx );
        return;
    }
    self->flags &= ~FLAG_POST_SPAWN;
    ++event_threads_not_blocked;
    fprintf( trace, "idx: %i  thread START  %u\n", self->tid,
             event_threads_not_blocked );
    fflush( trace );
    if( 1 < event_threads_not_blocked )
    {
        assert_ns( thread_queue_back || 2 == event_threads_not_blocked );
        /* The following block is probably not necessary.  It's
         * purpose is to ensure that a newly created thread gets a
         * chance to run before its parent.  It doesn't always
         * accomplish that, but it does sometimes. */
        if( thread_queue_back && self->os_ptid == thread_queue_back->os_tid )
        {
            thread_info_t temp;
            while( self->os_ptid != thread_queue_front->os_tid )
            {
                dequeue_thread( &temp );
                enqueue_thread( temp );
            }
            enqueue_thread( self );
            dequeue_thread( &temp ); /* should be the parent */
            enqueue_thread( temp );
        }
        else
        {
            enqueue_thread( self );
        }
        fprintf( trace, "!!!!!! WOW 3\n" );
        fflush( trace );
        PIN_MutexUnlock( &threads_mtx );
        PIN_SemaphoreWait( &self->run_sem );
        fprintf( trace, "!!!!!! WOW 4\n" );
        fflush( trace );
    }
    else
    {
        PIN_MutexUnlock( &threads_mtx );
    }
}

void instrument_trace( TRACE trace, VOID *v )
{
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
            thread_info, PIN_ThreadId() ) ) );
    if( self->flags & FLAG_POST_SPAWN )
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_post_spawn, IARG_END );
    if( self->flags & FLAG_POST_SYSCALL )
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_post_syscall, IARG_END );
}

void handle_thread_fini(
    THREADID threadIndex,
    const CONTEXT *ctxt,
    INT32 code,
    VOID *v )
{
    PIN_MutexLock( &threads_mtx );
    fprintf( trace, "idx: %i  thread FIN\n", threadIndex );
    thread_info_t self = (thread_info_t)(assert_ns( PIN_GetThreadData(
        thread_info, PIN_ThreadId() ) ) );
    PIN_SemaphoreFini( &self->run_sem );
    free( self );
    --event_threads_not_blocked;
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
    thread_queue_front = NULL;
    thread_queue_back = NULL;


    PIN_AddThreadStartFunction  ( handle_thread_start,  NULL );
    PIN_AddThreadFiniFunction   ( handle_thread_fini,   NULL );
    PIN_AddSyscallEntryFunction ( handle_syscall_entry, NULL );
    PIN_AddSyscallExitFunction  ( handle_syscall_exit,  NULL );
    PIN_AddContextChangeFunction( handle_ctx_change,    NULL );
    TRACE_AddInstrumentFunction ( instrument_trace,     NULL );
    PIN_AddFiniFunction         (Fini, 0);


    // Never returns
    PIN_StartProgram();
    
    return 0;
}
