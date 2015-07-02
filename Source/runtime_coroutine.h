#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

#ifdef __CHARCOAL_CIL
/* #pragma cilnoremove("func1", "var2", "type foo", "struct bar") */
#pragma cilnoremove( "struct __charcoal_frame_t" )
#pragma cilnoremove( "struct activity_t" )
#pragma cilnoremove( "__charcoal_fn_generic_prologue" )
#pragma cilnoremove( "__charcoal_fn_generic_epilogueA" )
#pragma cilnoremove( "__charcoal_fn_generic_epilogueB" )
#pragma cilnoremove( "__charcoal_activate" )
#pragma cilnoremove( "__charcoal_activity_init" )
#pragma cilnoremove( "__charcoal_yield" )
#pragma cilnoremove( "__charcoal_yield_impl" )
#endif

#include <charcoal_runtime_common.h>
#include <charcoal_runtime_atomics.h>
#include <charcoal_semaphore.h>
#include <stdlib.h>
/* NOTE: Eventually make different dev/production configs that ifdef logging */
#include <zlog.h>
extern char *zlog_config_full_filename;

zlog_category_t *crcl( c );

crcl(frame_p) crcl(activity_start_resume)( activity_p activity );

typedef void (*crcl(epilogueB_t))( crcl(frame_p), void * );

void activate_in_thread(
    cthread_p, activity_p, crcl(frame_p), crcl(epilogueB_t) );

crcl(frame_p) crcl(activate)( crcl(frame_p), void *,
                    activity_p, crcl(frame_p), crcl(epilogueB_t) );

void crcl(activity_init)( activity_p act );

crcl(frame_p) crcl(activity_blocked)( crcl(frame_p) frame );

crcl(frame_p) crcl(fn_generic_prologue)(
    size_t sz, void *return_ptr, crcl(frame_p) caller,
    crcl(frame_p) (*fn)( crcl(frame_p) ) );

crcl(frame_p) crcl(fn_generic_epilogueA)( crcl(frame_p) frame );

void crcl(fn_generic_epilogueB)( crcl(frame_p) frame );

extern cthread_p crcl(main_thread);
extern activity_t crcl(main_activity);
extern int crcl(process_return_value);

int crcl(yield)( void );
crcl(frame_p) crcl(yield_impl)( crcl(frame_p) frame, void *ret_addr );

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
