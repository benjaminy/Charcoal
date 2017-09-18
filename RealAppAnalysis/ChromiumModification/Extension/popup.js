/*
 * Top Matter
 */

/* Calls to the next two functions will be recorded (by name) in the event trace */

var b;
var e;

var CHARCOAL_BEGIN_RECORDING_TRACE = function ()
{
    console.log( "begin" );
    // alert( "begin" );
    b.className = "hidden";
    e.className = "visible";
};

var CHARCOAL_END_RECORDING_TRACE = function ()
{
    console.log( "end" );
    // alert( "end" );
    e.className = "hidden";
    b.className = "visible";
};

document.addEventListener( "DOMContentLoaded", () => {
    console.log( "yay" );
    // alert( "yay" );

    b = document.getElementById( "recording_off" );
    e = document.getElementById( "recording_on" );
    b.addEventListener( "click", CHARCOAL_BEGIN_RECORDING_TRACE );
    e.addEventListener( "click", CHARCOAL_END_RECORDING_TRACE );
} );
