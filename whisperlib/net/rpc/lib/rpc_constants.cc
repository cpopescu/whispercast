// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

#include <whisperlib/common/base/log.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>

namespace rpc {

const string kHttpFieldCodec = "Rpc_Codec_Id";
const string kHttpFieldParams = "params";

const string kCodecNameJson = "json";
const string kCodecNameBinary = "binary";

const string& CodecName(CodecId id) {
  switch ( id ) {
    case kCodecIdJson: return kCodecNameJson;
    case kCodecIdBinary: return kCodecNameBinary;
  }
  LOG_FATAL << "Illegal codec id: " << id;
}
bool GetCodecIdFromName(const string& codec_name, CodecId* out_codec_id) {
  if ( codec_name == kCodecNameBinary ) {
    *out_codec_id = kCodecIdBinary;
    return true;
  }
  if ( codec_name == kCodecNameJson ) {
    *out_codec_id = kCodecIdJson;
    return true;
  }
  return false;
}

}
