#include <charcoal.h>

#include <assert.h>
#include <charcoal_runtime_coroutine.h>
#include <charcoal_runtime_io_commands.h>

/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "crcl(replace_main)".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

#define app_main_prologue crcl(fn_prologue___charcoal_application_main)
#define app_main_epilogueB crcl(fn_epilogueB___charcoal_application_main)

static void crcl(yield_heartbeat)( int sig, siginfo_t *info, void *uc );
static int crcl(init_yield_heartbeat)();
cthread_p crcl(main_thread);
activity_t crcl(main_activity);
int crcl(process_return_value);

void app_main_epilogueB(
    crcl(frame_p) caller, int *lhs );

crcl(frame_p) app_main_prologue(
    crcl(frame_p) caller, void *ret_addr, int argc, char **argv, char **env );

static int start_application_main( int argc, char **argv, char **env );

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
    int rc;
    if( ( rc = zlog_init( "charcoal_log.conf" ) ) )
    {
        return -1;
    }

    if( !( crcl(c) = zlog_get_category( "main_cat" ) ) )
    {
        zlog_error( crcl(c), "Failure: Missing logging category\n" );
        zlog_fini();
        return -2;
    }

    if( ( rc = crcl(init_io_loop)( &io_thread, &io_activity ) ) )
    {
        zlog_error( crcl(c), "Failure: Initialization of the I/O loop: %d\n", rc );
        return rc;
    }

    if( ( rc = start_application_main( argc, argv, env ) ) )
    {
        zlog_error( crcl(c), "Failure: Launch of application main: %d\n", rc );
        return rc;
    }

    assert( !crcl(init_yield_heartbeat)() );
    crcl(activity_start_resume)( &io_activity );
    if( ( rc = uv_run( crcl(io_loop), UV_RUN_DEFAULT ) ) )
    {
        zlog_error( crcl(c), "Failure: Running the I/O loop: %d", rc );
        return rc;
    }

    zlog_info( crcl(c), "Charcoal app finished!!! return code:%d\n",
               crcl(process_return_value) );
    zlog_fini();
    return crcl(process_return_value);
}

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
    /* zlog_debug( crcl(c), "Establishing handler for signal %d\n", SIG); */
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

    zlog_info( crcl(c), "timer ID is 0x%lx\n", (long) 42 /*crcl(heartbeat_timer)*/ );
    return 0;
}

static int start_application_main( int argc, char **argv, char **env )
{
    zlog_info( crcl(c), "Starting app main %d %p %p\n", argc, argv, env );
    int rc;
    /* There's nothing particularly special about the thread that runs
     * the application's 'main' procedure.  The application will
     * continue running until all its threads finish (or exit is called
     * or whatever). */
    rc = thread_start( &crcl(main_thread), NULL /* options */ );
    if( rc )
    {
        return rc;
    }

    crcl(frame_p) main_frame = app_main_prologue( 0, 0, argc, argv, env );
    if( !main_frame )
    {
        return -3;
    }
    crcl(frame_p) next_frame = activate_in_thread(
        crcl(main_thread), &crcl(main_activity), main_frame,
        (crcl(epilogueB_t))app_main_epilogueB );
    if( !next_frame )
    {
        return -4;
    }
    uv_cond_signal( &crcl(main_thread)->thd_management_cond );
    return 0;
}
