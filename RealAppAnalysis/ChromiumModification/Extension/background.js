/*
 * Top Matter
 */

var printProcess = function printProcess( process, prefix )
{
    console.log( "CHARCOAL_PROCESS_INFO " + prefix + " " + process.osProcessId + " " + process.type );
};

var main = function()
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

main();
