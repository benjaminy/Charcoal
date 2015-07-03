#ifndef __CHARCOAL_RUNTIME
#define __CHARCOAL_RUNTIME

#if 0
#include <unistd.h>
#include <charcoal_runtime_io_commands.h>
#endif

#include <uv.h>

#define __CHARCOAL_SET_FLAG(x,f)   do { (x).flags |=  (f); } while( 0 )
#define __CHARCOAL_CLEAR_FLAG(x,f) do { (x).flags &= ~(f); } while( 0 )
#define __CHARCOAL_CHECK_FLAG(x,f) ( !!( (x).flags & (f) ) )

/* Thread flags */
#define __CHARCOAL_THDF_IDLE       (1 << 0)
#define __CHARCOAL_THDF_KEEP_ALIVE (1 << 1) /* even after last activity finishes */
#define __CHARCOAL_THDF_NEVER_RUN  (1 << 2)
#define __CHARCOAL_THDF_TIMER_ON   (1 << 3)

/* Activity flags */
#define __CHARCOAL_ACTF_DETACHED    (1 << 0)
#define __CHARCOAL_ACTF_BLOCKED     (1 << 1)
#define __CHARCOAL_ACTF_READY_QUEUE (1 << 2)
#define __CHARCOAL_ACTF_REAP_QUEUE  (1 << 3)
#define __CHARCOAL_ACTF_DONE        (1 << 4)

extern cthread_p crcl(threads);
extern uv_key_t crcl(self_key);

void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a, cthread_p t, activity_p *qp );
activity_p crcl(pop_special_queue)(
    unsigned queue_flag, cthread_p t, activity_p *qp );

void crcl(push_ready_queue)( activity_p a, cthread_p t );

/* join thread t.  Return True if t was the last application
 * thread. */
int crcl(join_thread)( cthread_p t );

activity_p crcl(get_self_activity)( void );
void crcl(set_self_activity)( activity_p a );
int crcl(activity_detach)( activity_p );

int thread_start( cthread_p *thd, void *options );

#endif /* __CHARCOAL_RUNTIME */
