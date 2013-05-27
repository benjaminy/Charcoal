#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

volatile int foo = 0;

struct timeval const foot  = {1,0};
struct timeval const foot2 = {1,0};

static void inter2( int sig, siginfo_t *siginfo, void *context )
{
    printf( "EXIT\n" );
    exit( foo );
}

static void __charcoal_timer_handler( int sig, siginfo_t *siginfo, void *context )
{
    struct itimerval value;
    value.it_interval = foot;
    value.it_value = foot2;
    ++foo;
    printf( "hello\n" );
    assert( 0 == setitimer( ITIMER_REAL, &value, NULL ) );
}

int main( int argc, char **argv )
{
    struct sigaction sigact;
    sigact.sa_sigaction = __charcoal_timer_handler;
    sigact.sa_flags = SA_SIGINFO;
    assert( 0 == sigemptyset( &sigact.sa_mask ) );
    assert( 0 == sigaction( SIGALRM, &sigact, NULL ) );


    struct sigaction sigact2;
    sigact2.sa_sigaction = inter2;
    sigact2.sa_flags = SA_SIGINFO;
    assert( 0 == sigemptyset( &sigact2.sa_mask ) );
    assert( 0 == sigaction( SIGINT, &sigact2, NULL ) );


    struct itimerval value;
    value.it_interval = foot;
    value.it_value = foot2;
    assert( 0 == setitimer( ITIMER_REAL, &value, NULL ) );
    while ( 1 ) {}
    return foo;
}
