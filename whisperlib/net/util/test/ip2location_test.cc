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
//
// Simple test for looking up ips
//

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "net/base/address.h"
#include "net/util/ip2location.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(ip2loc_file,
              "",
              "IP location database");
DEFINE_string(ip,
              "",
              "IP to lookup");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_ip.empty()) << "Please specify an IP";
  CHECK(!FLAGS_ip2loc_file.empty()) << "Please specify the IP2Location DB";

  net::Ip2Location ip2loc(FLAGS_ip2loc_file.c_str());

  LOG_INFO << "LookupCountry: "
           << ip2loc.LookupCountry(FLAGS_ip.c_str());
  LOG_INFO << "LookupCountryLong: "
           << ip2loc.LookupCountryLong(FLAGS_ip.c_str());
  LOG_INFO << "LookupRegion: "
           << ip2loc.LookupRegion(FLAGS_ip.c_str());
  LOG_INFO << "LookupCity: "
           << ip2loc.LookupCity(FLAGS_ip.c_str());
  LOG_INFO << "LookupIsp: "
           << ip2loc.LookupIsp(FLAGS_ip.c_str());
}
