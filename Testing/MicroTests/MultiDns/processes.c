#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <multi_dns_utils.h>

int error_count = 0;

pid_t get_one( int idx )
{
    const char *name = pick_name( idx );
    pid_t child = fork();
    if( !child )
    {
        struct addrinfo *info;
        int rc = getaddrinfo( name, NULL, NULL, &info );
        if( rc )
        {
            printf( "ERROR %d %d %s\n", rc, idx, name );
            ++error_count;
        }
        else
        {
            print_dns_info( name, info );
        }
        /* Don't care about deallocating info, because we're
         * exiting */
        exit( 0 );
    }
    return child;
}

int main( int argc, char **argv, char **env )
{
    int urls_to_get;
    get_cmd_line_args( argc, argv, &urls_to_get );
    pid_t *child_handles =
        (pid_t *)malloc( urls_to_get * sizeof(child_handles[0]) );

    for( int i = 0; i < urls_to_get; ++i )
    {
        child_handles[i] = get_one( i );
    }
    printf( "\nErrors in thread creation? %d\n", error_count );

    for( int i = 0; i < urls_to_get; ++i )
    {
        int thread_return;
        pid_t p = waitpid( child_handles[i], NULL, 0 );
        if( p == -1 )
        {
            printf( "ERROR IN wait %d\n", errno );
            ++error_count;
        }
    }
    printf( "\nERROR COUNT: %d\n", error_count );
    return 0;
}
