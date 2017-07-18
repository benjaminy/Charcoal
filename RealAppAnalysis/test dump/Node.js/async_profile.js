//var asyncListener = require("async-listener");
var AsyncProfile = require("async-profile");
//the "h" prefix stands for handler
//Ex: hCreate means handler upon creation

process.nextTick(function () {
 
    // Now start profiling. The profile will include all 
    // callbacks created while the current callback is running. 
    new AsyncProfile()
 
    // Finally queue up the work to be done asynchronously. 
    process.nextTick(init);
});

function init(){
	setImmediate(function a() {
    	setImmediate(function b() {
        	setImmediate(function c() {
            	setImmediate(function d() {
                	setImmediate(function e() {
                		console.log("Hello");
                	});
            	});
        	});
    	});
	});
};