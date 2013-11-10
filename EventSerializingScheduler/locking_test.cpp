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
#include <unistd.h>

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

static FILE * trace;
static TLS_KEY thread_flags;

VOID SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
    unsigned int *my_flag = (unsigned int *)(assert_ns( PIN_GetThreadData(
            thread_flags, PIN_ThreadId() ) ) );
    *my_flag = 1;
}

void handle_thread_start(
    THREADID threadIndex,
    CONTEXT *ctxt,
    INT32 flags,
    VOID *v )
{
    unsigned int *my_flags = (unsigned int *)assert_ns(
        malloc( sizeof( my_flags[0] ) ) );
    *my_flags = 2;
    assert_ns( PIN_SetThreadData(
                thread_flags, my_flags, PIN_ThreadId() ) );
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf(trace,"#eof\n");
    fclose(trace);
}

void analyze_post_spawn( void )
{
    unsigned int *my_flag = (unsigned int *)(assert_ns( PIN_GetThreadData(
            thread_flags, PIN_ThreadId() ) ) );
    if( *my_flag == 2 )
    {
        *my_flag = 0;
        sleep( 1 );
    }
}

void analyze_post_syscall( void )
{
    unsigned int *my_flag = (unsigned int *)(assert_ns( PIN_GetThreadData(
            thread_flags, PIN_ThreadId() ) ) );
    if( *my_flag == 1 )
    {
        *my_flag = 0;
    }
}

void instrument_trace( TRACE trace, VOID *v )
{
    unsigned int *my_flag = (unsigned int *)(assert_ns( PIN_GetThreadData(
            thread_flags, PIN_ThreadId() ) ) );
    switch( *my_flag )
    {
    case 0: break; /* normal case */
    case 1:
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_post_syscall, IARG_END );
        break;
    case 2:
        TRACE_InsertCall( trace, IPOINT_BEFORE, analyze_post_spawn, IARG_END );
        break;
    default:
        assert_ns( 0 );
        break;
    }
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This tool prints a log of system calls" 
                + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("strace.out", "w");
    thread_flags = PIN_CreateThreadDataKey( NULL );

    PIN_AddSyscallEntryFunction ( SyscallEntry, 0);
    PIN_AddThreadStartFunction  ( handle_thread_start, NULL );
    TRACE_AddInstrumentFunction ( instrument_trace, NULL );
    PIN_AddFiniFunction         ( Fini, 0 );

    // Never returns
    PIN_StartProgram();
    
    return 0;
}
