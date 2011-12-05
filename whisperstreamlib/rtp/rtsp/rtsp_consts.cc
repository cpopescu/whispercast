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

#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>

namespace streaming {
namespace rtsp {

const string kTrackIdName("trackID");
const string kTrackIdAudio("0");
const string kTrackIdVideo("1");

const char* RequestMethodName(REQUEST_METHOD method) {
  switch ( method ) {
    CONSIDER(METHOD_DESCRIBE);
    CONSIDER(METHOD_ANNOUNCE);
    CONSIDER(METHOD_GET_PARAMETER);
    CONSIDER(METHOD_OPTIONS);
    CONSIDER(METHOD_PAUSE);
    CONSIDER(METHOD_PLAY);
    CONSIDER(METHOD_RECORD);
    CONSIDER(METHOD_REDIRECT);
    CONSIDER(METHOD_SETUP);
    CONSIDER(METHOD_SET_PARAMETER);
    CONSIDER(METHOD_TEARDOWN);
  };
  RTSP_LOG_FATAL << "Unknown REQUEST_METHOD: " << method;
  return "UNKNOWN";
}
// !WARNING! Don't change these strings.
//           These names are used in encoding.
const string kRequestMethodDescribe("DESCRIBE");
const string kRequestMethodAnnounce("ANNOUNCE");
const string kRequestMethodGetParameter("GET_PARAMETER");
const string kRequestMethodOptions("OPTIONS");
const string kRequestMethodPause("PAUSE");
const string kRequestMethodPlay("PLAY");
const string kRequestMethodRecord("RECORD");
const string kRequestMethodRedirect("REDIRECT");
const string kRequestMethodSetup("SETUP");
const string kRequestMethodSetParameter("SET_PARAMETER");
const string kRequestMethodTeardown("TEARDOWN");
const string& RequestMethodEnc(REQUEST_METHOD method) {
  switch ( method ) {
    case METHOD_DESCRIBE: return kRequestMethodDescribe;
    case METHOD_ANNOUNCE: return kRequestMethodAnnounce;
    case METHOD_GET_PARAMETER: return kRequestMethodGetParameter;
    case METHOD_OPTIONS: return kRequestMethodOptions;
    case METHOD_PAUSE: return kRequestMethodPause;
    case METHOD_PLAY: return kRequestMethodPlay;
    case METHOD_RECORD: return kRequestMethodRecord;
    case METHOD_REDIRECT: return kRequestMethodRedirect;
    case METHOD_SETUP: return kRequestMethodSetup;
    case METHOD_SET_PARAMETER: return kRequestMethodSetParameter;
    case METHOD_TEARDOWN: return kRequestMethodTeardown;
  }
  RTSP_LOG_FATAL << "Illegal REQUEST_METHOD: " << method;
  static const string kUnknown("UNKNOWN");
  return kUnknown;
}
REQUEST_METHOD RequestMethodDec(const string& method) {
  if ( method == kRequestMethodDescribe) { return METHOD_DESCRIBE; }
  if ( method == kRequestMethodAnnounce) { return METHOD_ANNOUNCE; }
  if ( method == kRequestMethodGetParameter) { return METHOD_GET_PARAMETER; }
  if ( method == kRequestMethodOptions) { return METHOD_OPTIONS; }
  if ( method == kRequestMethodPause) { return METHOD_PAUSE; }
  if ( method == kRequestMethodPlay) { return METHOD_PLAY; }
  if ( method == kRequestMethodRecord) { return METHOD_RECORD; }
  if ( method == kRequestMethodRedirect) { return METHOD_REDIRECT; }
  if ( method == kRequestMethodSetup) { return METHOD_SETUP; }
  if ( method == kRequestMethodSetParameter) { return METHOD_SET_PARAMETER; }
  if ( method == kRequestMethodTeardown) { return METHOD_TEARDOWN; }
  return (REQUEST_METHOD)-1;
}

const char* StatusCodeName(STATUS_CODE status) {
  switch ( status ) {
    CONSIDER(STATUS_CONTINUE);
    CONSIDER(STATUS_OK);
    CONSIDER(STATUS_CREATED);
    CONSIDER(STATUS_LOW_ON_STORAGE_SPACE);
    CONSIDER(STATUS_MULTIPLE_CHOICES);
    CONSIDER(STATUS_MOVED_PERMANENTLY);
    CONSIDER(STATUS_MOVED_TEMPORARILY);
    CONSIDER(STATUS_SEE_OTHER);
    CONSIDER(STATUS_NOT_MODIFIED);
    CONSIDER(STATUS_USE_PROXY);
    CONSIDER(STATUS_BAD_REQUEST);
    CONSIDER(STATUS_UNAUTHORIZED);
    CONSIDER(STATUS_PAYMENT_REQUIRED);
    CONSIDER(STATUS_FORBIDDEN);
    CONSIDER(STATUS_NOT_FOUND);
    CONSIDER(STATUS_METHOD_NOT_ALLOWED);
    CONSIDER(STATUS_NOT_ACCEPTABLE);
    CONSIDER(STATUS_PROXY_AUTHENTICATION_REQUIRED);
    CONSIDER(STATUS_REQUEST_TIME_OUT);
    CONSIDER(STATUS_GONE);
    CONSIDER(STATUS_LENGTH_REQUIRED);
    CONSIDER(STATUS_PRECONDITION_FAILED);
    CONSIDER(STATUS_REQUEST_ENTITY_TOO_LARGE);
    CONSIDER(STATUS_REQUEST_URI_TOO_LARGE);
    CONSIDER(STATUS_UNSUPPORTED_MEDIA_TYPE);
    CONSIDER(STATUS_PARAMETER_NOT_UNDERSTOOD);
    CONSIDER(STATUS_CONFERENCE_NOT_FOUND);
    CONSIDER(STATUS_NOT_ENOUGH_BANDWIDTH);
    CONSIDER(STATUS_SESSION_NOT_FOUND);
    CONSIDER(STATUS_METHOD_NOT_VALID_IN_THIS_STATE);
    CONSIDER(STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE);
    CONSIDER(STATUS_INVALID_RANGE);
    CONSIDER(STATUS_PARAMETER_IS_READ_ONLY);
    CONSIDER(STATUS_AGGREGATE_OPERATION_NOT_ALLOWED);
    CONSIDER(STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED);
    CONSIDER(STATUS_UNSUPPORTED_TRANSPORT);
    CONSIDER(STATUS_DESTINATION_UNREACHABLE);
    CONSIDER(STATUS_INTERNAL_SERVER_ERROR);
    CONSIDER(STATUS_NOT_IMPLEMENTED);
    CONSIDER(STATUS_BAD_GATEWAY);
    CONSIDER(STATUS_SERVICE_UNAVAILABLE);
    CONSIDER(STATUS_GATEWAY_TIME_OUT);
    CONSIDER(STATUS_RTSP_VERSION_NOT_SUPPORTED);
    CONSIDER(STATUS_OPTION_NOT_SUPPORTED);
  };
  RTSP_LOG_FATAL << "Illegal STATUS_CODE: " << status;
  return "UNKNOWN";
}
// !WARNING! Don't change these descriptions.
//           This StatusCodeEnc is used in encoding rtsp::Response.
const char* StatusCodeDescription(STATUS_CODE status) {
  switch ( status ) {
    case STATUS_CONTINUE: return "Continue";
    case STATUS_OK: return "OK";
    case STATUS_CREATED: return "Created";
    case STATUS_LOW_ON_STORAGE_SPACE: return "Low on Storage Space";
    case STATUS_MULTIPLE_CHOICES: return "Multiple Choices";
    case STATUS_MOVED_PERMANENTLY: return "Moved Permanently";
    case STATUS_MOVED_TEMPORARILY: return "Moved Temporarily";
    case STATUS_SEE_OTHER: return "See Other";
    case STATUS_NOT_MODIFIED: return "Not Modified";
    case STATUS_USE_PROXY: return "Use Proxy";
    case STATUS_BAD_REQUEST: return "Bad Request";
    case STATUS_UNAUTHORIZED: return "Unauthorized";
    case STATUS_PAYMENT_REQUIRED: return "Payment Required";
    case STATUS_FORBIDDEN: return "Forbidden";
    case STATUS_NOT_FOUND: return "Not Found";
    case STATUS_METHOD_NOT_ALLOWED: return "Method Not Allowed";
    case STATUS_NOT_ACCEPTABLE: return "Not Acceptable";
    case STATUS_PROXY_AUTHENTICATION_REQUIRED: return "Proxy Authentication Required";
    case STATUS_REQUEST_TIME_OUT: return "Request Time-out";
    case STATUS_GONE: return "Gone";
    case STATUS_LENGTH_REQUIRED: return "Length Required";
    case STATUS_PRECONDITION_FAILED: return "Precondition Failed";
    case STATUS_REQUEST_ENTITY_TOO_LARGE: return "Request Entity Too Large";
    case STATUS_REQUEST_URI_TOO_LARGE: return "Request-URI Too Large";
    case STATUS_UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
    case STATUS_PARAMETER_NOT_UNDERSTOOD: return "Parameter Not Understood";
    case STATUS_CONFERENCE_NOT_FOUND: return "Conference Not Found";
    case STATUS_NOT_ENOUGH_BANDWIDTH: return "Not Enough Bandwidth";
    case STATUS_SESSION_NOT_FOUND: return "Session Not Found";
    case STATUS_METHOD_NOT_VALID_IN_THIS_STATE: return "Method Not Valid in This State";
    case STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE: return "Header Field Not Valid for Resource";
    case STATUS_INVALID_RANGE: return "Invalid Range";
    case STATUS_PARAMETER_IS_READ_ONLY: return "Parameter Is Read-Only";
    case STATUS_AGGREGATE_OPERATION_NOT_ALLOWED: return "Aggregate operation not allowed";
    case STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED: return "Only aggregate operation allowed";
    case STATUS_UNSUPPORTED_TRANSPORT: return "Unsupported transport";
    case STATUS_DESTINATION_UNREACHABLE: return "Destination unreachable";
    case STATUS_INTERNAL_SERVER_ERROR: return "Internal Server Error";
    case STATUS_NOT_IMPLEMENTED: return "Not Implemented";
    case STATUS_BAD_GATEWAY: return "Bad Gateway";
    case STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
    case STATUS_GATEWAY_TIME_OUT: return "Gateway Time-out";
    case STATUS_RTSP_VERSION_NOT_SUPPORTED: return "RTSP Version not supported";
    case STATUS_OPTION_NOT_SUPPORTED: return "Option not supported";
  };
  RTSP_LOG_FATAL << "Unkown STATUS_CODE: " << status;
  return "Unknown";
}
STATUS_CODE StatusCodeDec(uint32 status) {
  switch ( (STATUS_CODE)status ) {
    case STATUS_CONTINUE:
    case STATUS_OK:
    case STATUS_CREATED:
    case STATUS_LOW_ON_STORAGE_SPACE:
    case STATUS_MULTIPLE_CHOICES:
    case STATUS_MOVED_PERMANENTLY:
    case STATUS_MOVED_TEMPORARILY:
    case STATUS_SEE_OTHER:
    case STATUS_NOT_MODIFIED:
    case STATUS_USE_PROXY:
    case STATUS_BAD_REQUEST:
    case STATUS_UNAUTHORIZED:
    case STATUS_PAYMENT_REQUIRED:
    case STATUS_FORBIDDEN:
    case STATUS_NOT_FOUND:
    case STATUS_METHOD_NOT_ALLOWED:
    case STATUS_NOT_ACCEPTABLE:
    case STATUS_PROXY_AUTHENTICATION_REQUIRED:
    case STATUS_REQUEST_TIME_OUT:
    case STATUS_GONE:
    case STATUS_LENGTH_REQUIRED:
    case STATUS_PRECONDITION_FAILED:
    case STATUS_REQUEST_ENTITY_TOO_LARGE:
    case STATUS_REQUEST_URI_TOO_LARGE:
    case STATUS_UNSUPPORTED_MEDIA_TYPE:
    case STATUS_PARAMETER_NOT_UNDERSTOOD:
    case STATUS_CONFERENCE_NOT_FOUND:
    case STATUS_NOT_ENOUGH_BANDWIDTH:
    case STATUS_SESSION_NOT_FOUND:
    case STATUS_METHOD_NOT_VALID_IN_THIS_STATE:
    case STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE:
    case STATUS_INVALID_RANGE:
    case STATUS_PARAMETER_IS_READ_ONLY:
    case STATUS_AGGREGATE_OPERATION_NOT_ALLOWED:
    case STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED:
    case STATUS_UNSUPPORTED_TRANSPORT:
    case STATUS_DESTINATION_UNREACHABLE:
    case STATUS_INTERNAL_SERVER_ERROR:
    case STATUS_NOT_IMPLEMENTED:
    case STATUS_BAD_GATEWAY:
    case STATUS_SERVICE_UNAVAILABLE:
    case STATUS_GATEWAY_TIME_OUT:
    case STATUS_RTSP_VERSION_NOT_SUPPORTED:
    case STATUS_OPTION_NOT_SUPPORTED:
      return (STATUS_CODE)status;
  }
  return (STATUS_CODE)-1;
}

// !WARNING! Don't change these strings.
//           These names are used in encoding.
const string kHeaderFieldAccept("Accept");
const string kHeaderFieldAcceptEncoding("Accept-Encoding");
const string kHeaderFieldAcceptLanguage("Accept-Language");
const string kHeaderFieldAllow("Allow");
const string kHeaderFieldAuthorization("Authorization");
const string kHeaderFieldBandwidth("Bandwidth");
const string kHeaderFieldBlocksize("Blocksize");
const string kHeaderFieldCacheControl("Cache-Control");
const string kHeaderFieldConference("Conference");
const string kHeaderFieldConnection("Connection");
const string kHeaderFieldContentBase("Content-Base");
const string kHeaderFieldContentEncoding("Content-Encoding");
const string kHeaderFieldContentLanguage("Content-Language");
const string kHeaderFieldContentLength("Content-Length");
const string kHeaderFieldContentLocation("Content-Location");
const string kHeaderFieldContentType("Content-Type");
const string kHeaderFieldCSeq("CSeq");
const string kHeaderFieldDate("Date");
const string kHeaderFieldExpires("Expires");
const string kHeaderFieldFrom("From");
const string kHeaderFieldHost("Host");
const string kHeaderFieldIfMatch("If-Match");
const string kHeaderFieldIfModifiedSince("If-Modified-Since");
const string kHeaderFieldLastModified("Last-Modified");
const string kHeaderFieldLocation("Location");
const string kHeaderFieldProxyAuthenticate("Proxy-Authenticate");
const string kHeaderFieldProxyRequire("Proxy-Require");
const string kHeaderFieldPublic("Public");
const string kHeaderFieldRange("Range");
const string kHeaderFieldReferer("Referer");
const string kHeaderFieldRetryAfter("Retry-After");
const string kHeaderFieldRequire("Require");
const string kHeaderFieldRtpInfo("RTP-Info");
const string kHeaderFieldScale("Scale");
const string kHeaderFieldSpeed("Speed");
const string kHeaderFieldServer("Server");
const string kHeaderFieldSession("Session");
const string kHeaderFieldTimestamp("Timestamp");
const string kHeaderFieldTransport("Transport");
const string kHeaderFieldUnsupported("Unsupported");
const string kHeaderFieldUserAgent("User-Agent");
const string kHeaderFieldVary("Vary");
const string kHeaderFieldVia("Via");
const string kHeaderFieldWwwAuthenticate("WWW-Authenticate");

} // namespace rtsp
} // namespace streaming
