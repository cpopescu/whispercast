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

function RPCCallStatus(success, error, userData) {
  this.success_ = success;
  this.error_ = error;
  this.userData_ = userData;
  this.toString = function() {
    return "success=" + this.success_ + " error=" +
    (this.error_=="" ? "none" : this.error_) + " userData=" + this.userData_;
  }
}

function RPCConnection() {
  this.pending_calls_ = {};
  this.GenXID = function()
  {
    if(this.next_xid_ == undefined) {
      this.next_xid_ = 0;
    }
    return this.next_xid_++;
  }
  this.Call = function(service, method, str_params, result_handler, user_data)
  {
    var xid = this.GenXID();

    var msg = new RPCMessage();
    msg.header_.xid_ = xid;
    msg.header_.msgType_ = RPC_CALL;
    msg.cbody_.service_ = service;
    msg.cbody_.method_ = method;
    msg.cbody_.params_ = str_params;

    this.pending_calls_[xid] = {'handler_' : result_handler,
                                'user_data_' : user_data};

    // SendMessage should be implemented by the derived class
    this.SendMessage(msg);
  };
  // HandleMessage should be called by the derived class, with
  // every RPCMessage received.
  this.HandleMessage = function(msg)
  {
    var xid = msg.header_.xid_;
    var call = this.pending_calls_[xid];
    delete this.pending_calls_[xid];
    var result_handler = call.handler_;
    var user_data = call.user_data_;

    var error = "";
    if(msg.header_.msgType_ != RPC_REPLY)
    {
      LOG_ERROR("Bad msgType: " + msg.header_.msgType_);
      error = "Illegal RPC message from server";
    }
    else if(msg.rbody_.replyStatus_ != RPC_SUCCESS)
    {
      var strStatusName = RPCReplyStatusName(msg.rbody_.replyStatus_);
      LOG_ERROR("call failed: " + strStatusName);
      error = "server error: " + strStatusName;
    }
    if (error == "")
      result_handler(new RPCCallStatus(true,     "", user_data),
                     msg.rbody_.result_);
    else
      result_handler(new RPCCallStatus(false, error, user_data),
                     msg.rbody_.result_);
  };
  this.HandleError = function(error_description, xid)
  {
    if(xid != undefined)
    {
      var call = this.pending_calls_[xid];
      delete this.pending_calls_[xid];
      var result_handler = call.handler_;
      var user_data = call.user_data_;
      result_handler(new RPCCallStatus(false, error_description, user_data));
      return;
    }
    for(var i in this.pending_calls_)
    {
      var call = this.pending_calls_[i];
      var result_handler = call.handler_;
      var user_data = call.user_data_;
      result_handler(new RPCCallStatus(false, error_description, user_data));
    }
    this.pending_calls_ = {};
    return;
  };
}


/*********************************************************************/
/*              An RPCConnection using TCP through flash             */
/*********************************************************************/

