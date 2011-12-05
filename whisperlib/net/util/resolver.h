// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// License Info: New BSD License.
//               Consult the LICENSE.BSD file distributed with this package
//

// This is a utility resolver class. Do not use it in production environments:
// it is synchronous and with unpredictable results.
// Use just for testing and simple utilities.

#ifndef __NET_UTIL_RESOLVER_H__
#define __NET_UTIL_RESOLVER_H__

#include <vector>
#include <string>
#include <whisperlib/net/base/address.h>

namespace net {
namespace util {
// Resolves the given hostname. Returns the first resolved address.
// If all_addresses is not null returns all resolved addresses there.
// On error, the result IsInvalid().
net::HostPort DnsResolve(const char* hostname, const char* service,
                         vector<net::HostPort>* all_addresses);

inline net::HostPort DnsResolve(const string& hostname, const string& service) {
  return DnsResolve(hostname.c_str(), service.c_str(), NULL);
}
inline net::HostPort DnsResolve(const string& hostname) {
  return DnsResolve(hostname.c_str(), NULL, NULL);
}
} // namespace util
} // namespace net

#endif  // __NET_UTIL_RESOLVER_H__
