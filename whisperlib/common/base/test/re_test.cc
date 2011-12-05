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
#include <stdlib.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/re.h"
#include "common/base/system.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  {
    re::RE re1("^Xcp");
    CHECK(!re1.HasError()) << re1.ErrorName();
    CHECK(re1.Matches("Xcp fjhhs"));
    CHECK(!re1.Matches("Xcdp fjhhs"));
    CHECK(!re1.Matches(""));
  }

  {
    re::RE re2(".*");
    string t("gigi marga");
    CHECK(!re2.HasError()) << re2.ErrorName();
    CHECK(re2.Matches(t));
    string s;
    CHECK(re2.MatchNext(t, &s));
    CHECK_EQ(t, s);
    CHECK(!re2.MatchNext(t, &s));
  }
  {
    re::RE re3("\\(.*\\)-\\1");
    string t("go-go klh loc-loc");

    CHECK(!re3.HasError()) << re3.ErrorName();
    CHECK(re3.Matches(t));
    string s;
    CHECK(re3.MatchNext(t, &s));
    CHECK_EQ(s, string("go-go"));
    CHECK(re3.MatchNext(t, &s));
    CHECK_EQ(s, string("loc-loc"));
    CHECK(!re3.MatchNext(t, &s));
  }

  {
    re::RE re4(
      "\\(.*\\)-\\(.*\\)-\\(.*\\)-\\(.*\\)"
      "-\\(.*\\)-\\(.*\\)-\\(.*\\)-\\(.*\\)-\\(.*\\)"
    );
    string t("1-2-3-4-5-6-7-8-9");

    CHECK(!re4.HasError()) << re4.ErrorName();
    CHECK(re4.Matches(t));
    string s;
    CHECK(re4.Replace(t.c_str(), "a$1+b$2+c$3+d$4+e$5+f$6+g$7+h$8+i$9", s));
    CHECK_EQ(s, string("a1+b2+c3+d4+e5+f6+g7+h8+i9"));
  }
  LOG_INFO << "PASS";
}
