/*
 * Top Matter
 */

/* Calls to the next two functions will be recorded (by name) in the event trace */

var b;
var e;

var CHARCOAL_BEGIN_RECORDING_TRACE = function ()
{
    console.log( "Event Recorder Begin" );
    // alert( "begin" );
    b.className = "hidden";
    e.className = "visible";
    chrome.storage.sync.set( { on: true } );
};

var CHARCOAL_END_RECORDING_TRACE = function ()
{
    console.log( "Event Recorder End" );
    // alert( "end" );
    e.className = "hidden";
    b.className = "visible";
    chrome.storage.sync.set( { on: false } );
};

document.addEventListener( "DOMContentLoaded", () => {
    console.log( "WOOOO" );
    console.log( Object.keys( chrome ) );
    console.log( Object.keys( chrome.processes ) );

    chrome.storage.sync.get( "on", ( items ) => {
        console.log( "Event Recorder Loaded" );
        // alert( "yay" );

        b = document.getElementById( "recording_off" );
        e = document.getElementById( "recording_on" );
        b.addEventListener( "click", CHARCOAL_BEGIN_RECORDING_TRACE );
        e.addEventListener( "click", CHARCOAL_END_RECORDING_TRACE );

        if( ( !chrome.runtime.lastError ) && items.on )
        {
            b.className = "hidden";
            e.className = "visible";
        }
    } );
} );
