<?php

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
// Author: Cosmin Tudorache

include_once "rpc_log.php";
include_once "rpc_common.php";

function JsonEncodeValue($a) {
  if ( is_null($a) ) {
    return "null";
  }

  if ( is_scalar($a) ) {
    // Scalar types: integer, float, string or boolean. Types array, object and resource are not scalar.
    // NOTE: string needs to be escaped!!! Check if json_encode does string escape.
    return json_encode($a);
  }

  if ( IsVector($a) ) {
    //vector
    $text="";
    foreach($a as $v) {
      if ( $text !== "" ) {
        $text .= ", ";
      }
      $text .= JsonEncodeValue($v);
    }
    return "[" . $text . "]";
  }

  if ( IsDateTime($a) ) {
    // date
    return strval(intval($a->format("U")) * 1000);
  }

  if ( method_exists($a, "__toString" ) ) {
    // rpc custom type object
    return $a->__toString();
  }

  //map or object
  $text="";
  foreach($a as $k => $v) {
    if ( $text !== "" ) {
      $text .= ", ";
    }
    $text .= JsonEncodeValue($k) . " : " . JsonEncodeValue($v);
  }
  return "{" . $text . "}";
}

function JsonEncodePacket($msg) {
  if ( !is_object($msg) || 
       get_class($msg) !== "RpcMessage" ) {
    throw new Exception("TypeError: JsonEncodePacket can only serialize RpcMessage objects, provided: [" . gettype($msg) . "]");
  }
  $text = "{";
  $header = &$msg->header_;
  $text .= "\"header\" : { " .
                          "\"xid\" : " . $header->xid_ .
                          " , " .
                          "\"msgType\" : " . $header->msgType_ .
                        "}";
  $text .= " , ";
  if ( $header->msgType_ === RPC_CALL ) {
    $cbody = &$msg->cbody_;
    $text .= "\"cbody\" : { " .
                            "\"service\" : \"" . $cbody->service_ . "\"" .
                            " , " .
                            "\"method\" : \"" . $cbody->method_ . "\"" .
                            " , " .
                            "\"params\" : " . $cbody->params_ .
                         "} ";
  } else if ($header->msgType_ === RPC_REPLY) {
    $rbody = &$msg->rbody_;
    $text .= "\"rbody\" : { " .
                            "\"replyStatus\" : " . $rbody->replyStatus_ .
                            " , " .
                            "\"result\" : " . $rbody->result_ .
                         "} ";
  } else {
    throw new Exception("Bad RpcMessage.header_.msgType_ = " . $header->msgType_);
  }
  $text .= "}";

  return $text;
}

function JsonDecodePacket($str)
{
  LOG_DEBUG("JsonDecodePacket reading RpcMessage from: " . $str);
  try {
    // json_decode returns: an associative array on success,
    //                      or NULL on failure.
    $x = json_decode($str);
    if ( is_null($x) ) {
      LOG_ERROR("json_decode failed for input string: [" . $str . "]");
      return NULL;
    }
    if ( !is_object($x) ||
         !isset($x->header) ||
         !isset($x->cbody) &&
         !isset($x->rbody) ) {
      LOG_ERROR("invalid server message. not an RPC packet: '" . $str . "' decoded as: " . var_export($x, true));
      return NULL;
    }
  } catch(Exception $e) {
    LOG_ERROR("json_decode error: " . $e);
    return NULL;
  }
  $msg = new RpcMessage();
  $msg->header_->xid_ = $x->header->xid;
  $msg->header_->msgType_ = $x->header->msgType;
  if($msg->header_->msgType_ === RPC_REPLY) {
    $msg->rbody_->replyStatus_ = $x->rbody->replyStatus;
    $msg->rbody_->result_ = $x->rbody->result;
  } else {
    LOG_ERROR("Illegal msgType: " . $msg->header_->msgType_);
    return NULL;
  }
  return $msg;
}

// Tries to parse a Json encoded structure from a given string.
// Returns the structure size(in bytes), or 0 if data contains an
// incomplete structure, or -1 if data is not a Json structure.
//e.g.:
// if str = "{'a':'3', 'b':'7'}{'c" returns 18
// if str = "{'a':'3', 'b':" returns 0
// if str = "kjh23@lj2rfO@#$2" returns -1
/*
function JsonParseStruct(str)
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
  LOG_DEBUG("JsonParseStruct insufficient data, state: " . state);
  return 0;
}
*/

?>