// tcp_socket_swf: the flash movie exporting TcpConnection*** methods
// host: RPC server IP or domain name, as string
// port: RPC server port, as number
function RPCTcpConnection(tcp_socket_swf, host, port) {
  // inherit RPCConnection
  var rpc_connection = new RPCConnection();
  for(x in rpc_connection) {
    this[x] = rpc_connection[x];
    LOG_DEBUG("inherit copy: " + x);
  }

  this.swf_object_ = tcp_socket_swf;
  this.remote_host_ = host;
  this.remote_port_ = port;

  this.swf_conn_id_ = null;

  // bytes arriving from the network will be stacked here
  // until composing a full RPCMessage
  this.inbuf_ = "";
  // bytes to be sent to the network. If the connection is opening,
  // but not opened, outgoing bytes are stacked here.
  this.outbuf_ = "";

  // 0 - nothing happened
  // 1 - client hand sent, awaiting server response
  // 2 - server response received and reply sent. Handshake completed, we're OK.
  this.rpc_handshake_state_ = 0;
  this.rpc_mark_ = "rpc";
  this.rpc_vers_hi_ = 1;
  this.rpc_vers_lo_ = 0;
  this.rpc_codec_id_ = 2; // 1=binary, 2=json
  this.rpc_client_hand_ = "12345678901234567890123456789012"; // 32 bytes
  this.rpc_server_hand_ = "";

  this.Open = function()
  {
    if(this.IsOpen() || this.IsOpening()) {
      return;
    };
    this.swf_conn_id_ = GlobalTcpConnectionOpen(
        this.swf_object_, this, this.remote_host_, this.remote_port_);
    if(this.swf_conn_id_ == null)
    {
      LOG_ERROR("Failed to allocate TcpConnection.");
      return;
    }
    this.rpc_handshake_state_ = 0;
  }
  this.IsOpen = function()
  {
    return this.swf_conn_id_ != null && this.rpc_handshake_state_ == 2;
  }
  this.IsOpening = function()
  {
    return this.swf_conn_id_ != null && this.rpc_handshake_state_ != 2;
  }
  this.SendMessage = function(rpc_msg)
  {
    var data = JSONEncodePacket(rpc_msg);
    if(this.IsOpen())
    {
      GlobalTcpConnectionSend(this.swf_object_, this.swf_conn_id_, data);
    }
    else
    {
      this.outbuf_ += data;
      if(!this.IsOpening())
      {
        this.Open();
      }
    }
  }

  this.CloseAndClear = function(error_description)
  {
    LOG_ERROR("CloseAndClear error=" + error_description);
    GlobalTcpConnectionClose(this.swf_object_, this.swf_conn_id_);
    this.inbuf_ = "";
    this.outbuf_ = "";
    this.rpc_handshake_state_ = 0;
    this.swf_conn_id_ = null;
    this.HandleError(error_description);
  }
  this.DoHandshake = function()
  {
    LOG_DEBUG("DoHandshake rpc_handshake_state_ = " +
              this.rpc_handshake_state_);
    if(this.rpc_handshake_state_ == 0) {
      GlobalTcpConnectionSend(this.swf_object_,
                              this.swf_conn_id_,
                              this.rpc_mark_ +
                              String.fromCharCode(this.rpc_vers_hi_) +
                              String.fromCharCode(this.rpc_vers_lo_) +
                              String.fromCharCode(this.rpc_codec_id_) +
                              this.rpc_client_hand_);
      this.rpc_handshake_state_ = 1;
      return;
    }
    if(this.rpc_handshake_state_ == 1) {
      if(this.inbuf_.length < 70) {
        LOG_INFO("DoHandshake: not enough data. inbuf_.length()=" +
                 inbuf_.length());
        return; // not enough data
      }
      var mark = this.inbuf_.substr(0, 3);
      var vers_hi = this.inbuf_.charCodeAt(3);
      var vers_lo = this.inbuf_.charCodeAt(4);
      var codec_id = this.inbuf_.charCodeAt(5);
      var server_hand = this.inbuf_.substr(6, 32);
      var client_hand = this.inbuf_.substr(38, 32);
      if(mark != this.rpc_mark_) {
        this.CloseAndClear("handshake failed: bad header");
        return;
      }
      if(vers_hi != this.rpc_vers_hi_ ||
         vers_lo != this.rpc_vers_lo_) {
        this.CloseAndClear("handshake failed: bad version, server is: " +
                           vers_hi + "." + vers_lo + " , we are: " +
                           this.rpc_vers_hi_ + "." + this.rpc_vers_lo_);
        return;
      }
      if(codec_id != this.rpc_codec_id_) {
        this.CloseAndClear("handshake failed: bad codec, server has: " +
                           codec_id + " , we have: " + this.rpc_codec_id_);
        return;
      }
      if(client_hand != this.rpc_client_hand_) {
        this.CloseAndClear("handshake failed: bad client hand");
        return;
      }
      this.inbuf_ = this.inbuf_.substr(70);
      GlobalTcpConnectionSend(this.swf_object_,
                              this.swf_conn_id_, this.rpc_mark_ +
                              String.fromCharCode(this.rpc_vers_hi_) +
                              String.fromCharCode(this.rpc_vers_lo_) +
                              String.fromCharCode(this.rpc_codec_id_) +
                              server_hand);
      this.rpc_handshake_state_ = 2;

      // handshake completed, send outbuf_ (pending messages from client
      // queued while still in handshake)
      if(this.outbuf_.length > 0) {
        GlobalTcpConnectionSend(this.swf_object_,
                                this.swf_conn_id_,
                                this.outbuf_);
        this.outbuf_ = "";
      }

      return;
    }
    this.CloseAndClear("handshake failed: bad rpc_handshake_state_: " +
                       rpc_handshake_state_);
  }
  this.ConnectHandler = function() {
    LOG_DEBUG("RPCTcpConnection connected to: " + this.remote_host_ + ":" +
              this.remote_port_ + " , starting handshake.");
    // start handshake
    this.DoHandshake();
  }
  this.CloseHandler = function() {
    this.CloseAndClear("connection closed by server");
  }
  this.ReadHandler = function(data) {
    LOG_DEBUG("ReadHandler: data=[" + data + "]");
    this.inbuf_ += data;
    if(this.IsOpening()) { // in handshake ?
      this.DoHandshake();
      if(this.IsOpening()) { // still in handshake ?
        return;
      }
      LOG_INFO("ReadHandler: Handshake completed");
      // handshake completed, handle remaining data in inbuf_
      // (pending messages from server arrived just after handshake)
    }
    // parse & deocode multiple packets in inbuf_
    while (true) {
      var len = JSONParseStruct(this.inbuf_);
      if (len == -1) {
        this.CloseAndClear("Bad data in ReadHandler, not a JSON struct. " +
                           "inbuf: [" + this.inbuf_ + "]");
        return;
      }
      if (len == 0) {
        // not enough data in inbuf
        LOG_DEBUG("ReadHandler: not enough data for JSONParseStruct");
        return;
      }
      LOG_INFO("ReadHandler: successfull JSONParseStruct, (struct)length = " +
               len);
      var msg_data = this.inbuf_.substr(0, len);
      this.inbuf_ = this.inbuf_.substr(len);
      var rpc_msg = JSONDecodePacket(msg_data);
      if (rpc_msg == undefined) {
        this.CloseAndClear("Failed to decode RPC message from JSON struct: [" +
                           msg_data + "]");
        return;
      }
      LOG_INFO("ReadHandler: successfull JSONDecodePacket");
      this.HandleMessage(rpc_msg);
    }
  }
  this.WriteHandler = function() {
  }
  this.ErrorHandler = function(error_description) {
    this.CloseAndClear(error_description);
  }
}


