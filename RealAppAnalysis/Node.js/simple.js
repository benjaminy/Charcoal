require("async-tree");
require("superstacktrace");

function main(){
	setImmediate(() => console.log("Hello!"));
}

main();
throw new Error();
