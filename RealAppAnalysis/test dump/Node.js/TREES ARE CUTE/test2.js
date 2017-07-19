'use strict';
require('superstacktrace');

setTimeout(chain, 10);
setTimeout(chain, 0);

function chain () {
  setImmediate(function a() {
      setTimeout(function b() {
          setTimeout(function c() {
              setTimeout(function d() {
                  setTimeout(function e() {
                      console.log("the end 1");
                  }, 100);
              }, 10);
          }, 100);
      }, 10);
  } );
}


function hello(){
  console.log("hello");
}
function foo(callback){ console.log("hello"); callback();  }
function bar(){ console.log("bar"); }
