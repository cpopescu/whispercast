if (typeof(whispercast) == 'undefined') {
  whispercast  = {};
}
whispercast.player = whispercast.player ? whispercast.player : {};

/**
 * Constructor for the whispercast Flash media player.
 *
 * @access public
 *
 * @parameter asset_url_base:string
 *            The location of the SWF assets used by the player.
 * @parameter id:string
 *            The ID of the generated player.
 * @parameter width:number
 *            The width of the generated player.
 * @parameter height:number
 *            The height of the generated player.
 * @parameter background-color:string
 *            The background color of the generated player.
 *
 * @return void
 **/
whispercast.player.Player = whispercast.player.Player ? whispercast.player.Player :
function(asset_url_base, id,  width, height, background_color, vars, swf) {
  this.asset_url_base = asset_url_base;

  this.id = id;
  this.player = null;

  this.width = width;
  this.height = height;

  this.background_color = background_color;

  this.parent = null;
  this.original = null;

  this.vars = vars;

  this.swf = (swf == undefined) ? 'player.swf' : swf;
}

/**
 * Creates the element that represents the whispercast Flash media player.
 *
 * @access public
 *
 * @return void
 **/
whispercast.player.Player.prototype.create = function() {
  var vars = {
  };
  for (var name in this.vars) {
    vars[name] = escape(this.vars[name]);
  }

  this.parent = document.getElementById(this.id).parentNode;
  if (this.original == null)
    this.original = this.parent.innerHTML;

  swfobject.embedSWF(
    this.asset_url_base+'/'+this.swf,
    this.id,
    this.width,
    this.height,
    '10',
    this.asset_url_base+'expressinstall.swf',
    vars,
    {
      quality: 'high',
      play: 'true',
      loop: 'true',
      scale: 'noscale',
      wmode: 'window',
      devicefont: 'false',
      menu: 'false',
      allowfullscreen: 'true',
      allowscriptaccess: 'always',
      salign: 'lt',
      bgcolor: (this.background_color ? this.background_color : '')
    },
    {
      id:this.id+'_applet'
    }
  );
}
/**
 * Destroys the element that represents the whispercast Flash media player.
 *
 * @access public
 *
 * @return void
 **/
whispercast.player.Player.prototype.destroy = function() {
  var player = this.getPlayer();
  this.player = null;

  if (player) {
    try {
      player.setURL_(0, null, null);
    } catch(e) {
    }

    try
    {
    // Break circular references.
    (function (o) {
    var a = o.attributes, i, l, n, c;
    if (a) {
    l = a.length;
    for (i = 0; i <l; i += 1) {
    n = a[i].name;
    if (typeof o[n] === 'function') {
    o[n] = null;
    }
    }
    }
    a = o.childNodes;
    if (a) {
    l = a.length;
    for (i = 0; i <l; i += 1) {
    c = o.childNodes[i];
    // Purge child nodes.
    arguments.callee(c);
    }
    }
    }
    )(player);
    }
    catch(e)
    {
    }
    player.parentNode.removeChild(player);
  }

  this.parent.innerHTML = this.original;
  this.parent = null;
}

/**
 * Returns the element that represents the whispercast Flash media player.
 *
 * @access public
 *
 * @return object
 *         The actual HTML element that is the whispercast Flash media player.
 **/
whispercast.player.Player.prototype.getPlayer = function() {
  if (this.player == null) {
    this.player = document.getElementById(this.id+'_applet');
  }
  return this.player;
}
