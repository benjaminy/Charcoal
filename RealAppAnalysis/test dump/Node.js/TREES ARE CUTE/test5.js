'use strict';
require('superstacktrace');



var p = new Promise( function a(){
  console.log("hello")
});
p .then(foo)



function foo(){
  return "foo";
}

function hello(){
  console.log("hello");
}

function goodbye(){
  console.log("goodbye");
}
