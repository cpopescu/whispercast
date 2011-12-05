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

// This JS is used in auto-forms only! Don't include it in any HTML pages!
// For new HTML pages, use: log.js, rpc_types.js, rpc_connection.js, rpc_json.js


function json_string(a) {
  var l = a.length;
  var s = '"';
  for (var i = 0; i < l; i += 1)  {
    var c = a.charAt(i);
    if (c >= ' ' && c <= '~') {
      if (c == '\\' || c == '"' || c == '/' ) {
        s += '\\';
      }
      s += c;
    } else {
      switch (c) {
        case '\b': s += '\\b'; break;
        case '\f': s += '\\f'; break;
        case '\n': s += '\\n'; break;
        case '\r': s += '\\r'; break;
        case '\t': s += '\\t'; break;
        default:   s += c;     break;
      }
    }
  }
  return s + '"';
}

function json_pretty_print(s) {
  r = "";
  indent = "";
  skip = false;
  instring = false;
  last_eol = false;
  skip_space = false;
  for ( i = 0; i < s.length; i++ ) {
    if ( instring ) {
      if ( skip == true ) {
        skip = false;
      } else if ( s.charAt(i) == "\"") {
        instring = false;
      } else if ( s.charAt(i) == "\\") {
        skip = true;
      }
      r += s.charAt(i);
      last_eol = false;
      skip_space = false;
    } else if ( s.charAt(i) == "}" || s.charAt(i) == "]") {
      indent = indent.substring(0, indent.length - 4);
      if ( !last_eol ) {
        r += "\n";
      } else {
        last_eol = false;
      }
      r += indent + s.charAt(i);
      skip_space = true;
    } else if ( s.charAt(i) == "{" || s.charAt(i) == "[" ) {
      indent = indent + "    ";
      r += s.charAt(i) + "\n" + indent;
      skip_space = true;
    } else if ( s.charAt(i) == "," ) {
      r += ",\n" + indent;
      last_eol = true;
      skip_space = true;
    } else {
      last_eol = ( s.charAt(i) == "\n")
      if ( s.charAt(i) == "\"") {
        instring = true;
      }
      last_eol = false;
      if ( !skip_space || s.charAt(i) != ' ' ) {
        r += s.charAt(i);
        skip_space = false;
      }
    }
  }
  return r;
}
function object_pretty_print(a, indent) {
  if ( a == undefined ) return "undefined";
  if ( a == null ) return "null";
  
  var str_indent = function(ind) {
    var s = "";
    for ( var i = 0; i < ind; i++ ) {
      s += "  ";
    }
    return s;
  };

  switch ( typeof(a) ) {
    case 'number' : return String(a);
    case 'boolean': return String(a);
    case 'string' : return String(a);
    case 'object' :
      if (InstanceOfArray(a)) {
        var text = "[\n";
        for (var i = 0; i < a.length; ++i) {
          text += str_indent(indent + 1) + object_pretty_print(a[i], indent + 1);
          if (i + 1 < a.length) {
            text += ", ";
          }
          text += "\n";
        }
        text += str_indent(indent) + "]";
        return text;
      }
      if (InstanceOfDate(a)) {
        return String(a.getTime());
      }
    default:
      var text = "{\n";
      for ( attr in a ) {
        text += str_indent(indent + 1) + attr + ": " + object_pretty_print(a[attr], indent + 1) + "\n";
      }
      text += str_indent(indent) + "}";
      return text;
  }
}
function json_pretty_print_new(s) {
  var x = eval("(" + s + ")");
  return object_pretty_print(x, 0);
}

