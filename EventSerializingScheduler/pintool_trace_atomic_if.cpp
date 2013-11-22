#include "pin.H"
#include "atomic.hpp"

struct {
    int buffer_space1[20];
    int the_key;
    int buffer_space2[20];
} foo;

int *key = &foo.the_key;

static ADDRINT trace_if( void )
{
    return ATOMIC::OPS::Load( key );
}

static void trace_then( void )
{
    ATOMIC::OPS::Store( key, 0 );
}

static void trace_instrument( TRACE trace, VOID *v )
{
    TRACE_InsertIfCall  ( trace, IPOINT_BEFORE, (AFUNPTR)trace_if,   IARG_END );
    TRACE_InsertThenCall( trace, IPOINT_BEFORE,          trace_then, IARG_END );
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
