#include <charcoal_runtime.h>
#include <stdio.h>
#include <math.h>

void fa( void *a )
{
}

int __charcoal_replace_main( int argc, char **argv )
{
    __charcoal_activity_t* a = __charcoal_activate(fa, NULL );
    __charcoal_activity_t* b = __charcoal_activate(fa, NULL );
}