function extract_form_data(f, first_element, last_element, read_one_element) {
  var in_struct = 0;
  var in_array  = 0;
  var in_map = 0;
  var last_was_begin = true;
  var ret = "";
  for ( var i = first_element; i < last_element; i++ ) {
    e = f.elements[i];
    t = e.type;
    n = e.name;
    v = e.value;
    if ( t == "hidden" ) {
      if ( v == "__begin_struct__" ) {
        if ( !last_was_begin )  ret += ', ';
        last_was_begin = true;
        if ( n == "" ) {
          ret += '{';
        } else {
          ret += '"' + n + '" : {';
        }
        in_struct ++;
      } else if ( v == "__end_struct__" ) {
        ret += '}';
        last_was_begin = false;
        in_struct --;
        if ( in_map == 0 &&
             in_array == 0 &&
             in_struct == 0 &&
             read_one_element ) {
          return [ret, i];
        }
      } else if ( v == "__begin_map__" ) {
        if ( !last_was_begin )  ret += ', ';
        last_was_begin = true;
        if ( n == "" ) {
          ret += '{';
        } else {
          ret += '"' + n + '" : {';
        }
        in_map ++;
      } else if ( v == "__begin_pair__" ) {
        if ( !last_was_begin )  ret += ', ';
        last_was_begin = true;
        // ret += ' {';
      } else if ( v == "__middle_pair__" ) {
        last_was_begin = true;
        ret += ' : ';
      } else if ( v == "__end_pair__" ) {
        last_was_begin = false;
        // ret += ' }';
      } else if ( v == "__end_map__" ) {
        ret += '}';
        last_was_begin = false;
        in_map --;
        if ( in_map == 0 &&
             in_array == 0 &&
             in_struct == 0 &&
             read_one_element ) {
          return [ret, i];
        }
      } else if ( v == "__begin_array__" ) {
        if ( !last_was_begin )  ret += ', ';
        last_was_begin = true;
        if ( n == "" ) {
          ret += '[';
        } else {
          ret += '"' + n + '" : [';
        }
        in_array ++;
      } else if ( v == "__end_array__" ) {
        ret += ']';
        last_was_begin = false;
        in_array --;
        if ( in_map == 0 &&
             in_array == 0 &&
             in_struct == 0 &&
             read_one_element ) {
          return [ret, i];
        }
      }
    } else if ( n == '__next_is_optional__' && t == 'checkbox' ) {
      if ( !e.checked ) {
        next_elem = extract_form_data(f, i + 1, last_element, true);
        i = next_elem[1];
      }
    } else if ( t == 'checkbox' || t == 'text' ) {
      if ( !last_was_begin )  ret += ', ';
      last_was_begin = false;
      if ( t == 'checkbox' ) {
        if ( e.checked ) {
          if ( n == "" ) {
            ret += ' true ';
          } else {
            ret += '"' + n + '" : true ';
          }
        } else {
          if ( n == "" ) {
            ret += ' false ';
          } else {
            ret += '"' + n + '" : false ';
          }
        }
      } else if ( t == 'textarea' || t == 'text' ) {
        new_name = n.substr(1);
        if ( n.charAt(0) == 'n' ) {
          if ( new_name == "" ) {
            ret += " " + String(Number(v)) + " ";
          } else {
            ret += '"' + new_name + '" : ' + String(Number(v)) + ' ';
          }
        } else {
          if ( new_name == "" ) {
            ret += " " + json_string(v) + " ";
          } else {
            ret += '"' + new_name + '" : ' + json_string(v) + ' ';
          }
        }
      }
      if ( in_map == 0 &&
           in_array == 0 &&
           in_struct == 0 &&
           read_one_element ) {
        return [ret, i];
      }
    }
  }
  return [ret, i];
}

function add_control_text(name, f, elem) {
  if ( elem == null ) {
    f.appendChild(document.createTextNode(name + ":"));
    f.appendChild(document.createElement("br"));
  } else {
    f.insertBefore(document.createTextNode(name, elem));
    f.insertBefore(document.createElement("br"), elem);
  }
}

function add_control_element(name, value, f, elem) {
  var textinput = document.createElement('input');
  textinput.setAttribute('type', 'hidden');
  textinput.setAttribute('name', name);
  textinput.setAttribute('value', value);
  if ( elem == null ) {
    f.appendChild(textinput);
  } else {
    f.insertBefore(textinput, elem);
  }
}

