#include <charcoal.h>

#include <assert.h>
#include <charcoal_runtime_coroutine.h>
#include <charcoal_runtime_io_commands.h>

#include <stdio.h>

/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "crcl(replace_main)".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

#define app_main_prologue crcl(fn_prologue___charcoal_application_main)

/* yield_heartbeat is the signal handler that should run every few
 * milliseconds (give or take) when any activity is running.  It
 * atomically modifies the activity state so that the next call to
 * yield will do a more thorough check to see if it should switch to
 * another activity.
 *
 * XXX Actually, this only really needs to be armed if there are any
 * threads with more than one ready (or running) activity. */
static void crcl(yield_heartbeat)( int sig, siginfo_t *info, void *uc )
{
    if( sig != 0 /*XXX SIG*/ )
    {
        /* XXX Very weird */
        exit( sig );
    }

    void *p = info->si_value.sival_ptr;
    if( p == &crcl(threads) )
    {
        /* XXX Is it worth worrying about a weird address collision
         * here? */
        /* XXX disarm if no activities are running */
        /* XXX For all threads: check flags, decr unyielding */
    }
    else
    {
        /* XXX pass the signal on to the application? */
        exit( -sig );
    }
}

static int crcl(init_yield_heartbeat)()
{
    /* Establish handler for timer signal */
    struct sigaction sa;
    /* printf("Establishing handler for signal %d\n", SIG); */
    sa.sa_flags     = SA_SIGINFO;
    sa.sa_sigaction = crcl(yield_heartbeat);
    sigemptyset( &sa.sa_mask );
    // RET_IF_ERROR( sigaction( 0 /*SIG*/, &sa, NULL ) );

#if 0
    /* XXX deprecated??? Create the timer */
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = 0 /*XXX SIG*/;
    sev.sigev_value.sival_ptr = &crcl(threads);
    // XXX RET_IF_ERROR( timer_create( 0 /*CLOCKID*/, &sev, &crcl(heartbeat_timer) ) );
#endif

    printf( "timer ID is 0x%lx\n", (long) 42 /*crcl(heartbeat_timer)*/ );
    return 0;
}

static int process_return_value;

crcl(frame_p) app_main_prologue(
    crcl(frame_p) caller,
    int argc,
    char **argv,
    char **env );

static crcl(frame_p) after_main( crcl(frame_p) frame );
static void initialize_after_main( activity_p activity, crcl(frame_p) frame );
static void start_application_main( int argc, char **argv, char **env );

/* Architecture note: For the time being (as of early 2014, at least),
 * we're using libuv to handle asynchronous I/O stuff.  It would be
 * lovely if we could "embed" libuv's event loop in the yield logic.
 * Unfortunately, libuv embedding is not totally solidified yet.  So
 * we're going to have a separate thread for running the I/O event
 * loop.  Eventually we should be able to get rid of that.
 *
 * ... On the other hand, having a separate thread for the event loop
 * means that we can do the heartbeat timer there and not have to worry
 * about signal handling junk.  Seems like that might be a nicer way to
 * go for many systems.  In the long term, probably should support
 * both. */
int main( int argc, char **argv, char **env )
{
    /* Okay to stack-allocate these here because the I/O thread should
     * always be the last thing running in a Charcoal process.
     * TODO: Document why we need these for the I/O thread */
    cthread_t  io_thread;
    activity_t io_activity;
    assert( !crcl(init_io_loop)( &io_thread, &io_activity ) );

    start_application_main( argc, argv, env );

    assert( !crcl(init_yield_heartbeat)() );
    crcl(activity_start_resume)( &io_activity );
    // XXX crcl(create_thread)( thd, act, crcl(main_activity_entry), &params );

    int rc = uv_run( crcl(io_loop), UV_RUN_DEFAULT );

    if( rc )
    {
        printf( "Error running the I/O loop: %i", rc );
        exit( rc );
    }

    // printf( "Charcoal program finished!!! return code:%d\n", process_return_value );
    return process_return_value;
}

static void crcl(remove_activity_from_thread)( activity_p a, cthread_p t)
{
    /* printf( "Remove activity %p  %p\n", a, t ); */
    if( a == a->next )
    {
        t->activities = NULL;
    }
    if( t->activities == a )
    {
        t->activities = a->next;
    }
    a->next->prev = a->prev;
    a->prev->next = a->next;
    a->next = NULL;
    a->prev = NULL;

    // printf( "f:%p  r:%p\n", t->activities, t->activities->prev );
}

static crcl(frame_p) after_main( crcl(frame_p) frame )
{
    printf( "[CRCL_RT] after_main %p\n", frame );
    /* XXX call epilogueB */
    int *p = (int *)frame->callee->specific;
    process_return_value = *p;
    free( frame->callee );
    free( frame );
    crcl(remove_activity_from_thread)( frame->activity, frame->activity->thread );
    printf( "[CRCL_RT] after_main finished %d\n", process_return_value );
    return NULL;
}

static void initialize_after_main( activity_p activity, crcl(frame_p) frame )
{
    frame->activity     = activity;
    frame->fn           = after_main;
    frame->caller       = NULL;
    frame->callee       = NULL;
    frame->goto_address = NULL;
}

static void start_application_main( int argc, char **argv, char **env )
{
    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    cthread_p main_thread;
    assert( !thread_start( &main_thread, NULL /* options */ ) );

    /* XXX leak? Maybe we should have an auto-free option for activities */
    activity_p main_activity = (activity_p)malloc( sizeof( main_activity[0] ) );
    assert( main_activity );
    crcl(frame_p) after_main_frame = (crcl(frame_p))malloc( sizeof( after_main_frame[0] ) );
    assert( after_main_frame );
    initialize_after_main( main_activity, after_main_frame );
    crcl(frame_p) main_frame = app_main_prologue( after_main_frame, argc, argv, env );
    assert( main_frame );
    assert( !activate_in_thread( main_thread, main_activity, main_frame ) );
    uv_cond_signal( &main_thread->thd_management_cond );
}
