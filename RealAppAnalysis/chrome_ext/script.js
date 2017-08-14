(function() {
  var callbackFrames = [];
  var has = Object.prototype.hasOwnProperty;
  var id_counter = 0;
  var current_id = null;

  function CallbackFrame(parent_id, api_name, callback_id, callback_name, start_time, end_time){
    self.parent_id = parent_id;
    self.api_name = api_name;
    self.callback_id = callback_id;
    self.callback_name = callback_name;
    self.start_time = start_time;
    self.end_time = end_time;
  }
  // Takes an object, a property name for the callback function to wrap, and an argument position
  // and overwrites the function with a wrapper that captures the stack at the time of callback registration
  function wrapRegistrationFunction(object, property, callbackArg) {

      if (typeof object[property] !== "function") {
          console.error("(long-stack-traces) Object", object, "does not contain function", property);
          return;
      }
      if (!has.call(object, property)) {
          console.warn("(long-stack-traces) Object", object, "does not directly contain function", property);
      }
      console.log("function wrapped: " + property);

      var fn = object[property];
      object[property] = function() {
          id_counter++;
          arguments[callbackArg] = makeWrappedCallback(object, property, arguments[callbackArg], id_counter, current_id);
          return fn.apply(this, arguments);
      }
  }

  // Takes a callback function and name, and captures a stack trace, returning a new callback that restores the stack frame
  // This function adds a single function call overhead during callback registration vs. inlining it in wrapRegistationFunction
  function makeWrappedCallback(object, api_name, callback, self_id, parent_id) {
    var pre_ts = null;


    try {
      return function() {
        current_id = self_id;
        pre_ts = Date.now();
        //console.log("PRE (" + self_id + "): "+ pre_ts);
        var rv = callback.apply(this, arguments);
        return rv;
      }
    }
    catch(e){ console.error(e.stack); }
    finally {
      var post_ts = Date.now();

      //console.log("AFTER (" + self_id + "): "+ post_ts);

      callbackFrames.push( new CallbackFrame(parent_id, api_name, self_id, callback.name, pre_ts, post_ts) );
      console.log(parent_id + "->" + self_id);
      //console.log( "parent: " + parent_id + " (API: \"" + api_name + "\")" + ", self: " + self_id + " (callback function name: \"" + callback.name + "\")" + (post_ts - pre_ts) );
      current_id = null;
    }
  }

  if (typeof window !== "undefined") {
      wrapRegistrationFunction(window, "setTimeout", 0);
      wrapRegistrationFunction(window, "setInterval", 0);

      //experimental
      wrapRegistrationFunction(navigator.geolocation, "getCurrentPosition", 0);
      wrapRegistrationFunction(Array.prototype, "forEach", 0);

      wrapRegistrationFunction(window.Node.prototype, "addEventListener", 1);
      wrapRegistrationFunction(window.constructor.prototype, "addEventListener", 1);


      var objs = [
          //window.Node.prototype,
          window.MessagePort.prototype,
          window.SVGElementInstance.prototype, //complaining for some reason
          window.WebSocket.prototype,
          window.XMLHttpRequest.prototype,
          window.EventSource.prototype,
          window.XMLHttpRequestUpload.prototype,
          window.SharedWorker.prototype.__proto__,
          //window.constructor.prototype,
          window.applicationCache.constructor.prototype
      ];

      //all of these objects recieve events and may have listeners for them
      objs.forEach(function(object) {
          wrapRegistrationFunction(object, "addEventListener", 1);
      });

      // this actually captures the stack when "send" is called, which isn't ideal,
      // but it's the best we can do without hooking onreadystatechange assignments
      var _send = XMLHttpRequest.prototype.send;
      XMLHttpRequest.prototype.send = function() {
          this.onreadystatechange = makeWrappedCallback(this.onreadystatechange, "onreadystatechange");
          return _send.apply(this, arguments);
      }
  }

} )(typeof exports !== "undefined" ? exports : {});

/*
"content_scripts": [
  {
    "js": ["contentscript.js"],
    "matches": ["<all_urls>"]

  }
],
*/
