'use strict';
require('superstacktrace');


init()


function init()
{
	a(b);
	//setTimeout(d, 300);
}

function a(func)
{
		console.log("a");
		setTimeout(func, 100, c);

}

function b(func)
{
		console.log("b");
		setTimeout(func, 100, d);
}

function c(func)
{
		console.log("c");
		setTimeout(func, 100);

}

function d()
{
	console.log("d. the end");
}
