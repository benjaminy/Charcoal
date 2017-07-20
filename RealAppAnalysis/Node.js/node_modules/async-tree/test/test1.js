'use strict';
require('superstacktrace');


setImmediate(function a() {
    foo( );
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

function foo(){
  var ben = "a cat. Ben says: MEOOOOOOOOOOOOOW";
}

function bar(){
  var miguel = "an architect astronaut";
}
