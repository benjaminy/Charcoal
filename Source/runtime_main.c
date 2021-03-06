#include <core.h>

#include <assert.h>
#include <runtime_coroutine.h>
#include <runtime_io_commands.h>

/* The Charcoal preprocessor replaces all instances of "main" in
 * Charcoal code with "crcl(replace_main)".  The "real" main is
 * provided by the runtime library below.
 *
 * "main" might actually be something else (like "WinMain"), depending
 * on the platform. */

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

#define app_main_prologue crcl(fn_prologue___charcoal_application_main)

cthread_t crcl(main_thread);
activity_t crcl(main_activity);
int crcl(process_exit_code);

char *zlog_config_full_filename;

crcl(frame_p) app_main_prologue(
    crcl(frame_p) caller, void *ret_addr, int *lhs, int argc, char **argv, char **env );

static int __argc;
static char **__argv;
static char **__env;

static int start_application_main( void );

extern long crcl(switch_cnt);

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
    int rc;
    if( ( rc = zlog_init( zlog_config_full_filename ) ) )
    {
        return -1;
    }

    if( !( crcl(c) = zlog_get_category( "main_cat" ) ) )
    {
        zlog_error( crcl(c), "Failure: Missing logging category\n" );
        zlog_fini();
        return -2;
    }

    __argc = argc;
    __argv = argv;
    __env  = env;
    if( ( rc = crcl(init_io_loop)( start_application_main ) ) )
    {
        zlog_error( crcl(c), "Failure: Initialization of the I/O loop: %d\n", rc );
        return rc;
    }

    if( ( rc = uv_run( crcl(evt_loop), UV_RUN_DEFAULT ) ) )
    {
        zlog_error( crcl(c), "Failure: Running the I/O loop: %d", rc );
        return rc;
    }

    zlog_info( crcl(c), "switch cnt: %ld", crcl(switch_cnt) );
    zlog_info( crcl(c), "Charcoal application finished.  Exit code: %d",
               crcl(process_exit_code) );
    zlog_fini();
    return crcl(process_exit_code);
}

static int start_application_main( void )
{
    zlog_info( crcl(c), "Charcoal application starting.  argc: %d  argv: %p  env: %p",
               __argc, __argv, __env );
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

    /* XXX This is getting a little hacky */
    // crcl(main_activity).oldest_frame.activity = &crcl(main_activity);
    activity_t dummy_act;
    crcl(frame_t) dummy_frm;
    dummy_frm.activity = &dummy_act;
    crcl(frame_p) main_frame = app_main_prologue(
        &dummy_frm, 0, &crcl(process_exit_code), __argc, __argv, __env );
    if( !main_frame )
    {
        return -3;
    }
    activate_in_thread(
        &crcl(main_thread),
        &crcl(main_activity),
        &dummy_frm,
        main_frame );
    crcl(push_ready_queue)( &crcl(main_activity) );
    crcl(main_thread).running = &crcl(main_activity);
    uv_cond_signal( &crcl(main_thread).thd_management_cond );
    return 0;
}
