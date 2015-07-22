#ifndef __CHARCOAL_RUNTIME
#define __CHARCOAL_RUNTIME

#include <zlog.h>

/* NOTE: Eventually make different dev/production configs that ifdef logging */
extern zlog_category_t *crcl( c );
extern uv_key_t crcl(self_key);

void crcl(push_ready_queue)( activity_p a, cthread_p t );
activity_p crcl(pop_ready_queue)( cthread_p t );
void crcl(push_waiting_queue)( activity_p a, activity_p *q );
activity_p crcl(pop_waiting_queue)( activity_p *q );

/* join thread t.  Return True if t was the last application
 * thread. */
int crcl(join_thread)( cthread_p t );

activity_p crcl(get_self_activity)( void );
void crcl(set_self_activity)( activity_p a );
int crcl(activity_detach)( activity_p );

#endif /* __CHARCOAL_RUNTIME */
