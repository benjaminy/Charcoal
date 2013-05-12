
void __charcoal_yield()
{
    /* examine some stuff. */
    if( 0 )
    {
        /* invoke the scheduler */
    }
}

void __charcoal_activity_set_return_value( void *ret_val_ptr )
{
}

void __charcoal_activity_get_return_value( __charcoal_activity_t *act, void *ret_val_ptr )
{
}

void *__charcoal_start_activity( void *activity_stuff )
{
    __charcoal_activity_t _self;
    activity_stuff->f( activity_stuff->args );
    /* wait around until the activity should deallocate itself */
}

/* static assert PTHREAD_STACK_MIN > sizeof activity */


/* I think the Charcoal type for activities and the C type need to be
 * different.  The Charcoal type should have the return type as a
 * parameter. */
__charcoal_activity_t *__charcoal_activate( void (*f)( void *args ), void *args )
{
    size_t stack_size = ( 1000000 / PTHREAD_STACK_MIN ) * PTHREAD_STACK_MIN;
    void *new_stack = malloc( stack_size );
    if( NULL == new_stack )
    {
        exit( 1 );
    }
    __charcoal_activity_info_t *act_info = (__charcoal_activity_info_t *)new_stack;
    act_info->f = f;
    act_info->args = args;
    sem_init( &act_info->sem, 0 );
    new_stack += PTHREAD_STACK_MIN /* effective base of stack */;
    pthread_attr_t attr;
    int rc = pthread_attr_init( &attr );
    int rc = pthread_attr_setstack( &attr, new_stack, stack_size - PTHREAD_STACK_MIN );
    // int pthread_attr_setdetachstate ( &attr, int detachstate );
    // int pthread_attr_setinheritsched( &attr, int inheritsched);
    // int pthread_attr_setschedparam  ( &attr, const struct sched_param *restrict param);
    // int pthread_attr_setschedpolicy ( &attr, int policy);
    // int pthread_attr_setscope       ( &attr, int contentionscope);

    int rc = pthread_create( &act_info->_self, &attr, __charcoal_start_activity, act_info );
    int rc = pthread_attr_destroy( &attr );
    sem_post( &act_info->sem );
    sem_wait();
}
