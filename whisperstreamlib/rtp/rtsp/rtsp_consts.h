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

#ifndef __MEDIA_RTP_RTSP_CONSTS_H__
#define __MEDIA_RTP_RTSP_CONSTS_H__

#include <whisperlib/common/base/types.h>

namespace streaming {
namespace rtsp {

const uint32 kRtspVersionHi = 1;
const uint32 kRtspVersionLo = 0;

const uint16 kDefaultPort = 554;

// default session timeout, for server side, in seconds
const uint32 kSessionTimeoutSec = 60;

// For generating unique URL for audio/video streams.
// e.g. audio stream: rtsp://.../file.f4v?trackID=0
//      video stream: rtsp://.../file.f4v?trackID=1
extern const string kTrackIdName;
extern const string kTrackIdAudio;
extern const string kTrackIdVideo;

enum REQUEST_METHOD {
  METHOD_DESCRIBE,
  METHOD_ANNOUNCE,
  METHOD_GET_PARAMETER,
  METHOD_OPTIONS,
  METHOD_PAUSE,
  METHOD_PLAY,
  METHOD_RECORD,
  METHOD_REDIRECT,
  METHOD_SETUP,
  METHOD_SET_PARAMETER,
  METHOD_TEARDOWN,
};
const char* RequestMethodName(REQUEST_METHOD method);
const string& RequestMethodEnc(REQUEST_METHOD method);
// Takes a method string (network format), returns the REQUEST_METHOD constant.
// e.g. for "SETUP" => returns METHOD_SETUP
//      for "invalid" => returns -1
REQUEST_METHOD RequestMethodDec(const string& method);

enum STATUS_CODE {
  STATUS_CONTINUE = 100,
  STATUS_OK = 200,
  STATUS_CREATED = 201,
  STATUS_LOW_ON_STORAGE_SPACE = 250,
  STATUS_MULTIPLE_CHOICES = 300,
  STATUS_MOVED_PERMANENTLY = 301,
  STATUS_MOVED_TEMPORARILY = 302,
  STATUS_SEE_OTHER = 303,
  STATUS_NOT_MODIFIED = 304,
  STATUS_USE_PROXY = 305,
  STATUS_BAD_REQUEST = 400,
  STATUS_UNAUTHORIZED = 401,
  STATUS_PAYMENT_REQUIRED = 402,
  STATUS_FORBIDDEN = 403,
  STATUS_NOT_FOUND = 404,
  STATUS_METHOD_NOT_ALLOWED = 405,
  STATUS_NOT_ACCEPTABLE = 406,
  STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
  STATUS_REQUEST_TIME_OUT = 408,
  STATUS_GONE = 410,
  STATUS_LENGTH_REQUIRED = 411,
  STATUS_PRECONDITION_FAILED = 412,
  STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
  STATUS_REQUEST_URI_TOO_LARGE = 414,
  STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
  STATUS_PARAMETER_NOT_UNDERSTOOD = 451,
  STATUS_CONFERENCE_NOT_FOUND = 452,
  STATUS_NOT_ENOUGH_BANDWIDTH = 453,
  STATUS_SESSION_NOT_FOUND = 454,
  STATUS_METHOD_NOT_VALID_IN_THIS_STATE = 455,
  STATUS_HEADER_FIELD_NOT_VALID_FOR_RESOURCE = 456,
  STATUS_INVALID_RANGE = 457,
  STATUS_PARAMETER_IS_READ_ONLY = 458,
  STATUS_AGGREGATE_OPERATION_NOT_ALLOWED = 459,
  STATUS_ONLY_AGGREGATE_OPERATION_ALLOWED = 460,
  STATUS_UNSUPPORTED_TRANSPORT = 461,
  STATUS_DESTINATION_UNREACHABLE = 462,
  STATUS_INTERNAL_SERVER_ERROR = 500,
  STATUS_NOT_IMPLEMENTED = 501,
  STATUS_BAD_GATEWAY = 502,
  STATUS_SERVICE_UNAVAILABLE = 503,
  STATUS_GATEWAY_TIME_OUT = 504,
  STATUS_RTSP_VERSION_NOT_SUPPORTED = 505,
  STATUS_OPTION_NOT_SUPPORTED = 551,
};
const char* StatusCodeName(STATUS_CODE status);
const char* StatusCodeDescription(STATUS_CODE status);
// Takes a status code, returns the STATUS_CODE constant.
// e.g. for 200 => returns STATUS_OK
//      for 137 => returns -1
STATUS_CODE StatusCodeDec(uint32 status);


extern const string kHeaderFieldAccept;
extern const string kHeaderFieldAcceptEncoding;
extern const string kHeaderFieldAcceptLanguage;
extern const string kHeaderFieldAllow;
extern const string kHeaderFieldAuthorization;
extern const string kHeaderFieldBandwidth;
extern const string kHeaderFieldBlocksize;
extern const string kHeaderFieldCacheControl;
extern const string kHeaderFieldConference;
extern const string kHeaderFieldConnection;
extern const string kHeaderFieldContentBase;
extern const string kHeaderFieldContentEncoding;
extern const string kHeaderFieldContentLanguage;
extern const string kHeaderFieldContentLength;
extern const string kHeaderFieldContentLocation;
extern const string kHeaderFieldContentType;
extern const string kHeaderFieldCSeq;
extern const string kHeaderFieldDate;
extern const string kHeaderFieldExpires;
extern const string kHeaderFieldFrom;
extern const string kHeaderFieldHost;
extern const string kHeaderFieldIfMatch;
extern const string kHeaderFieldIfModifiedSince;
extern const string kHeaderFieldLastModified;
extern const string kHeaderFieldLocation;
extern const string kHeaderFieldProxyAuthenticate;
extern const string kHeaderFieldProxyRequire;
extern const string kHeaderFieldPublic;
extern const string kHeaderFieldRange;
extern const string kHeaderFieldReferer;
extern const string kHeaderFieldRetryAfter;
extern const string kHeaderFieldRequire;
extern const string kHeaderFieldRtpInfo;
extern const string kHeaderFieldScale;
extern const string kHeaderFieldSpeed;
extern const string kHeaderFieldServer;
extern const string kHeaderFieldSession;
extern const string kHeaderFieldTimestamp;
extern const string kHeaderFieldTransport;
extern const string kHeaderFieldUnsupported;
extern const string kHeaderFieldUserAgent;
extern const string kHeaderFieldVary;
extern const string kHeaderFieldVia;
extern const string kHeaderFieldWwwAuthenticate;

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_CONSTS_H__
