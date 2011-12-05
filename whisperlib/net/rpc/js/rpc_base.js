// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

// This JS is compilation of:
//   log.js + rpc_types.js + rpc_connection.js + rpc_json.js
// This JS is used in auto-forms only! Don't include it in any HTML pages!
// For new HTML pages, use: log.js, rpc_types.js, rpc_connection.js, rpc_json.js

// Bridge XMLHTTP to XMLHttpRequest in pre-7.0 Internet Explorers
if (typeof XMLHttpRequest == "undefined")
  XMLHttpRequest = function() {
    try { return new ActiveXObject("Msxml2.XMLHTTP.6.0"); } catch(e) {};
    try { return new ActiveXObject("Msxml2.XMLHTTP.3.0"); } catch(e) {};
    try { return new ActiveXObject("Msxml2.XMLHTTP"); }     catch(e) {};
    try { return new ActiveXObject("Microsoft.XMLHTTP"); }  catch(e) {};
    return null;
  };


function DefaultLogger() {
  // Required functions:
  // - Log(string): to display a message
  // - Clear(): to clear the log display 
  this.Log = function(level, text) {
    var div = document.getElementById('LOGAREA');
    if (div) {
      div.innerHTML += '<pre style=\"color: ' + LCOLORS[level] + '\">' +
                       LPREFIX[level] + text + '</pre>';
    }
  }
  this.Clear = function() {
    var div = document.getElementById('LOGAREA');
    if (!div) {
      alert("LOGAREA not found");
      return;
    }
    div.innerHTML = "";
  }
}
// if you like a different logger view then create a new Logger class
var g_logger = new DefaultLogger();

var LERROR = 1;
var LWARNING = 2;
var LINFO = 3;
var LDEBUG = 4;
var LPREFIX = ["[X] ", "[E] ", "[W] ", "[I] ", "[D] "];
var LCOLORS = ["blue", "red" , "purple", "black", "silver"];
function LOG(level, text) {
  if(typeof(LOG_LEVEL) == 'undefined' || level > LOG_LEVEL)
    return;
  if(level < LERROR) level = 0;
  if(level > LDEBUG) level = LDEBUG;

  g_logger.Log(level, text);
}
function ClearLog() {
  g_logger.Clear();
}
function LOG_ERROR(text)   { LOG(LERROR, text); }
function LOG_WARNING(text) { LOG(LWARNING, text); }
function LOG_INFO(text)    { LOG(LINFO, text); }
function LOG_DEBUG(text)   { LOG(LDEBUG, text); }

function CHECK(cond, strErr) {
  if(cond) return;
  else {
    var err = (strErr != undefined) ? strErr : "Check failed";
    LOG_ERROR("Check failed: " + err);
    throw err;
  }
}
function CHECK_EQ(a, b) {
  if(a == b) return;
  else {
    var err = "CHECK_EQ failed: " + a + " == " + b;
    throw err;
  }
}

function InstanceOfArray(obj) {
  return typeof(obj) == 'object' && obj != null && obj != undefined
      && String(obj.constructor).substring(0, 30).match(/function\s+Array/)
      != null;
}
function InstanceOfDate(obj) {
  return typeof(obj) == 'object' && obj != null && obj != undefined
         && String(obj.constructor).substring(0, 30).match(/function\s+Date/)
      != null;
}

//
// RPC message types
//
var RPC_CALL = 0;
var RPC_REPLY = 1;

/////////////////////////////////////////////////////////
//
// RPC reply codes
//
var RPC_SUCCESS         = 0; /* RPC executed successfully.                  */
var RPC_SERVICE_UNAVAIL = 1; /* No such service.                            */
var RPC_PROC_UNAVAIL    = 2; /* No such procedure in the given service.     */
var RPC_GARBAGE_ARGS    = 3; /* Illegal number of params
                              * or wrong type for at least one param.       */
