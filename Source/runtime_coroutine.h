#ifndef __CHARCOAL_RUNTIME_COROUTINE
#define __CHARCOAL_RUNTIME_COROUTINE

#ifdef __CHARCOAL_CIL
/* #pragma cilnoremove("func1", "var2", "type foo", "struct bar") */
#endif

#include <core_runtime.h>
#include <runtime_semaphore.h>
#include <stdlib.h>
extern char *zlog_config_full_filename;

zlog_category_t *crcl( c );

crcl(frame_p) crcl(activity_start_resume)( activity_p activity );

void activate_in_thread(
    cthread_p,
    activity_p,
    crcl(frame_p),
    crcl(frame_p) );

crcl(frame_p) crcl(activity_waiting)( crcl(frame_p) frame );

extern cthread_t crcl(main_thread);
extern activity_t crcl(main_activity);
extern int crcl(process_return_value);

#endif /* __CHARCOAL_RUNTIME_COROUTINE */
