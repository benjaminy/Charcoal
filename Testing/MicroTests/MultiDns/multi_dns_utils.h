#include <netdb.h>

static const int    DEFAULT_URLS_TO_GET = 10;
static const int    DEFAULT_START_IDX   = 0;

void print_dns_info( const char *name, struct addrinfo *info );
void get_cmd_line_args( int argc, char **argv, int *urls, int *idx );
