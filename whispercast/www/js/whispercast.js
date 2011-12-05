if (typeof(whispercast) == 'undefined') {
  whispercast = {};
}

/**
 * Creates a closure (a function that binds the optional arguments to it's
 * scope).
 *
 * @access public
 *
 * @parameter s:object
 *            The scope of the closure (the function will be called in the
 *            context of this object).
 * @parameter f:function
 *            The function to be called.
 *
 * @return function
 *            A function that, when called, will call the f function in the
 *            scope of s and will also bind the optional arguments to it's
 *            scope.
 **/
whispercast.closure = function(s, f) {
  return function() {
    f.apply(s, arguments);
  }
}

/**
 * Logs the given message.
 *
 * @access public
 *
 * @parameter m:string
 *            The message to be logged.
 *
 * @return void
 **/
whispercast.log = function(m) {
  if (typeof(console) != 'undefined')
    if (typeof(console.log) != 'undefined')
      console.log(m);
}