function add_begin_struct(name, f, elem) {
  if ( name != "" ) {
    add_control_text(name, f, elem);
  }
  add_control_element(name, '__begin_struct__', f, elem);
}
function add_end_struct(name, f, elem) {
  add_control_element(name, '__end_struct__', f, elem);
}
function add_begin_array(name, f, elem) {
  add_control_element(name, '__begin_array__', f, elem);
}
function add_end_array(name, f, elem) {
  add_control_element(name, '__end_array__', f, elem);
}
function add_begin_map(name, f, elem) {
  add_control_element(name, '__begin_map__', f, elem);
}
function add_end_map(name, f, elem) {
  add_control_element(name, '__end_map__', f, elem);
}
function add_begin_pair(name, f, elem) {
  add_control_element(name, '__begin_pair__', f, elem);
}
function add_middle_pair(name, f, elem) {
  add_control_element(name, '__middle_pair__', f, elem);
}
function add_end_pair(name, f, elem) {
  add_control_element(name, '__end_pair__', f, elem);
}

function add_optional_element(f, elem) {
  var textinput = document.createElement('input');
  textinput.setAttribute('type', 'checkbox');
  textinput.setAttribute('name', '__next_is_optional__');
  if ( elem == null ) {
    f.appendChild(textinput);
  } else {
    f.insertBefore(textinput, elem);
  }
}


function add_text_control(label, name, is_optional, f, elem) {
  if ( is_optional ) {
    add_optional_element(f, elem);
  }
  var textinput = document.createElement('input');
  textinput.setAttribute('type', 'text');
  textinput.setAttribute('name', 's' + name);
  if ( elem == null ) {
    if ( label != "" ) {
      f.appendChild(document.createTextNode(label + ": "));
    }
    f.appendChild(textinput);
    f.appendChild(document.createElement('br'));
  } else {
    if ( label != "" ) {
      f.insertBefore(document.createTextNode(label + ": "), elem);
    }
    f.insertBefore(textinput, elem);
    f.insertBefore(document.createElement('br'), elem);
  }
}

function add_boolean_control(label, name, is_optional, f, elem) {
  if ( is_optional ) {
    add_optional_element(f, elem);
  }
  var textinput = document.createElement('input');
  textinput.setAttribute('type', 'checkbox');
  textinput.setAttribute('name', name);
  if ( elem == null ) {
    if ( label != "" ) {
      f.appendChild(document.createTextNode(label + ": "));
    }
    f.appendChild(textinput);
    f.appendChild(document.createElement('br'));
  } else {
    if ( label != "" ) {
      f.insertBefore(document.createTextNode(label + ": "), elem);
    }
    f.insertBefore(textinput, elem);
    f.insertBefore(document.createElement('br'), elem);
  }
}

function add_number_control(label, name, is_optional, f, elem) {
  if ( is_optional ) {
    add_optional_element(f, elem);
  }
  var textinput = document.createElement('input');
  textinput.setAttribute('type', 'text');
  textinput.setAttribute('name', 'n' + name);
  if ( elem == null ) {
    if ( label != "" ) {
      f.appendChild(document.createTextNode(label + ": "));
    }
    f.appendChild(textinput);
    f.appendChild(document.createElement('br'));
  } else {
    if ( label != "" ) {
      f.insertBefore(document.createTextNode(label + ": "), elem);
    }
    f.insertBefore(textinput, elem);
    f.insertBefore(document.createElement('br'), elem);
  }
}

function get_per_type_action(label, elem_type, is_key) {
  if ( elem_type[0] == "__text" ) {
    return 'add_text_control(\'' + label + '\', \'\', false, parentNode, this)';
  } else if ( elem_type[0] == "__number" && is_key == false ) {
    return 'add_number_control(\'' + label +
        '\', \'\', false, parentNode, this)';
  } else if ( elem_type[0] == "__number" && is_key == true ) {
    return 'add_string_control(\'' + label +
        '\', \'\', false, parentNode, this)';
  } else if ( elem_type[0] == "__boolean" ) {
    return 'add_boolean_control(\'' + label +
        '\', \'\', false, parentNode, this)';
  } else if ( elem_type[0] == "__array" ) {
    return 'add_array_control(\'' + label +
        '\', \'\', false, parentNode, this, ' +
        JSONEncodeValue(elem_type[1]) + ')';
  } else if ( elem_type[0] == "__map" ) {
    return 'add_map_control(\'' + label +
        '\', \'\', false, parentNode, this, ' +
        JSONEncodeValue(elem_type[1]) + ', ' +
        JSONEncodeValue(elem_type[2]) + ' )';
  } else {
    return 'add_' + elem_type[0] + '_control(\'' +
        label + '\', \'\', false, parentNode, this)';
  }
}

