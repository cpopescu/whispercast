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
// Author: Catalin Popescu

// A utility to manage a bunch of media element callbacks, which allows add,
// remove, running a tag through all registered callbacks and assorted.
// We take special care in insuring that adding/removing during tag processing
// by callbacks does not screw our internal structures.

#ifndef  __MEDIA_BASE_CALLBACKS_MANAGER_H__
#define  __MEDIA_BASE_CALLBACKS_MANAGER_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/element.h>

namespace streaming {

class CallbacksManager {
 public:
  typedef map<Request*, ProcessingCallback*> CallbackMap;
 public:
  CallbacksManager()
      : distributing_tag_(false) {
  }
  ~CallbacksManager() {
    CHECK(callbacks_.empty());
  }

  void add_callback(Request* req, ProcessingCallback* callback);
  void remove_callback(Request* req);

  uint32 callbacks_size() const {
    return callbacks_.size();
  }

  // Distributes a tag to all our callbacks
  void DistributeTag(const Tag* tag, int64 timestamp_ms);

  // remove all callbacks from us, add them to 'out'
  void MoveAllCallbacks(map<Request*, ProcessingCallback*>* out) {
    for ( CallbackMap::const_iterator it = callbacks_.begin();
          it != callbacks_.end(); ++it ) {
      out->insert(make_pair(it->first, it->second));
    }
    callbacks_.clear();
  }

 private:
  CallbackMap callbacks_;

  // Bug trap: shows that DistributeTag() is running. The execution must NOT
  //           call add/remove callback or DistributeCallback.
  bool distributing_tag_;

  DISALLOW_EVIL_CONSTRUCTORS(CallbacksManager);
};
}

#endif
