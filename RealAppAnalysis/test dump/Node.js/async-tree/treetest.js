function hello(){console.log("Hello!");}

function loop(number, func){
	for(var i = 0; i < number; i++){
		func();
	}
}

loop(18, function(){
	setTimeout(hello, 100);});
