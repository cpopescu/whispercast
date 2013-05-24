// Copyright (c) 2011, Whispersoft s.r.l.
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
//
#ifndef __NET_BASE_DNS_RESOLVER_H__
#define __NET_BASE_DNS_RESOLVER_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/ref_counted.h>
#include <whisperlib/common/base/callback/callback1.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/base/selector.h>

namespace net {

// Asynchronous DNS resolver.
// Usage:
//   - DnsInit() // before resolving anything
//   - DnsResolve(..) // start a DNS query
//   either: - your result callback is called with the resolved query
//           - Or you call DnsCancel(..) to cancel the query.
// Implementation:
//   Uses a single separate thread (solver) to resolve the queries.
//   The solver receives queries through a limited ProducerConsumerQueue.
//   The actual resolve uses ::getaddrinfo().. but that may be changed.

struct DnsHostInfo : public RefCounted {
  string hostname_;
  vector<IpAddress> ipv4_;
  vector<IpAddress> ipv6_;
  DnsHostInfo(const string& hostname, set<IpAddress> ipv4, set<IpAddress> ipv6)
    : RefCounted(),
      hostname_(hostname),
      ipv4_(ipv4.begin(), ipv4.end()),
      ipv6_(ipv6.begin(), ipv6.end()) {}
  string ToString() const {
    return strutil::StringPrintf(
        "DnsHostInfo{hostname_: '%s', ipv4_: %s, ipv6_: %s}",
        hostname_.c_str(),
        strutil::ToString(ipv4_).c_str(),
        strutil::ToString(ipv6_).c_str());
  }
};
typedef Callback1< scoped_ref<DnsHostInfo> > DnsResultHandler;

// Initialize DNS.
void DnsInit();

// Queue a DNS query. The result will be delivered through 'result_handler'
// on the given 'selector'.
// On success, the result is a vector containing all host IPs.
// On error, an the result is an empty vector.
void DnsResolve(net::Selector* selector, const string& hostname,
                DnsResultHandler* result_handler);

// Cancel a pending DNS query.
void DnsCancel(DnsResultHandler* result_handler);

// Stop DNS.
void DnsExit();

// Synchronous DNS query.
scoped_ref<DnsHostInfo> DnsBlockingResolve(const string& hostname);

}

inline ostream& operator<<(ostream& os, const net::DnsHostInfo& info) {
  return os << info.ToString();
}

#endif // __NET_BASE_DNS_RESOLVER_H__
