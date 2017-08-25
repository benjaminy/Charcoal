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

namespace blink {
namespace probe {

void ctxDesc( ExecutionContext *c, char *b )
{
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

// printf -> fprintf
//   - fopen
//   - temp names
//   - process id in the file name
// more info
// chrome extension for start/stop
// timestamp

FILE * file_hack( void )
{
    static FILE * f = NULL;
    static pid_t pid = 0;
    if( !f || pid != getpid() )
    {
        pid = getpid();
        /* include pid in file name */
        f = fopen( "/tmp/blah.txt", "w" );
    }
    return f;
}

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
  char d[ 16 ];
  ctxDesc( context, d );
  printf( "{ \"micro\": 0, \"phase\": 2, \"ctx\": \"%s\", \"task_ptr\": %p, \"ctx_ptr\": %p, "
          "\"step\": \"%s\" }\n",
          d, task, context, step ? step : "" );
  if (debugger_)
    debugger_->AsyncTaskStarted(task_);
}

AsyncTask::~AsyncTask() {
  printf( "{ \"micro\": 0, \"phase\": 3, \"task_ptr\": %p, \"recurring\": %s }\n",
          task_, recurring_ ? "yes" : "no" );
  if( !recurring_ )
      printf( "{ \"micro\": 0, \"phase\": 3, \"task_ptr\": %p }\n", task_ );
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
