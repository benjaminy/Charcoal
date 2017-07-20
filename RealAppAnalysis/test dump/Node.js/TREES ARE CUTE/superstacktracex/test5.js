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


function findallchildren( parent, calls ) {
  children = {};

  for (var i=0;i<calls.length;i++) {
    if(calls.current_parent == parent) {
      children[calls.current] = {};
    }
  }
  return children;
}

function find_subtree(calls){
  for (var i=0;i<calls.length;i++) {

    var children = findallchildren( i, calls );

    var parent = calls[i].current_parent;
    var child = calls[i].current;


}
  //key:value
  //parent:{}  <-- return dic

function getTree( calls ){
  var tree = {};
  var original = tree;
  var last_child = null;

  for (var i=0;i<calls.length;i++) {
    var parent = calls[i].current_parent;
    var child = calls[i].current;

    //traverse tree and get subtree

    if(parent in tree)
      tree[parent][child] = {};

    else {
      var dict = {};
      dict[child] = {};
      tree[parent] = dict; }

    if( parent === last_child) {

    }

  }
  return original;

}
