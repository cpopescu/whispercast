// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// License Info: New BSD License.
//               Consult the LICENSE.BSD file distributed with this package
//

#include <netdb.h>
#include <whisperlib/net/util/resolver.h>

namespace net {
namespace util {

net::HostPort DnsResolve(const string& hostname, const string& service,
                         vector<net::HostPort>* all_addresses) {
  return DnsResolve(hostname.c_str(), service.empty() ? NULL : service.c_str(),
                    all_addresses);
}

net::HostPort DnsResolve(const char* hostname, const char* service,
                         vector<net::HostPort>* all_addresses) {
  struct addrinfo* result;
  const int error = getaddrinfo(hostname, service, NULL, &result);
  if (error) {
    LOG_ERROR << " Error resolving [" << hostname << "] - error: " << error;
    return net::HostPort();
  }
  net::HostPort first_address;
  for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
    if (first_address.IsInvalid()) {
      first_address = net::HostPort(
          reinterpret_cast<const struct sockaddr_storage*>(res->ai_addr));
    }
    if (all_addresses == NULL) {
      if (!first_address.IsInvalid()) {
        break;
      }
    } else {
      all_addresses->push_back(
          net::HostPort(
              reinterpret_cast<const struct sockaddr_storage*>(res->ai_addr)));
    }
  }
  freeaddrinfo(result);
  return first_address;
}

} // namespace util
} // namespace net