var RPC_SYSTEM_ERR      = 4; /* Unpredictable errors inside server, like
                              *   memory allocation failure.
                              * Errors in remote method calls should be handled
                              *   and error codes shoud be returned as valid
                              *   rpc-types. (e.g. a method can return
                              *   RPCUInt32 with value 0=success or (!0)=error
                              *   code)     */
var RPC_CONN_CLOSED     = 5; /* The transport layer was closed while
                              * waiting for reply. */
function RPCReplyStatusName(replyStatus) {
  switch(replyStatus) {
    case RPC_SUCCESS: return "success";
    case RPC_SERVICE_UNAVAIL: return "no such service";
    case RPC_PROC_UNAVAIL: return "no such method";
    case RPC_GARBAGE_ARGS: return "garbage arguments";
    case RPC_SYSTEM_ERROR: return "server internal error";
    case RPC_CONN_CLOSED: return "connection closed";
    default: return "unknown replyStatus: " + replyStatus;
  }
}
//
// Codec ID s
//
var RPC_CID_BINARY = 1;
var RPC_CID_JSON = 2;

function RPCHeader() {
  this.xid_ = 0;
  this.msgType_ = 0;
}
function RPCCallBody() {
  this.service_ = "";
  this.method_ = "";
  this.params_ = "{}";
}
function RPCReplyBody() {
  this.replyStatus_ = 0;
  this.result_ = undefined;
}
function RPCMessage() {
  this.header_ = new RPCHeader();
  this.cbody_ = new RPCCallBody();
  this.rbody_ = new RPCReplyBody();
}

function JSONEncodeValue(a) {
  // FACTS:
  // null == undefined
  // typeof(null) == object
  // typeof(undefined) == undefined

  if(typeof(a) == undefined) {
    LOG_ERROR("illegal parameter to JSONEncodeValue: " + a);
    throw "JSONEncodeValue cannot encode: undefined";
  }
  if (a == null) {
    return "null";
  }

  switch (typeof(a)) {
    case 'number' : return String(a);
    case 'boolean': return String(a);
    case 'string' :  {
      //[COSMIN] TODO test, remove. Try remove all escaping.
      //return '"' + a + '"';
      var l = a.length;
      var s = '"';
      for (var i = 0; i < l; i += 1) {
        var c = a.charAt(i);
        if (c >= ' ' && c <= '~') {
          if (c == '\\' || c == '"') {
            s += '\\';
          }
          s += c;
        } else {
          switch (c) {
            case '\b':
              s += '\\b';
              break;
            case '\f':
              s += '\\f';
              break;
            case '\n':
              s += '\\n';
              break;
            case '\r':
              s += '\\r';
              break;
            case '\t':
              s += '\\t';
              break;
            default:
              // use \uxxxx encoding
              c = c.charCodeAt();
              s += '\\u' + ('0000' + Math.floor(c / 16).toString(16) +
                            (c % 16).toString(16)).slice(-4);
          }
          }
      }
      return s + '"';
    }
    case 'object' :
      if (InstanceOfArray(a)) {
        var text = "[";
        for (var i = 0; i < a.length; ++i) {
          text += JSONEncodeValue(a[i]);
          if (i + 1 < a.length) {
            text += ", ";
          }
        }
        text += "]";
        return text;
      }
      if (InstanceOfDate(a)) {
      	return String(a.getTime());
      }
      // fallthrough
    default:
      if(a.toString != undefined &&
         String(a.toString) != String(Object.toString))
        return a.toString(); // custom type object

      // probably a map
      {
        var text = "";
        for (var attr in a) {
          if(typeof(a[attr]) == 'function') { continue; }
          if(text != "") { text += ", "; }
          text += "\"" + attr + "\" : " + JSONEncodeValue(a[attr]);
        }
        return "{" + text + "}";
      }
  };
}

