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

#include "net/util/ip2location.h"
#include "net/util/third-party/IP2Location.h"

namespace net {

Ip2Location::Ip2Location(const char* db_file)
  : iploc_(NULL) {
  iploc_ = IP2Location_open(db_file);
  CHECK(iploc_ != NULL)
    << " Cannot initialize IP2Location structure from file: " << db_file;
}

Ip2Location::~Ip2Location() {
  CHECK(iploc_ != NULL);
  const uint32 err = IP2Location_close(iploc_);
  LOG_INFO << " Closed IP2Location structure. Error code: " << err;
}

#define DEFINE_LOOKUP_FUNCTION(member)                                  \
  IP2LocationRecord* const ret = IP2Location_get_##member(iploc_, addr); \
  if ( ret == NULL ) {                                                  \
    LOG_ERROR << " Error looking up IP: " << addr;                      \
    return string("");                                                  \
  }                                                                     \
  const string ret_str(ret->member); \
  IP2Location_free_record(ret); \
  return ret_str

string Ip2Location::LookupCountry(const char* addr) {
  DEFINE_LOOKUP_FUNCTION(country_short);
}
string Ip2Location::LookupCountryLong(const char* addr) {
  DEFINE_LOOKUP_FUNCTION(country_long);
}
string Ip2Location::LookupRegion(const char* addr) {
  DEFINE_LOOKUP_FUNCTION(region);
}
string Ip2Location::LookupCity(const char* addr) {
  DEFINE_LOOKUP_FUNCTION(city);
}
string Ip2Location::LookupIsp(const char* addr) {
  DEFINE_LOOKUP_FUNCTION(isp);
}
#undef DEFINE_LOOKUP_FUNCTION

bool Ip2Location::LookupInternal(const char* addr, Record* record) {
  IP2LocationRecord* const ret = IP2Location_get_all(iploc_, addr);
  if ( ret == NULL ) {
    LOG_ERROR << " Error looking up IP: " << addr;
    return false;
  }
  record->country_short_ = ret->country_short;
  record->country_long_ = ret->country_long;
  record->region_ = ret->region;
  record->city_ = ret->city;
  record->isp_ = ret->isp;

  if ( cache_.size() > kMaxCacheSize ) {
    delete cache_.begin()->second;
    cache_.erase(cache_.begin());
  }
  cache_.insert(make_pair(IpAddress(addr).ipv4(), new Record(*record)));

  return true;
}
}
