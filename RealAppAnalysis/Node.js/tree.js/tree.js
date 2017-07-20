// polyfill process.addAsyncListener for older nodes.
if (!process.addAsyncListener) require('async-listener');

var current = null;
var stack = [];
var leaves = [];
var nextId = 1;

process.addAsyncListener({
  before: onBefore,
  after: onAfter,
  error: onError
}, setup());

function guessName() {
  var stack = (new Error()).stack.split("\n");
  var seen = false;
  var line;
  for (var i = 1; i < stack.length; i++) {
    line = stack[i];
    var isLib = /node_modules\/async-listener/.test(line);
    if (!seen) {
      if (isLib) seen = true;
      continue;
    }
    if (isLib) continue;
    break;
  }
  var match = line.match(/at ([^ ]*).*\(([^)]*)\)/);
  return {
    name: match && match[1],
    file: match && match[2]
  };
}

function setup() {
  var info = guessName();
  info.parent = current;
  info.parentIndex = current ? current.children.length - 1 : 0;
  info.setup = Date.now();
  info.id = nextId++;
  info.children = [];
  if (current) {
    var index = leaves.indexOf(current);
    if (index >= 0) leaves.splice(index, 1);
  }
  leaves.push(info);
  return info;
}

function onBefore(context, storage) {
  if (current) stack.push(current);
  storage.children.push({ before: Date.now() });
  current = storage;
}

function onAfter(context, storage) {
  storage.children[storage.children.length - 1].after = Date.now();
  current = stack.pop();
}

function onError(storage, error) {
  if (!storage) return false;
  storage.error = error;
  return true;
}

if(true){
	var targetModule = process.argv[2];
	console.log(targetModule);
	require(targetModule);
}

process.on('exit', function () {
  var inspect = require('util').inspect;
  var nodes = leaves;
  leaves = [];
  var seen = [];
  console.error(inspect(nodes, {colors:true,depth:100}));
  console.log('digraph events {');
  console.log('graph [rankdir = "LR"];');

  nodes.forEach(function (node) {
    while (node) {
      if (seen.indexOf(node) >= 0) return;
      seen.push(node);
      var ch = node.children.map(function (child, i) {
        return '<c' + i + '>' + (child.before - node.setup) + 'ms - ' + (child.after - child.before) + 'ms';
      }).join("|");
      console.log('n' + node.id + ' [ shape=record label="<name>' + ("" + node.name).replace('<', "(").replace(">", ")") + '|' + node.file + '|' + ch + '" ];');
      var parent = node.parent;
      if (parent) {
        console.log('n' + parent.id + ':c' + node.parentIndex + ' -> n' + node.id + ':name');
      }
      node = parent;
    }
  });

  console.log('}');
});
