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
#define app_main_epilogueB crcl(fn_epilogueB___charcoal_application_main)

cthread_p crcl(main_thread);
activity_t crcl(main_activity);
int crcl(process_return_value);

crcl(frame_p) app_main_prologue(
    crcl(frame_p) caller, void *ret_addr, int argc, char **argv, char **env );
void app_main_epilogueB( crcl(frame_p) caller, int *lhs );

static int __argc;
static char **__argv;
static char **__env;

static int start_application_main( void );

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
    if( ( rc = crcl(init_io_loop)( &io_thread, &io_activity, start_application_main ) ) )
    {
        zlog_error( crcl(c), "Failure: Initialization of the I/O loop: %d\n", rc );
        return rc;
    }

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

static int start_application_main( void )
{
    zlog_info( crcl(c), "Starting app main %d %p %p\n", __argc, __argv, __env );
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
    crcl(frame_p) main_frame = app_main_prologue(
        /*XXX*/crcl(main_activity).oldest_frame, 0, __argc, __argv, __env );
    if( !main_frame )
    {
        return -3;
    }
    activate_in_thread( crcl(main_thread), &crcl(main_activity), main_frame,
        (crcl(epilogueB_t))app_main_epilogueB );
    crcl(push_ready_queue)( &crcl(main_activity), crcl(main_thread) );
    uv_cond_signal( &crcl(main_thread)->thd_management_cond );
    return 0;
}
