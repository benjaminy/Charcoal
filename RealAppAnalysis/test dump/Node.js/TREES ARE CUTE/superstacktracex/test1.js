'use strict';
require('superstacktrace');


setImmediate(function a() {
    foo( function hello() {console.log("blah");} );
    bar();
    setImmediate(function b() {
        setImmediate(function c() {
            setImmediate(function d() {
                setImmediate(function e() {
                    console.log("the end 1");
                });
            });
        });
    });
} );

function hello(){
  console.log("hello");
}
function foo(callback){ console.log("hello"); callback();  }
function bar(){ console.log("bar"); }
