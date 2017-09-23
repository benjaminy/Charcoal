/*
 * Top Matter
 */

var printProcess = function printProcess( process, prefix )
{
    console.log( prefix + " " + process.osProcessId + " " + process.type );
};

var doIt = function()
{
    chrome.processes.getProcessInfo( [], false, ( processes ) => {
        console.log( "PROCESSES" );
        for( id in processes )
        {
            printProcess( processes[ id ], "T" );
        }
        chrome.processes.onCreated.addListener( ( process ) => {
            printProcess( process, "A" );
        } );
    } );
};

doIt();
