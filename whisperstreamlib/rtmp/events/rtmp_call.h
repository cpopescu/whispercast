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

#ifndef __NET_RTMP_EVENTS_RTMP_CALL_H__
#define __NET_RTMP_EVENTS_RTMP_CALL_H__

// This helper object encapsulates some data used in calls to
// a rtmp server. We encapsulate one of this in EventNotify and
// EventInvoke.
// Because of some extra parameters needed for serialization method
// we chose not to make this an CObject .. but this is a negociable
// fact..

#include <string>
#include <deque>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

// - EventInvoke carries a PendingCall
// - EventNotify carries just a Call

namespace rtmp {

class Call {
 public:
  typedef deque<CObject*> ArgVector;
  enum Status {
    // Pending status constant
    STATUS_PENDING = 0x01,
    // Success result constant
    STATUS_SUCCESS_RESULT = 0x02,
    // Returned value is null constant
    STATUS_SUCCESS_NULL = 0x03,
    // Service returns no value constant
    STATUS_SUCCESS_VOID = 0x04,
    // Service not found constant
    STATUS_SERVICE_NOT_FOUND = 0x10,
    // Service's method not found constant
    STATUS_METHOD_NOT_FOUND = 0x11,
    // Access denied constant
    STATUS_ACCESS_DENIED = 0x12,
    // Exception on invocation constant
    STATUS_INVOCATION_EXCEPTION = 0x13,
    // General exception constant
    STATUS_GENERAL_EXCEPTION = 0x14,
  };
  static const char* StatusName(Status status);

 public:
  //Call() : invoke_id_(0),
  //         status_(STATUS_PENDING),
  //         connection_params_(NULL) { }
  // we take control of 'connection_params'
  Call(const string& service_name, const string& method_name, uint32 invoke_id,
       Status status, CObject* connection_params) :
      service_name_(service_name),
      method_name_(method_name),
      invoke_id_(invoke_id),
      status_(status),
      connection_params_(connection_params) {
  }
  virtual ~Call() {
    while ( !arguments_.empty() ) {
      delete *arguments_.begin();
      arguments_.pop_front();
    }
    delete connection_params_;
    connection_params_ = NULL;
  }

  bool IsSuccess() const {
    return (status_ == STATUS_SUCCESS_RESULT ||
            status_ == STATUS_SUCCESS_NULL ||
            status_ == STATUS_SUCCESS_VOID);
  }
  Status status() const {
    return status_;
  }
  const char* status_name() const {
    return StatusName(status_);
  }
  void set_status(Status status) {
    status_ = status;
  }

  const string& method_name() const {
    return method_name_;
  }
  void set_method_name(const string& method_name) {
    method_name_ = method_name;
  }

  const string& service_name() const {
    return service_name_;
  }
  void set_service_name(const string& service_name) {
    service_name_ = service_name;
  }

  uint32 invoke_id() const {
    return invoke_id_;
  }
  void set_invoke_id(uint32 invoke_id) {
    invoke_id_ = invoke_id;
  }

  const CObject* connection_params() const {
    return connection_params_;
  }
  // Setter for connection_params_ (we take control of the provided object)
  void set_connection_params(CObject* connection_params) {
    delete connection_params_;
    connection_params_ = connection_params;
  }

  const ArgVector& arguments() const {
    return arguments_;
  }

  // Important Note !! we do not own arguments !!
  void AddArgument(CObject* arg) {
    arguments_.push_back(arg);
  }
  void AddArguments(const ArgVector& args) {
    for ( int i = 0; i < args.size(); i++ ) {
      AddArgument(args[i]);
    }
  }

  virtual string ToString() const;

  AmfUtil::ReadStatus Decode(io::MemoryStream* in, AmfUtil::Version version);

  void Encode(bool write_invoke_id, AmfUtil::Version version,
              io::MemoryStream* out);

 private:
  string service_name_;
  string method_name_;
  uint32 invoke_id_;
  Status status_;
  CObject* connection_params_;
 protected:
  ArgVector arguments_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Call);
};

// A pending call is just a Call with a result (which MUST be the first
// argument).
class PendingCall : public Call {
 public:
  // we take control of both 'connection_params' and 'result'
  PendingCall(const string& service, const string& method, uint32 invoke_id,
              Status status, CObject* connection_params, CObject* result)
      : Call(service, method, invoke_id, status, connection_params) {
    if ( result != NULL ) {
      AddArgument(result);
    }
  }
  virtual ~PendingCall() {
  }
  const CObject* result() const {
    return arguments_.empty() ? NULL : arguments_.front();
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(PendingCall);
};
}

#endif  // __NET_RTMP_EVENTS_RTMP_CALL_H__
