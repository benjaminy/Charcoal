/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/probe/CoreProbes.h"

#include "bindings/core/v8/V8BindingForCore.h"
#include "core/CoreProbeSink.h"
#include "core/events/Event.h"
#include "core/events/EventTarget.h"
#include "core/inspector/InspectorCSSAgent.h"
#include "core/inspector/InspectorDOMDebuggerAgent.h"
#include "core/inspector/InspectorNetworkAgent.h"
#include "core/inspector/InspectorPageAgent.h"
#include "core/inspector/InspectorSession.h"
#include "core/inspector/InspectorTraceEvents.h"
#include "core/inspector/MainThreadDebugger.h"
#include "core/inspector/ThreadDebugger.h"
#include "core/page/Page.h"
#include "core/workers/WorkerGlobalScope.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/loader/fetch/FetchInitiatorInfo.h"

/* BEGIN CHARCOAL */

// extern const char *CHARCOAL_BLAH;

struct json_hack {
    const char * name;
    /* 0 = string
       1 = number
       2 = object
       3 = array
       4 = bool
       5 = ptr
       6 = int */
    char kind;
    union {
        double d;
        const char *s;
        const void *p;
        int i;
    } value;
};

static FILE *dumb_file = NULL;

#define HACK_BUF_SZ 10000

#define P(...) \
    do { \
        printed = snprintf( &buf[ total_printed ], HACK_BUF_SZ - total_printed, __VA_ARGS__ ); \
        total_printed += printed; \
    } while( 0 )

void closeDumbFile()
{
    fprintf( stderr, "closeDumbFileC %p\n", dumb_file );
    if( dumb_file && dumb_file != stderr )
    {
        fclose( dumb_file );
    }
}

void printJsonObject( const char *phase, const void *task, struct json_hack *entries )
{
    static pid_t old_pid = 0;
    pid_t pid = getpid();
    if( old_pid != pid )
    {
        char fname[ 100 ];
        sprintf( fname, "./Traces/pc_%u.XXXXXX", pid );
        if( !mktemp( fname ) )
        {
            fprintf( stderr, "mktemp FAILED!!!!" );
            exit( 1 );
        }
        dumb_file = fopen( fname, "w" );
        fprintf( stderr, "CoreProbes.cpp %p %s\n", dumb_file, fname );
        if( dumb_file )
        {
            if( atexit( closeDumbFile ) )
            {
                fprintf( stderr, "atexit FAILED!!!!!\n" );
            }
        }
        else
            dumb_file = stderr;
    }
    old_pid = pid;

    char buf[ HACK_BUF_SZ ];
    int total_printed = 0;
    int printed;

    P( "[ " );
    P( "%lld, ", ( TimeTicks::Now() - *( new TimeTicks() ) ).InMicroseconds() );
    P( "%u, ", pid );
    P( "%u, ", base::PlatformThread::CurrentId() );
    P( "\"macro\", " );
    if( task )
        P( "\"%p\", ", task );
    else
        P( "null, " );
    P( "\"%s\", { ", phase );

    int i = 0;
    while( entries[ i ].name )
    {
        if( i > 0 )
            P( ", " );

        P( "\"%s\": ", entries[ i ].name );
        switch( entries[ i ].kind )
        {
        case 0:
            P( "\"%s\"", entries[ i ].value.s ? entries[ i ].value.s : "" );
            break;
        case 1:
            P( "%f", entries[ i ].value.d );
            break;
        case 4:
            P( "%s", entries[ i ].value.i ? "true" : "false" );
            break;
        case 5:
            if( entries[ i ].value.p )
                P( "\"%p\"", entries[ i ].value.p );
            else
                P( "null" );
            break;
        case 6:
            P( "%i", entries[ i ].value.i );
            break;
        default:
            fprintf( stderr, "CoreProbes.cpp BAD KIND: %i (%i)\n", entries[ i ].kind, i );
            exit( 1 );
        }
        ++i;
    }
    P( " } ]\n" );
    fprintf( dumb_file, "%s", buf );
    fflush( dumb_file );
}

void ctxDesc( blink::ExecutionContext *c, char *b )
{
    if( !c )
    {
        b[ 0 ] = 0;
        return;
    }
    b[  0 ] = c->IsDocument()                     ? 'A' : 'a';
    b[  1 ] = c->IsWorkerOrWorkletGlobalScope()   ? 'B' : 'b';
    b[  2 ] = c->IsWorkerGlobalScope()            ? 'C' : 'c';
    b[  3 ] = c->IsWorkletGlobalScope()           ? 'D' : 'd';
    b[  4 ] = c->IsMainThreadWorkletGlobalScope() ? 'E' : 'e';
    b[  5 ] = c->IsDedicatedWorkerGlobalScope()   ? 'F' : 'f';
    b[  6 ] = c->IsSharedWorkerGlobalScope()      ? 'G' : 'g';
    b[  7 ] = c->IsServiceWorkerGlobalScope()     ? 'H' : 'h';
    b[  8 ] = c->IsCompositorWorkerGlobalScope()  ? 'I' : 'i';
    b[  9 ] = c->IsAnimationWorkletGlobalScope()  ? 'J' : 'j';
    b[ 10 ] = c->IsAudioWorkletGlobalScope()      ? 'K' : 'k';
    b[ 11 ] = c->IsPaintWorkletGlobalScope()      ? 'L' : 'l';
    b[ 12 ] = c->IsThreadedWorkletGlobalScope()   ? 'M' : 'm';
    b[ 13 ] = c->IsJSExecutionForbidden()         ? 'N' : 'n';
    b[ 14 ] = c->IsContextThread()                ? 'O' : 'o';
    b[ 15 ] = 0;
}
/* END CHARCOAL */

