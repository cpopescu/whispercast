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

function InstanceOfArray(obj) {
  return typeof(obj) == 'object' && obj != null && obj != undefined
         && String(obj.constructor).substring(0, 30).match(/function\s+Array/)
      != null;
}
function InstanceOfDate(obj)
{
  return typeof(obj) == 'object' && obj != null && obj != undefined
         && String(obj.constructor).substring(0, 30).match(/function\s+Date/)
      != null;
}

function JSONEncodeValue(a)
{
  // FACTS:
  // null == undefined
  // typeof(null) == object
  // typeof(undefined) == undefined

  if(typeof(a) == undefined)
  {
    LOG_ERROR("illegal parameter to JSONEncodeValue: " + a);
    throw "JSONEncodeValue cannot encode: undefined";
  }
  if (a == null)
  {
    return "null";
  }

  switch(typeof(a))
  {
    case 'number' : return String(a);
    case 'boolean': return String(a);
    case 'string' :
      {
        //[COSMIN] TODO test, remove. Try remove all escaping.
        //return '"' + a + '"';
      	var l = a.length;
        var s = '"';
        for (var i = 0; i < l; i += 1)
        {
          var c = a.charAt(i);
          if (c >= ' ' && c <= '~')
          {
            if (c == '\\' || c == '"')
            {
              s += '\\';
            }
            s += c;
          }
          else
          {
            switch (c)
            {
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
      if (InstanceOfArray(a))
      {
        var text = "[";
        for (var i = 0; i < a.length; ++i)
        {
          text += JSONEncodeValue(a[i]);
          if(i + 1 < a.length)
          {
            text += ", ";
          }
        }
        text += "]";
        return text;
      }
      if (InstanceOfDate(a))
      {
      	return String(a.getTime());
      }
      // fallthrough
    default:
      if(a.toString != undefined && String(a.toString) != String(Object.toString))
        return a.toString(); // custom type object

      // probably a map
    	{
        var text = "";
        for (var attr in a)
        {
          if(typeof(a[attr]) == 'function') { continue; }
          if(text != "") { text += ", "; }
          text += "\"" + attr + "\" : " + JSONEncodeValue(a[attr]);
        }
        return "{" + text + "}";
      }
  };
}

function JSONEncodePacket(msg)
{
  if( ! (msg instanceof RPCMessage) )
  {
    throw "TypeError: JSONSerialize can only serialize RPCMessage objects";
  }
  text = "{";
  header = msg.header_;
  text += "\"header\" : { " +
                         "\"xid\" : " + header.xid_ +
                         " , " +
                         "\"msgType\" : " + header.msgType_ +
                      "}";
  text += " , "
  if(header.msgType_ == RPC_CALL)
  {
    cbody = msg.cbody_;
    text += "\"cbody\" : { " +
                           "\"service\" : \"" + cbody.service_ + "\"" +
                           " , " +
                           "\"method\" : \"" + cbody.method_ + "\"" +
                           " , " +
                           "\"params\" : " + cbody.params_ +
                        "} ";
  }
  else if (header.msgType_ == RPC_REPLY)
  {
    rbody = msg.rbody_;
    text += "\"rbody\" : { " +
                           "\"replyStatus\" : " + rbody.replyStatus_ +
                           " , " +
                           "\"result\" : " + rbody.result_ +
                        "} ";
  }
  else
  {
    throw "Bad RPCMessage.header_.msgType_ = " + header.msgType_;
  }
  text += "}"

  return text;
}

function JSONDecodePacket(str)
{
  LOG_DEBUG("JSONDecodePacket reading RPCMessage from: " + str);
  try
  {
    var x = eval("(" + str + ")");
  }
  catch(err)
  {
    LOG_ERROR("eval error: " + err);
    return null;
  }
  var msg = new RPCMessage();
  msg.header_.xid_ = x.header.xid;
  msg.header_.msgType_ = x.header.msgType;
  if(msg.header_.msgType_ == RPC_REPLY)
  {
    msg.rbody_.replyStatus_ = x.rbody.replyStatus;
    msg.rbody_.result_ = x.rbody.result;
  }
  else
  {
    LOG_ERROR("Bad msgType: " + msg.header_.msgType_);
  }
  return msg;
}

// Tries to parse a JSON encoded structure from a given string.
// Returns the structure size(in bytes), or 0 if data contains an
// incomplete structure, or -1 if data is not a JSON structure.
//e.g.:
// if str = "{'a':'3', 'b':'7'}{'c" returns 18
// if str = "{'a':'3', 'b':" returns 0
// if str = "kjh23@lj2rfO@#$2" returns -1
function JSONParseStruct(str)
{
  if(str.length == 0) { return 0; }
  if(str.charAt(0) != '{') { return -1; }

  var CState = function(c, index) {
    this.c_ = c;
    this.index_ = index;
  }

  // we begin with a struct
  var state = 1;
  var in_string = false;
  var in_string_esc = false;

  for(var i = 1; i < str.length; ++i) {
    var c = str.charAt(i);
    if(in_string) {
      if(in_string_esc) {
        in_string_esc = false;
        continue;
      }
      if(c == '\\') {
        in_string_esc = true;
        continue;
      }
      if(c == '"') {
        in_string = false;
        continue;
      }
      continue;
    }
    switch(c) {
      case '{':
      case '[':
        state++;
        break;
      case '}':
      case ']':
        state--
        break;
      case '"':
        in_string = true;
        break;
    }
    if(state == 0) {
      // success
      return i + 1;
    }
  }
  LOG_DEBUG("JSONParseStruct insufficient data, state: " + state);
  return 0;
}