g_tcp_connections = new Object();
function GlobalTcpConnectHandler(conn_id) {
  LOG_INFO("GlobalTcpConnectHandler conn_id=" + conn_id);
  g_tcp_connections[conn_id].ConnectHandler();
}
function GlobalTcpCloseHandler(conn_id) {
  LOG_INFO("GlobalTcpCloseHandler conn_id=" + conn_id);
  g_tcp_connections[conn_id].CloseHandler();
}
function GlobalTcpReadHandler(conn_id, escaped_msg) {
  var unescaped_msg = unescape(escaped_msg);
  LOG_INFO("GlobalTcpReadHandler conn_id=" + conn_id +
           " data(unescaped)=[" + unescaped_msg + "]=" +
           unescaped_msg.length + " bytes");
  g_tcp_connections[conn_id].ReadHandler(unescaped_msg);
}
function GlobalTcpWriteHandler(conn_id) {
  g_tcp_connections[conn_id].WriteHandler();
}
function GlobalTcpErrorHandler(conn_id, error_description) {
  LOG_INFO("GlobalTcpErrorHandler conn_id=" + conn_id + " error=[" +
           error_description + "]");
  g_tcp_connections[conn_id].ErrorHandler(error_description);
}

function GlobalTcpConnectionOpen(swf_object, rpc_tcp_connection, host, port) {
  var conn_id = swf_object.TcpConnectionNew({
      "connect_handler" : "GlobalTcpConnectHandler",
      "close_handler" : "GlobalTcpCloseHandler",
      "read_handler" : "GlobalTcpReadHandler",
      "write_handler" : "GlobalTcpWriteHandler",
      "error_handler" : "GlobalTcpErrorHandler"});
  if (conn_id == null) {
    LOG_ERROR("Failed to allocate TcpConnection. error: " +
              swf_object.TcpConnectionGetLastError(null));
    return null;
  }
  g_tcp_connections[conn_id] = rpc_tcp_connection;
  var success = swf_object.TcpConnectionOpen(conn_id, host, port);
  if (!success) {
    LOG_ERROR("Failed to open connection using conn_id: [" + conn_id +
              "], host: [" + host + "], port: [" + port + "]. error: " +
              swf_object.TcpConnectionGetLastError(null));
    GlobalTcpConnectionClose(swf_object, conn_id);
    return null;
  }
  return conn_id;
}

