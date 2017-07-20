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
