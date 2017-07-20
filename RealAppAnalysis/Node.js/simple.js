require("async-tree")

function main(){
	submain();
}

function submain(){
	setImmediate(() => console.log("Hello!"));
}

function performanceTest(){
	var iterations = 1000000;
	var cur = 0;
	console.log("Date now")
	var start = Date.now();
	while(cur < iterations){
		Date.now();
		cur++;
	}
	cur = 0;
	var end = Date.now();
	var dateNowTime = end - start;
	console.log(dateNowTime)

	console.log("hrTime")
	start = Date.now();
	while(cur < iterations){
		cur++
		process.hrtime();
	}

	end = Date.now();
	var hrTime = end - start;
	console.log(hrTime)

}

main();
