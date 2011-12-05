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

class RpcCallStatus {
  public $success_; // boolean
  public $error_; // string
  public $result_array_; // an associative array, as it was decoded by JSON
  public $result_; // a specific object, created by RpcRebuildObject
  function __construct($success, $error, $result_array) {
    $this->success_ = $success;
    $this->error_ = $error;
    $this->result_array_ = $result_array;
    $this->result_ = NULL;
  }
  public function __toString() {
    return "success_=" . $this->success_ +
           ", error_=[" . $this->error_ . "]" +
           ", result_array_=" . $this->result_array_ +
           ", result_=" . $this->result_;
  }
}

abstract class RpcConnection {
  // map of calls
  public $pending_calls_;
  private $next_xid_;
  public function __construct() {
    $this->pending_calls_ = array();
    $this->next_xid_ = 0;
  }
  private function GenXID() {
    return $this->next_xid_++;
  }
  // Synchronous RPC. Returns an RpcCallStatus object.
  // What it does: construct rpc query, encode JSON, send, recv,
  //               decode JSON, construct RpcCallStatus, return it.
  public function Call($service, $method, $encoded_params_string) {
    $xid = $this->GenXID();

    $query = new RpcMessage();
    $query->header_->xid_ = $xid;
    $query->header_->msgType_ = RPC_CALL;
    $query->cbody_->service_ = $service;
    $query->cbody_->method_ = $method;
    $query->cbody_->params_ = $encoded_params_string;

    // SendMessage should be implemented by the derived class
    $response = $this->SendMessage($query);
    LOG_DEBUG("Got response: " . var_export($response, true));
    
    $error = "";
    if ( is_string($response) ) {
      $error = "SendMessage failed: " . $response;
    } else if($response->header_->xid_ != $xid) {
      $error = "Illegal xid: sent[" . $xid . "] != received[" . $response->header_->xid_ . "].";
    } else if($response->header_->msgType_ != RPC_REPLY) {
      $error = "Illegal Rpc message type: " . $response->header_->msgType_;
    } else if($response->rbody_->replyStatus_ != RPC_SUCCESS) {
      $strStatusName = RpcReplyStatusName($response->rbody_->replyStatus_);
      $error = "RPC error: " . $strStatusName;
    }
    if ( $error == "" ) {
      // success
      return new RpcCallStatus(true,      "", $response->rbody_->result_);
    } else {
      // failure
      LOG_ERROR($error);
      return new RpcCallStatus(false, $error, NULL);
    }
  }

  // Send the given RpcMessage.
  // On success it returns the response RpcMessage.
  // On failure it returns a string description of the error.
  abstract protected function SendMessage(RpcMessage $msg);
}

/*********************************************************************/
/*                 An RpcConnection using HTTP POST                  */
/*********************************************************************/

class RpcHttpConnection extends RpcConnection {

  // HTTP address of the Rpc server
  public $url_;

  function __construct($url) {
    parent::__construct();
    $this->url_ = $url;
  }

  public function SendMessage(RpcMessage $query) {
    $query_text = JsonEncodePacket($query);
    LOG_DEBUG("query '" . var_export($query, true) . "' encoded to: '" . $query_text . "'");
    /* // Using HttpRequest
    $http_request = new HttpRequest($this->url_, HttpRequest::METH_POST);
    $http_request->addHeaders(array("RPC_Codec_Id" => strval(RPC_CID_JSON)));
    $http_request->setRawPostData($query_text);

    try {
      $http_message = $http_request->send();
    } catch (Exception $e) {
      LOG_ERROR("Http Error: " . $e);
      return "HTTP Error: " . strval($e);
    }
    $response_text = $http_message->getBody();
    */
    
    // Using CURL
    $c = curl_init();
    curl_setopt($c, CURLOPT_URL, $this->url_);
    curl_setopt($c, CURLOPT_HTTPHEADER, array('RPC_Codec_Id: '.RPC_CID_JSON));
    curl_setopt($c, CURLOPT_POST, 1);
    curl_setopt($c, CURLOPT_POSTFIELDS, $query_text);
    curl_setopt($c, CURLOPT_RETURNTRANSFER, true);
    $response_text = curl_exec($c);
    if ($response_text === false) {
            $e = curl_error($c);
            LOG_ERROR("Http Error: " . $e);
            curl_close($c);
      return "HTTP Error: " . strval($e);
    }
    curl_close($c);
    
    $response = JsonDecodePacket($response_text);
    if ( is_null($response) ) {
      LOG_ERROR("JsonDecodePacket failed for input text: " . $response_text);
      return "Failed to decode Json response";
    }
    return $response;
  }
}

?>
