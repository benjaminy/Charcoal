require("asynctrace")

var state = 1;

init();

function init()
{
	a(b);
	var randomTime = 312; // > timeouts for a + b
	setInterval(d, randomTime);
}

function callbackDemonstration()
{
	if(state == 0 || state == 4)
	{
		state = 1;
		a(b);
	}
}

function a(func)
{
	if(state == 1)
	{
		state++;
		console.log("a");
		setTimeout(func, 100, c);
	}
}

function b(func)
{
	if(state == 2)
	{
		state++;
		console.log("b");
		setTimeout(func, 100, callbackDemonstration);
	}
}

function c(func)
{
	if(state == 3)
	{
		state++;
		console.log("c");
		var sec = 1000;
		setTimeout(func, sec);
	}
}

function d()
{
	if(state == 4)
	{
		console.log("d");
	}
	
	else
	{
		throw new Error("Callback overlap");
		console.log("d");
	}
}