function JSONEncodePacket(msg) {
  if( !(msg instanceof RPCMessage) ) {
    throw "TypeError: JSONSerialize can only serialize RPCMessage objects";
  }
  text = "{";
  header = msg.header_;
  text += "\"header\" : { " +
          "\"xid\" : " + header.xid_ +
          " , " +
          "\"msgType\" : " + header.msgType_ +
          "}";
  text += " , ";
  if(header.msgType_ == RPC_CALL) {
    cbody = msg.cbody_;
    text += "\"cbody\" : { " +
            "\"service\" : \"" + cbody.service_ + "\"" +
            " , " +
            "\"method\" : \"" + cbody.method_ + "\"" +
            " , " +
            "\"params\" : " + cbody.params_ +
            "} ";
  } else if (header.msgType_ == RPC_REPLY) {
    rbody = msg.rbody_;
    text += "\"rbody\" : { " +
            "\"replyStatus\" : " + rbody.replyStatus_ +
            " , " +
            "\"result\" : " + rbody.result_ +
            "} ";
  } else {
    throw "Bad RPCMessage.header_.msgType_ = " + header.msgType_;
  }
  text += "}";

  return text;
}

function JSONDecodePacket(str) {
  LOG_DEBUG("JSONDecodePacket reading RPCMessage from: " + str);
  try {
    var x = eval("(" + str + ")");
  } catch(err) {
    LOG_ERROR("eval error: " + err);
    return null;
  }
  var msg = new RPCMessage();
  msg.header_.xid_ = x.header.xid;
  msg.header_.msgType_ = x.header.msgType;
  if(msg.header_.msgType_ == RPC_REPLY) {
    msg.rbody_.replyStatus_ = x.rbody.replyStatus;
    msg.rbody_.result_ = x.rbody.result;
  } else {
    LOG_ERROR("Bad msgType: " + msg.header_.msgType_);
  }
  return msg;
}

function RPCCallStatus(success, error, userData) {
  this.success_ = success;
  this.error_ = error;
  this.userData_ = userData;
  this.toString = function() {
    return "success=" + this.success_ + " error=" +
    (this.error_=="" ? "none" : this.error_) + " userData=" + this.userData_;
  }
}

function RPCQuery(url, service, method, strParams,
                  handleResultCallback, userData)  {
  var msg = new RPCMessage();
  msg.header_.xid_ = 1
  msg.header_.msgType_ = RPC_CALL;
  msg.cbody_.service_ = service;
  msg.cbody_.method_ = method;
  msg.cbody_.params_ = strParams;

  text = JSONEncodePacket(msg);

  var http_request = new XMLHttpRequest();
  if (http_request == null) {
    handleResultCallback(
        new RPCCallStatus(false, "browser does not support XMLHttpRequest",
                          userData), null);
    return;
  }
  http_request.open( "POST", url, true );
  http_request.setRequestHeader( "Rpc_Codec_Id", RPC_CID_JSON.toString() )
  http_request.onreadystatechange = function()  {
    if (http_request.readyState != 4) {
      return;
    }
    if ( http_request.status != 200 ) {
      LOG_ERROR("Http Error: " + http_request.status);
      handleResultCallback(
          new RPCCallStatus(false, "http failed, http_request.status=" +
                            http_request.status, userData));
      return;
    }
    var msg = JSONDecodePacket(http_request.responseText);
    var error = "";
    if(msg == null) {
      error = "Failed to decode server reply";
    } else if(msg.header_.msgType_ != RPC_REPLY) {
      LOG_ERROR("Bad msgType: " + msg.header_.msgType_);
      error = "Illegal RPC message from server";
    } else if(msg.rbody_.replyStatus_ != RPC_SUCCESS) {
      var strStatusName = RPCReplyStatusName(msg.rbody_.replyStatus_);
      LOG_ERROR("call failed: " + strStatusName);
      error = "server error: " + strStatusName;
    }
    if (error == "")
      handleResultCallback(new RPCCallStatus(true,     "", userData),
                           msg.rbody_.result_);
    else
      handleResultCallback(new RPCCallStatus(false, error, userData),
                           msg.rbody_.result_);
    delete http_request;
  };

  http_request.send(text);
}
