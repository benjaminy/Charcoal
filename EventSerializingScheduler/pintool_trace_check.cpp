#include "pin.H"

TLS_KEY key;

void trace_analyze( void )
{
    if( PIN_GetThreadData( key, PIN_ThreadId() ) )
        PIN_SetThreadData( key, 0, PIN_ThreadId() );
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
    key = PIN_CreateThreadDataKey( 0 );
    TRACE_AddInstrumentFunction( trace_instrument, 0 );
    PIN_StartProgram();
    return 0;
}
