var asyncListener = require("async-listener");
//the "h" prefix stands for handler
//Ex: hCreate means handler upon creation

function hCreate(storage){
}

function hBefore(context, storage){
}

function hAfter(context, storage){
}

function hError(storage, error){
}

var handlers = {create: hCreate, before: hBefore, after: hAfter, error: hError};

var initValue = "Value"

/**
The process object is a global that provides information about, and control over, 
the current Node.js process. As a global, it is always available to Node.js 
applications without using require()
**/

al = process.createAsyncListener(handlers, initValue);
process.addAsyncListener(handlers, initValue);

var dummy = function(){console.log("hello")};
setImmediate(dummy);
new Promise(dummy);