function GlobalTcpConnectionClose(swf_object, conn_id) {
  swf_object.TcpConnectionCloseAndDelete(conn_id);
  g_tcp_connections[conn_id] = null;
}

function GlobalTcpConnectionSend(swf_object, conn_id, unescaped_data) {
  var escaped_data = escape(unescaped_data);
  LOG_INFO("GlobalTcpConnectionSend conn_id=" + conn_id +
           " data(unescaped)=[" + unescaped_data + "]");
  swf_object.TcpConnectionWrite(conn_id, escaped_data);
}


/*********************************************************************/
/*      An RPCConnection using HTTP POST through XMLHttpRequest      */
/*********************************************************************/

// Bridge XMLHTTP to XMLHttpRequest in pre-7.0 Internet Explorers
if (typeof XMLHttpRequest == "undefined")
  XMLHttpRequest = function() {
    try { return new ActiveXObject("Msxml2.XMLHTTP.6.0"); } catch(e) {}
    try { return new ActiveXObject("Msxml2.XMLHTTP.3.0"); } catch(e) {}
    try { return new ActiveXObject("Msxml2.XMLHTTP"); }     catch(e) {}
    try { return new ActiveXObject("Microsoft.XMLHTTP"); }  catch(e) {}
    return null;
  }

// host: RPC server IP or domain name, as string
// port: RPC server port, as number
function RPCHttpXmlConnection(url) {
  // inherit RPCConnection
  var rpc_connection = new RPCConnection();
  for(x in rpc_connection) {
    this[x] = rpc_connection[x];
    LOG_DEBUG("inherit copy: " + x);
  }

  this.url_ = url;

  this.SendMessage = function(rpc_msg) {
    var text = JSONEncodePacket(rpc_msg);
    var This = this;
    var http_request = new XMLHttpRequest();
    if (http_request == null) {
      This.HandleError("browser does not support XMLHttpRequest");
      return;
    }
    http_request.open( "POST", this.url_, true );
    http_request.setRequestHeader( "RPC_Codec_Id", RPC_CID_JSON.toString() )
    http_request.onreadystatechange =  function()  {
      if(http_request.readyState != 4) {
        return;
      }
      if ( http_request.status != 200 ) {
        LOG_ERROR("Http Error: " + http_request.status);
        This.HandleError("http failed, http_request.status=" +
                         http_request.status);
        return;
      }
      var msg = JSONDecodePacket(http_request.responseText);
      if (msg == null) {
        LOG_ERROR("Http Error: " + http_request.status);
        This.HandleError("http failed, http_request.status=" +
                         http_request.status);
        return;
      }
      This.HandleMessage(msg);
      delete http_request;
    }
    http_request.send(text);
  }
}

