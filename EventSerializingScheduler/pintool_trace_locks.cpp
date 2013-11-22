#include "pin.H"

PIN_MUTEX mtx;
int key;

void trace_analyze( void )
{
    PIN_MutexLock( &mtx );
    if( key )
        key = 0;
    PIN_MutexUnlock( &mtx );
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
    key = 1;
    PIN_MutexInit( &mtx );
    TRACE_AddInstrumentFunction( trace_instrument, 0 );
    PIN_StartProgram();
    return 0;
}