namespace blink {
namespace probe {

AsyncTask::AsyncTask(ExecutionContext* context,
                     void* task,
                     const char* step,
                     bool enabled)
    : debugger_(enabled ? ThreadDebugger::From(ToIsolate(context)) : nullptr),
      task_(task),
      recurring_(step) {
  if (recurring_) {
    TRACE_EVENT_FLOW_STEP0("devtools.timeline.async", "AsyncTask",
                           TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)),
                           step ? step : "");
  } else {
    TRACE_EVENT_FLOW_END0("devtools.timeline.async", "AsyncTask",
                          TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)));
  }

  /* BEGIN CHARCOAL */
  char d[ 16 ];
  ctxDesc( context, d );
  // d[ 0 ] = 0;
  json_hack vs[] = {
      { "ctx",      0, { .s = d } },
      { "ctx_ptr",  5, { .p = context } },
      { "step",     0, { .s = step } },
      { 0, 0, { .i = 0 } }
  };
  printJsonObject( "ctor", task, vs );
  /* END CHARCOAL */

  if (debugger_)
    debugger_->AsyncTaskStarted(task_);
}

AsyncTask::~AsyncTask() {

  /* BEGIN CHARCOAL */
  json_hack vs[] = {
      { "recurring", 4, { .i = !!recurring_ } },
      { 0, 0, { .i = 0 } }
  };
  printJsonObject( "dtor", task_, vs );
  /* END CHARCOAL */

  if (debugger_) {
    debugger_->AsyncTaskFinished(task_);
    if (!recurring_)
      debugger_->AsyncTaskCanceled(task_);
  }
}

void AsyncTaskScheduled(ExecutionContext* context,
                        const String& name,
                        void* task) {
  TRACE_EVENT_FLOW_BEGIN1("devtools.timeline.async", "AsyncTask",
                          TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)),
                          "data", InspectorAsyncTask::Data(name));

  /* BEGIN CHARCOAL */
  char d[ 16 ];
  ctxDesc( context, d );
  char dumb[ 1000 ];
  strlcpy( dumb, name.Utf8().data(), 1000 );
  json_hack vs[] = {
      { "ctx",      0, { .s = d } },
      { "ctx_ptr",  5, { .p = context } },
      { "name",     0, { .s = dumb } },
      { 0, 0, { .i = 0 } }
  };
  printJsonObject( "scheduled", task, vs );
  /* END CHARCOAL */

  if (ThreadDebugger* debugger = ThreadDebugger::From(ToIsolate(context)))
    debugger->AsyncTaskScheduled(name, task, true);
}

void AsyncTaskScheduledBreakable(ExecutionContext* context,
                                 const char* name,
                                 void* task) {
  AsyncTaskScheduled(context, name, task);
  breakableLocation(context, name);
}

void AsyncTaskCanceled(ExecutionContext* context, void* task) {

  /* BEGIN CHARCOAL */
  char d[ 16 ];
  ctxDesc( context, d );
  json_hack vs[] = {
      { "ctx",      0, { .s = d } },
      { "ctx_ptr",  5, { .p = context } },
      { 0, 0, { .i = 0 } }
  };
  printJsonObject( "canceled", task, vs );
  /* END CHARCOAL */

  if (ThreadDebugger* debugger = ThreadDebugger::From(ToIsolate(context)))
    debugger->AsyncTaskCanceled(task);
  TRACE_EVENT_FLOW_END0("devtools.timeline.async", "AsyncTask",
                        TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)));
}

void AsyncTaskCanceledBreakable(ExecutionContext* context,
                                const char* name,
                                void* task) {
  AsyncTaskCanceled(context, task);
  breakableLocation(context, name);
}

void AllAsyncTasksCanceled(ExecutionContext* context) {

  /* BEGIN CHARCOAL */
  char d[ 16 ];
  ctxDesc( context, d );
  json_hack vs[] = {
      { "ctx",      0, { .s = d } },
      { "ctx_ptr",  5, { .p = context } },
      { 0, 0, { .i = 0 } }
  };
  printJsonObject( "all_canceled", NULL, vs );
  /* END CHARCOAL */

  if (ThreadDebugger* debugger = ThreadDebugger::From(ToIsolate(context)))
    debugger->AllAsyncTasksCanceled();
}

void DidReceiveResourceResponseButCanceled(LocalFrame* frame,
                                           DocumentLoader* loader,
                                           unsigned long identifier,
                                           const ResourceResponse& r,
                                           Resource* resource) {
  didReceiveResourceResponse(frame->GetDocument(), identifier, loader, r,
                             resource);
}

void CanceledAfterReceivedResourceResponse(LocalFrame* frame,
                                           DocumentLoader* loader,
                                           unsigned long identifier,
                                           const ResourceResponse& r,
                                           Resource* resource) {
  DidReceiveResourceResponseButCanceled(frame, loader, identifier, r, resource);
}

void ContinueWithPolicyIgnore(LocalFrame* frame,
                              DocumentLoader* loader,
                              unsigned long identifier,
                              const ResourceResponse& r,
                              Resource* resource) {
  DidReceiveResourceResponseButCanceled(frame, loader, identifier, r, resource);
}

}  // namespace probe
}  // namespace blink
