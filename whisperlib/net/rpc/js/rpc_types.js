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
//
// RPC message types
//
var RPC_CALL = 0;
var RPC_REPLY = 1;

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
                                waiting for reply. */
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

// Codec ID s
//
var RPC_CID_BINARY = 1;
var RPC_CID_JSON = 2;

/////////////////////////////////////////////////////////

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
