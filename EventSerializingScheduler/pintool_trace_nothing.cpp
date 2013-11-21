#include "pin.H"

void trace_analyze( void )
{
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
