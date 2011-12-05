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


//
// Rpc message types
//
define("RPC_CALL", 0);
define("RPC_REPLY", 1);

//
// Rpc reply codes
//
define("RPC_SUCCESS", 0);
define("RPC_SERVICE_UNAVAIL", 1);
define("RPC_PROC_UNAVAIL", 2);
define("RPC_GARBAGE_ARGS", 3);
define("RPC_SYSTEM_ERR", 4);
define("RPC_SERVER_BUSY", 5);
// client side only error code
define("RPC_CONN_CLOSED", 7);

function RpcReplyStatusName($reply_status) {
  switch($reply_status) {
    case RPC_SUCCESS: return "RPC_SUCCESS";
    case RPC_SERVICE_UNAVAIL: return "RPC_SERVICE_UNAVAIL";
    case RPC_PROC_UNAVAIL: return "RPC_PROC_UNAVAIL";
    case RPC_GARBAGE_ARGS: return "RPC_GARBAGE_ARGS";
    case RPC_SYSTEM_ERROR: return "RPC_SYSTEM_ERROR";
    case RPC_SERVER_BUSY: return "RPC_SERVER_BUSY";
    case RPC_CONN_CLOSED: return "RPC_CONN_CLOSED";
    default: return "Unknown reply_status: " + $reply_status;
  }
}

// Codec ID s
//
define("RPC_CID_BINARY", 1);
define("RPC_CID_JSON", 2);

/////////////////////////////////////////////////////////

class RpcHeader {
  public $xid_ = 0;
  public $msgType_ = 0;
}
class RpcCallBody {
  public $service_ = "";
  public $method_ = "";
  public $params_ = array();
}
class RpcReplyBody {
  public $replyStatus_ = 0;
  public $result_ = NULL;
}
class RpcMessage {
  public $header_ = NULL;
  public $cbody_ = NULL;
  public $rbody_ = NULL;
  function __construct() {
    $this->header_ = new RpcHeader();
    $this->cbody_ = new RpcCallBody();
    $this->rbody_ = new RpcReplyBody();
  }
}

function RpcRebuildObject($arrStrTypes, $obj) {
  if ( !IsVector($arrStrTypes) ) {
    throw new Exception('Invalid arrStrTypes: not an array, provided: ' . var_export($arrStrTypes, true));
  }
  if ( count($arrStrTypes) === 0 ) {
    throw new Exception('Invalid arrStrTypes: empty array');
  }
  switch ( $arrStrTypes[0] ) {
    case 'null':
      if(!is_null($obj))
        throw new Exception('Expected \"null\", got: ' . var_export($obj, true));
      return $obj;
    case 'string':
      if(!is_string($obj))
        throw new Exception('Expected \"string\", got type: ' . var_export($obj, true));
      return $obj;
    case 'integer':
      if(!is_integer($obj) && !is_float($obj))
        throw new Exception('Expected \"integer\" or \"float\", got type: ' . var_export($obj, true));
      return $obj;
    case 'float':
      if(!is_float($obj))
        throw new Exception('Expected \"float\", got type: ' . var_export($obj, true));
      return $obj;
    case 'boolean':
      if(!is_bool($obj))
        throw new Exception('Expected \"boolean\", got type: ' . var_export($obj, true));
      return $obj;
    case 'date':
      if(!is_int($obj) && !is_float($obj))
        throw new Exception('Expected \"int\" or \"float\", got type: ' . var_export($obj, true));
      return $obj;
    case 'array':
      if(!IsVector($obj))
        throw new Exception('Expected \"array\", got type: ' . var_export($obj, true));
      $p = array();
      // duplicate and shift
      $remainingArrStrTypes = $arrStrTypes;
      array_shift($remainingArrStrTypes);
      foreach($obj as $attr => $value) {
        // $attr is the index
        $p[$attr] =  RpcRebuildObject($remainingArrStrTypes, $value);
      }
      return $p;
    case 'map':
      if(!IsMap($obj))
        throw new Exception('Expected \"array\", got type: ' . var_export($obj, true));
      $p = array();
      // duplicate and shift
      $remainingArrStrTypes = $arrStrTypes;
      array_shift($remainingArrStrTypes);
      foreach($obj as $attr => $value) {
        $p[$attr] =  RpcRebuildObject($remainingArrStrTypes, $value);
      }
      return $p;
    default:
      // probably a custom type object. Use reflection to create a custom class instance.
      $p = new $arrStrTypes[0];
      $p->LoadFromObject($obj);
      return $p;
  };
}

?>