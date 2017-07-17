var http = require('http');
var send = require('send');
var urlParse = require('url').parse;

var count = 2;
var server = http.createServer(function (req, res) {
  if (!--count) server.close(); // Only allow two connection and then exit.
  send(req, urlParse(req.url).pathname)
    .root(__dirname)
    .pipe(res);
});
server.listen(8080, function () {
  console.error("Server listening at http://localhost:8080/");
});
