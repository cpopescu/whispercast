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

#include <stdio.h>
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "net/base/address.h"
#include "net/util/ipclassifier.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(spec,
              "",
              "IP classifier specification");
DEFINE_string(ip,
              "",
              "IP to test");

DECLARE_string(ip2location_classifier_db);

//////////////////////////////////////////////////////////////////////

using namespace net;
void TestClassifier() {
  {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(
      "IPFILTER(1.2.3.4, 1.2.4.0/24)");
    CHECK(ipc->IsInClass(IpAddress("1.2.3.4")));
    CHECK(ipc->IsInClass(IpAddress("1.2.4.0")));
    CHECK(ipc->IsInClass(IpAddress("1.2.4.10")));
    CHECK(ipc->IsInClass(IpAddress("1.2.4.255")));
    CHECK(!ipc->IsInClass(IpAddress("1.2.3.5")));
    CHECK(!ipc->IsInClass(IpAddress("1.2.5.0")));
  }
  {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(
      "OR(IPFILTER(1.2.3.4), IPFILTER(1.2.4.0/24))");
    CHECK(ipc->IsInClass(IpAddress("1.2.3.4")));
    CHECK(ipc->IsInClass(IpAddress("1.2.4.10")));
    CHECK(!ipc->IsInClass(IpAddress("1.2.5.0")));
  }
  {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(
      "NOT(OR(IPFILTER(1.2.3.4), IPFILTER(1.2.4.0/24)))");
    CHECK(!ipc->IsInClass(IpAddress("1.2.3.4")));
    CHECK(!ipc->IsInClass(IpAddress("1.2.4.10")));
    CHECK(ipc->IsInClass(IpAddress("1.2.5.0")));
  }
  {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(
      "AND(IPFILTER(1.2.4.0/28), IPFILTER(1.2.4.0/24))");
    CHECK(!ipc->IsInClass(IpAddress("1.2.3.4")));
    CHECK(ipc->IsInClass(IpAddress("1.2.4.0")));
    CHECK(!ipc->IsInClass(IpAddress("1.2.4.240")));
  }
  if ( !FLAGS_ip2location_classifier_db.empty() ) {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(
      "IPLOC(C:ROMANIA,CITY:BUCHAREST)");
    for ( int i = 0; i < 100; i++ ) {
      CHECK(ipc->IsInClass(IpAddress("193.19.192.20")));
      CHECK(ipc->IsInClass(IpAddress("194.102.93.160")));
      CHECK(!ipc->IsInClass(IpAddress("209.85.129.104")));
    }
  } else {
    LOG_ERROR << " You need to specify --ip2location_classifier_db "
              << "for IpLocationClassifier to work!";
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  if ( !FLAGS_spec.empty() ) {
    net::IpClassifier* ipc = net::IpClassifier::CreateClassifier(FLAGS_spec);
    net::IpAddress ip(FLAGS_ip.c_str());
    const bool matches = ipc->IsInClass(ip);
    delete ipc;
    printf(matches ? "TRUE\n" : "FALSE\n");
    return !matches;
  } else {
    TestClassifier();
  }
}
