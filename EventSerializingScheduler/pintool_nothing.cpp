#include "pin.H"

int main(int argc, char *argv[])
{
    if( PIN_Init(argc,argv) )
    {
        return -1;
    }
    PIN_StartProgram();
    return 0;
}
