#ifndef __CHARCOAL_RUNTIME
#define __CHARCOAL_RUNTIME

extern uv_key_t crcl(self_key);

void crcl(push_special_queue)(
    unsigned queue_flag, activity_p a, cthread_p t, activity_p *qp );
activity_p crcl(pop_special_queue)(
    unsigned queue_flag, cthread_p t, activity_p *qp );

void crcl(push_ready_queue)( activity_p a, cthread_p t );
activity_p crcl(pop_ready_queue)( cthread_p t );

/* join thread t.  Return True if t was the last application
 * thread. */
int crcl(join_thread)( cthread_p t );

activity_p crcl(get_self_activity)( void );
void crcl(set_self_activity)( activity_p a );
int crcl(activity_detach)( activity_p );

#endif /* __CHARCOAL_RUNTIME */