/*********************************************************************/
/*           An RPCConnection using HTTP POST through Flash          */
/*********************************************************************/

// net_client_swf: the flash movie exporting HttpConnection*** methods
// host: RPC server IP or domain name, as string
// port: RPC server port, as number
function RPCHttpFlashConnection(net_client_swf, url, method) {
  // inherit RPCConnection
  var rpc_connection = new RPCConnection();
  for(x in rpc_connection) {
    this[x] = rpc_connection[x];
    LOG_DEBUG("inherit copy: " + x);
  }

  this.swf_object_ = net_client_swf;
  this.swf_conn_id_ = null;
  this.url_ = url;
  this.method_ = method;

  this.Open = function() {
    this.swf_conn_id_ = GlobalHttpFlashConnectionOpen(
        this.swf_object_, this, this.url_, this.method_);
    if(this.swf_conn_id_ == null) {
      LOG_ERROR("Failed to allocate HttpConnection.");
      return;
    }
  }
  this.IsOpen = function() {
    return this.swf_conn_id_ != null;
  }
  this.SendMessage = function(rpc_msg) {
    var qid = rpc_msg.header_.xid_;
    var data = JSONEncodePacket(rpc_msg);
    if(!this.IsOpen()) {
      this.Open();
    }
    GlobalHttpFlashConnectionSend(this.swf_object_,
                                  this.swf_conn_id_,
                                  qid, {"RpcCodecId" : "2"}, data);
  }
  this.ResponseHandler = function(qid, status, status_text, response_text) {
    if(status != 200) {
      this.HandleError(status_text, qid);
      return;
    }
    var rpc_msg = JSONDecodePacket(response_text);
    if(rpc_msg == undefined) {
      this.HandleError("Failed to decode RPC message from JSON struct: [" +
                       msg_data + "]", qid);
      return;
    }
    this.HandleMessage(rpc_msg);
  }
}

g_http_connections = new Object();
function GlobalHttpFlashResponseHandler(
    conn_id, qid, status, status_text, escaped_response_text) {
  var response_text = unescape(escaped_response_text);
  LOG_INFO("GlobalHttpFlash response conn_id=" + conn_id + " qid=" + qid +
           " status=" + status + " status_text=" + status_text +
           " response_text=" + response_text);
  g_http_connections[conn_id].ResponseHandler(
      qid, status, status_text, response_text);
}

function GlobalHttpFlashConnectionOpen(swf_object,
                                       rpc_http_connection,
                                       url,
                                       method) {
  LOG_INFO("GlobalHttpFlash open url=[" + url + "] method=[" + method + "]");
  var conn_id = swf_object.HttpConnectionNew(
      {"response_handler" : "GlobalHttpFlashResponseHandler"});
  if (conn_id == null) {
    LOG_ERROR("GlobalHttpFlash Failed to allocate HttpConnection.");
    return null;
  }
  g_http_connections[conn_id] = rpc_http_connection;
  var success = swf_object.HttpConnectionOpen(conn_id, url, method);
  if (!success) {
    LOG_ERROR("GlobalHttpFlash Failed to open connection using conn_id: [" +
              conn_id + "], url: [" + url + "], method: [" + method + "]");
    GlobalHttpFlashConnectionClose(swf_object, conn_id);
    return null;
  }
  return conn_id;
}

function GlobalHttpFlashConnectionClose(swf_object, conn_id) {
  LOG_INFO("GlobalHttpFlash close conn_id=" + conn_id);
  swf_object.HttpConnectionDelete(conn_id);
  g_http_connections[conn_id] = null;
}
function GlobalHttpFlashConnectionSend(
    swf_object, conn_id, qid, headers, unescaped_data) {
  LOG_INFO("GlobalHttpFlash send conn_id=" + conn_id + " qid=" +
           qid + " headers=" + headers + " data=" + unescaped_data);
  swf_object.HttpConnectionSend(conn_id, qid, headers, escape(unescaped_data));
}
