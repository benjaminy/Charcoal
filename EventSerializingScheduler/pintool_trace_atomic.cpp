#include "pin.H"
#include "atomic.hpp"

struct {
    int buffer_space1[20];
    int the_key;
    int buffer_space2[20];
} foo;

int *key = &foo.the_key;

void trace_analyze( void )
{
    if( ATOMIC::OPS::Load( key ) )
        ATOMIC::OPS::Store( key, 0 );
}

void trace_instrument( TRACE trace, VOID *v )
{
    TRACE_InsertCall( trace, IPOINT_BEFORE, trace_analyze, IARG_END );
}

int main(int argc, char *argv[])
{
    if( PIN_Init(argc,argv) )
    {
        return -1;
    }
    TRACE_AddInstrumentFunction( trace_instrument, 0 );
    PIN_StartProgram();
    return 0;
}
