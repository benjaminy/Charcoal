var asyncListener = require("async-listener");
//the "h" prefix stands for handler
//Ex: hCreate means handler upon creation

function hCreate(storage){
	console.log(storage);
}

function hBefore(context, storage){
	console.log("Something is about to happen?");
	console.log(context);
}

function hAfter(context, storage){
	console.log("Something has happened");
	console.log(context);
}

function hError(storage, error){
	console.log("Somee error");
	console.log(storage);
}

var handlers = {create: hCreate, before: hBefore, after: hAfter, error: hError};

var initValue = "Value"

/**
The process object is a global that provides information about, and control over, 
the current Node.js process. As a global, it is always available to Node.js 
applications without using require()
**/

al = process.createAsyncListener(handlers, initValue);
process.addAsyncListener(al);

setImmediate(function(){console.log("Hello")}, 1);
