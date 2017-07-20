'use strict';
require('superstacktrace');

var rand = get_random(10);
generate_chains( rand, get_random, 1000);


function get_random(upper_bound) {
  return Math.floor( (Math.random()*upper_bound) + 1 );
}


function generate_chains(rand, interval, upper_bound){

  for(var i = 0; i < rand; i++) {
    init_chain(interval(10), interval, upper_bound);
  }
}

function init_chain (calls, interval, upper_bound) {
  var counter = 0;
      function generate_call() {
        if (counter > calls)
          return;
        setTimeout(generate_call, interval(upper_bound));
        counter++;
      }

      generate_call()
}
