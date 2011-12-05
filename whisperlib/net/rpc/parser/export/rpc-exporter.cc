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

#include "net/rpc/parser/export/rpc-exporter.h"
#include <whisperlib/common/base/strutil.h>

rpc::Exporter::Exporter(const string& languageName,
                        const PCustomTypeArray& customTypes,
                        const PServiceArray& services,
                        const PVerbatimArray& verbatim,
                        const vector<string>& inputFilenames)
    : languageName_(languageName),
      customTypes_(customTypes),
      services_(services),
      verbatim_(verbatim),
      inputFilenames_(inputFilenames) {
}
rpc::Exporter::~Exporter() {
}

const string& rpc::Exporter::GetLanguageName() const {
  return languageName_;
}

string rpc::Exporter::GetVerbatim() const {
  vector<string> texts;
  for ( PVerbatimArray::const_iterator it = verbatim_.begin();
        it != verbatim_.end(); ++it ) {
    if ( it->language_ == languageName_ ) {
      texts.push_back(it->verbatim_);
    }
  }
  return strutil::JoinStrings(texts, "\n");
}
