'use strict';

if(!process.addAsyncListener){
    require('async-listener');
}

/**
 * AsyncListener object
 */
var AsyncListener = {};

var ID = 0;
var tree = [];

/**
Called when an asynchronous event is queued. Recieves the storage attached to the listener. 
Storage can be created by passing an initialStorage argument during costruction, 
or by returning a Value from create which will be attached to the listner and overwrite the initialStorage.
**/

AsyncListener.create = function create(storage) {
};

/**
A function (context, storage) that is called immediately before the asynchronous callback is about to run. 
It will be passed both the context (i.e. this) of the calling function and the storage.
**/
AsyncListener.before = function before(context, storage) {
};

/**
A function (context, storage) called immediately after the asynchronous event's callback is run. 
Note that if the event's callback throws during execution this will not be called.
**/
AsyncListener.after = function after(context, storage) {
};

/**
A function (storage, error) called if the event's callback threw. 
If error returns true then Node will assume the error has been properly handled and resume execution normally.
**/
AsyncListener.error = function error(storage, error){
};

var key = process.addAsyncListener(AsyncListener);
function printID(){console.log(ID);}
process.on('exit', printID);
exports.module = key;
