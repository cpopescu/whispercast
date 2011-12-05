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
// One layer over ip2location library ..
//
#ifndef __NET_UTIL_IP2LOCATION_H__
#define __NET_UTIL_IP2LOCATION_H__

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include <whisperlib/net/base/address.h>

struct IP2Location;
namespace net {

class Ip2Location {
 public:
  Ip2Location(const char* db_file);
  ~Ip2Location();

  string LookupCountry(const char* addr);
  string LookupCountryLong(const char* addr);
  string LookupRegion(const char* addr);
  string LookupCity(const char* addr);
  string LookupIsp(const char* addr);

  string LookupCountry(const IpAddress& addr) {
    return LookupCountry(addr.ToString().c_str());
  }
  string LookupCountryLong(const IpAddress& addr) {
    return LookupCountryLong(addr.ToString().c_str());
  }
  string LookupRegion(const IpAddress& addr) {
    return LookupRegion(addr.ToString().c_str());
  }
  string LookupCity(const IpAddress& addr) {
    return LookupCity(addr.ToString().c_str());
  }
  string LookupIsp(const IpAddress& addr) {
    return LookupIsp(addr.ToString().c_str());
  }

  struct Record {
    string country_short_;
    string country_long_;
    string region_;
    string city_;
    string isp_;
    Record() {
    }
    Record(const Record& r) {
      *this = r;
    }
    const Record& operator=(const Record& r) {
      country_short_ = r.country_short_;
      country_long_ = r.country_long_;
      region_ = r.region_;
      city_ = r.city_;
      isp_ = r.isp_;
      return *this;
    }
  };
  bool LookupAll(const IpAddress& addr, Record* record) {
    if ( !addr.is_ipv4() ) return false;
    const CacheMap::const_iterator it = cache_.find(addr.ipv4());
    if ( it != cache_.end() ) {
      *record = *it->second;
      return true;
    }
    return LookupInternal(addr.ToString().c_str(), record);
  }
  bool LookupAll(const char* addr, Record* record) {
    net::IpAddress ip_ob(addr);
    if ( !ip_ob.is_ipv4() ) {
      return false;
    }
    const int32 ip = ip_ob.ipv4();
    const CacheMap::const_iterator it = cache_.find(ip);
    if ( it != cache_.end() ) {
      *record = *it->second;
      return true;
    }
    return LookupInternal(addr, record);
  }

 private:
  bool LookupInternal(const char* addr, Record* record);

  IP2Location* iploc_;
  // A cache of last lookups
  typedef hash_map<int32, Record*> CacheMap;
  CacheMap cache_;
  static const int kMaxCacheSize = 1000;
  DISALLOW_EVIL_CONSTRUCTORS(Ip2Location);
};
}

#endif  // __NET_UTIL_IP2LOCATION_H__
