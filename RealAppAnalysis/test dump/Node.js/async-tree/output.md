```sh
$> node tree.js ./app.js | dot -Tsvg > events.svg && xdg-open events.svg
```

```sh
$> curl http://localhost:8080/app.js
... LOTS OF JS CODE ...
$> curl http://localhost:8080/favicon.ico
Not Found
```

![events](http://creationix.com/events2.svg)