function add_array_control(label, name, is_optional, f, elem, elem_type) {
  if ( is_optional ) {
    add_optional_element(f, elem);
  }

  var elem_add_button = document.createElement('input');
  elem_add_button.setAttribute('type', 'button');
  elem_add_button.setAttribute('value', 'Add ' + label);
  elem_add_button.setAttribute('name', 'set_value');
  elem_add_button.setAttribute('onclick',
                               get_per_type_action("elem" , elem_type, false));

  var ul = document.createElement('ul');
  add_begin_array(name, ul, null);
  if ( label != "" ) {
    ul.appendChild(document.createTextNode(label + ": "));
  }
  ul.appendChild(document.createElement('br'));
  ul.appendChild(elem_add_button);
  ul.appendChild(document.createElement('br'));
  add_end_array(label, ul, null);

  if ( elem == null ) {
    f.appendChild(ul);
  } else {
    f.insertBefore(ul, elem);
  }
}

function add_map_control(label, name, is_optional,
                         f, elem, elem_type1, elem_type2) {
  if ( is_optional ) {
    add_optional_element(f, elem);
  }
  var elem_add_button = document.createElement('input');
  elem_add_button.setAttribute('type', 'button');
  elem_add_button.setAttribute('value', 'Add ' + name);
  elem_add_button.setAttribute('name', 'set_value');
  elem_add_button.setAttribute(
      'onclick',
      "add_begin_pair('', parentNode, this); " +
      get_per_type_action('key', elem_type1, true) + "; " +
      "add_middle_pair('', parentNode, this); " +
      get_per_type_action('val', elem_type2, true) + "; " +
      "add_end_pair('', parentNode, this);");

  var ul = document.createElement('ul');

  add_begin_map(name, ul, null);
  if ( label != "" ) {
    ul.appendChild(document.createTextNode(label + ": "));
  }
  ul.appendChild(document.createElement('br'));
  ul.appendChild(elem_add_button);
  ul.appendChild(document.createElement('br'));
  add_end_map(name, ul, null);

  if ( elem == null ) {
    f.appendChild(ul);
  } else {
    f.insertBefore(ul, elem);
  }
}

function display_rpc_return(call_status, something) {
  if ( call_status.success == false ) {
    alert("RPC talking error :" + call_status.error);
    return;
  }
  LOG_INFO("Result: " + json_pretty_print(something));
}

LOG_LEVEL = 10;

function rpc_raw_query(url, service, method, strParams, handleResultCallback)
{
  LOG_INFO("Sending: " +
           "\n      service: [" + service + "]" +
           "\n      method: [" + method + "]" +
           "\n      params: [" + strParams + "]");
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
      new RPCCallStatus(false, "browser does not support XMLHttpRequest", null),
      null);
    return;
  }
  http_request.open( "POST", url, true );
  http_request.setRequestHeader( "Rpc_Codec_Id", RPC_CID_JSON.toString() );
  http_request.onreadystatechange = function()  {
    if(http_request.readyState != 4) {
      return;
    }
    if ( http_request.status != 200 ) {
      LOG_ERROR("Http Error: " + http_request.status);
      handleResultCallback(new RPCCallStatus(
                             false, "http failed, http_request.status=" +
                             http_request.status, null));
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
    if (error == "") {
      handleResultCallback(new RPCCallStatus(true,     "", null),
                           http_request.responseText);
    } else {
      handleResultCallback(new RPCCallStatus(false, error, null),
                           http_request.responseText);
    }
    delete http_request;
  };
  http_request.send(text);
}
