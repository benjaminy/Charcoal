#include <unistd.h>
#include <string.h>
#include <netdb.h>

static const int DEFAULT_URLS_TO_GET = 10;
static const int DEFAULT_START_IDX   = 0;

void print_dns_info( const char *name, struct addrinfo *info );
void get_cmd_line_args( int argc, char **argv, int *urls );
const char *pick_name( int idx );

extern int dns_error_count;
extern struct addrinfo hints;

#define FINISH_DNS(code) { \
    printf( "\nFinal error count: %d\n", dns_error_count ); \
    return code; \
}
