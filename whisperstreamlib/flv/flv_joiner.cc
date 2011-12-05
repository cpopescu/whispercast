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
// Authors: Catalin Popescu

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/flv/flv_joiner.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

//////////////////////////////////////////////////////////////////////

namespace streaming {

FlvJoinProcessor::FlvJoinProcessor(bool keep_all_metadata)
  : streaming::JoinProcessor(),
    writer_(true, true),
    keep_all_metadata_(keep_all_metadata),
    has_video_(false),
    has_audio_(false),
    video_tag_count_(0),
    audio_tag_count_(0),
    last_in_timestamp_ms_(-1),
    last_out_timestamp_ms_(0),
    next_cue_timestamp_ms_(0) {
}
FlvJoinProcessor::~FlvJoinProcessor() {
}

void FlvJoinProcessor::MarkNewFile() {
  LOG_DEBUG << "Joiner preparing for new...";
  last_in_timestamp_ms_ = -1;
}

bool FlvJoinProcessor::InitializeOutput(const string& out_file,
                                        int cue_ms_time) {
  if ( !streaming::JoinProcessor::InitializeOutput(out_file, cue_ms_time) ) {
    return false;
  }

  out_file_tmp_ = strutil::JoinPaths(strutil::Dirname(out_file),
                                     "tmp." + strutil::Basename(out_file));
  if ( !writer_.Open(out_file_tmp_) ) {
    LOG_ERROR << "Cannot open temporary output file for writing: ["
              << out_file_tmp_ << "]";
    return false;
  }
  cues_.clear();
  last_in_timestamp_ms_ = -1;
  last_out_timestamp_ms_ = 0;
  next_cue_timestamp_ms_ = 0;
  has_video_ = false;
  has_audio_ = false;
  return true;
}

//////////////////////////////////////////////////////////////////////

//  -- if it's onCuePoint - with navigation type - we skip it
//  -- if it's onMetaData
//          - we keep the first as a witness
//          - we skip the rest, and we compare the values with
//            the first one, in order to check if we can join the files
//
bool FlvJoinProcessor::ProcessMetaData(const FlvTag::Metadata& metadata,
    bool* skip_file) {
  // Check if it is a navigation cue point ..
  if ( metadata.name().value() == kOnCuePoint ) {
    const rtmp::CObject* ctype = metadata.values().Find("type");
    if ( ctype == NULL ||
         ctype->object_type() != rtmp::CObject::CORE_STRING ) {
      return true;
    }
    return static_cast<const rtmp::CString*>(ctype)->value() != "navigation";
  }

  if ( metadata.name().value() != kOnMetaData ) {
    return true;
  }

  // update has_audio_ / has_video_ (because some files have a wrong FlvHeader)
  has_audio_ = has_audio_ || (metadata.values().Find("audiocodecid") != NULL);
  has_video_ = has_video_ || (metadata.values().Find("videocodecid") != NULL);

  // remember first metadata (for later comparison with other incoming metadata)
  if ( metadata_.Empty() ) {
    metadata_.SetAll(metadata.values());
  }
  if ( keep_all_metadata_ ) {
    LOG_DEBUG << "Keeping all metadata...";
    return true;
  }
  // we won't keep this metadata. But let's check if it's identical with the
  // saved one. Identical => continue join. Different => skip file.
  for ( rtmp::CMixedMap::Map::const_iterator it = metadata_.data().begin();
        it != metadata_.data().end(); ++it ) {
    // these values are file specific, they are not a criteria in metadata
    // comparison
    if ( it->first == "cuePoints" ||
         it->first == "duration" ||
         it->first == "datasize" ||
         it->first == "filesize" ||
         it->first == "canSeekToEnd" ||
         it->first == "unseekable" ) {
      continue;
    }
    const rtmp::CObject* obj = metadata.values().Find(it->first);
    if ( obj == NULL ) {
      LOG_ERROR << "Last remembered metadata has attribute: " << it->first
                << " which is missing in current file metadata: "
                << metadata.ToString();
      *skip_file = true;
      return false;
    }
    if ( !it->second->Equals(*obj) ) {
      LOG_ERROR << "Last remembered metadata has attribute: [" << it->first
                << "], with value: " << it->second->ToString()
                << ", while current file metadata, has the same attribute"
                   " with value: " << obj->ToString();
      *skip_file = true;
      return false;
    }
  }
  return false;
}

JoinProcessor::PROCESS_STATUS FlvJoinProcessor::ProcessTag(const Tag* tag) {
  if ( tag->type() == Tag::TYPE_FLV_HEADER ) {
    const FlvHeader* header = static_cast<const FlvHeader*>(tag);
    has_audio_ = has_audio_ || header->has_audio();
    has_video_ = has_video_ || header->has_video();
  }
  if ( tag->type() != Tag::TYPE_FLV ) {
    LOG_WARNING << "Ignoring NON FLV tag: " << tag->ToString();
    return PROCESS_CONTINUE;
  }
  const streaming::FlvTag* flv_tag = static_cast<const streaming::FlvTag*>(tag);

  ////////////
  // Adjust the time stamps of all tags - we start from 0 and update them in
  // monotonic ascending order.
  //

  // Process the meta data in a special way - maybe skip it !
  if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
    bool skip_file = false;
    if ( !ProcessMetaData(flv_tag->metadata_body(), &skip_file) ) {
      LOG_WARNING << "Skipping metadata: "  << flv_tag->ToString();
      return skip_file ? PROCESS_SKIP_FILE : PROCESS_CONTINUE;
    }
    LOG_WARNING << "Keep metadata: " << flv_tag->ToString();
  }

  // mark: the output file contains video
  if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
    video_tag_count_++;
    has_video_ = true;
  }
  if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ) {
    audio_tag_count_++;
    has_audio_ = true;
  }

  // maybe initialize. A file may start with tags from 1234 for example.
  if ( last_in_timestamp_ms_ == -1 ) {
    last_in_timestamp_ms_ = flv_tag->timestamp_ms();
  }
  int64 delta = flv_tag->timestamp_ms() - last_in_timestamp_ms_;
  if ( delta < 0 || delta > 20000 ) {
    LOG_WARNING << "Weird timestamp difference - previous tag ts: "
             << last_in_timestamp_ms_ << " ms, current tag ts: "
             << flv_tag->timestamp_ms() << " ms";
    return PROCESS_CONTINUE;
  }
  // update last timestamps
  last_in_timestamp_ms_ = flv_tag->timestamp_ms();
  last_out_timestamp_ms_ += delta;
  // update tag timestamp
  const_cast<streaming::FlvTag*>(flv_tag)->set_timestamp_ms(last_out_timestamp_ms_);

  // possibly mark a cue point
  if (cue_ms_time_ > 0 && !keep_all_metadata_ ) {
    bool is_keyframe = (flv_tag->is_video_tag() && flv_tag->can_resync());
    if ( !has_video_ || is_keyframe ) {
      // good spot for a cuepoint, just before a keyframe
      if ( last_out_timestamp_ms_ >= next_cue_timestamp_ms_ ) {
        next_cue_timestamp_ms_ += cue_ms_time_;
        LOG_DEBUG << "Marking cue point, has_video_: " << has_video_
                 << ", at ts: " << last_out_timestamp_ms_
                 << ", just before tag: " << flv_tag->ToString();
        cues_.push_back(CuePoint(last_out_timestamp_ms_, writer_.Position()));
      }
    }
  }

  // write
  if ( !writer_.Write(*flv_tag) ) {
    return PROCESS_ABANDON;
  }

  return PROCESS_CONTINUE;
}

void FlvJoinProcessor::WriteCuePoint(int crt_cue,
                                     double position,
                                     uint32 timestamp) {
  scoped_ref<FlvTag> point(new FlvTag(0, kDefaultFlavourMask, timestamp,
      new FlvTag::Metadata()));
  point->set_stream_id(0);
  point->mutable_metadata_body().mutable_name()->set_value(streaming::kOnCuePoint);
  PrepareCuePoint(crt_cue, position, timestamp,
                  point->mutable_metadata_body().mutable_values());
  writer_.Write(point);
}

int64 FlvJoinProcessor::FinalizeFile() {
  // Flush the temp file
  writer_.Close();

  // Open the temp output.
  MediaFileReader reader;
  if ( !reader.Open(out_file_tmp_, TagSplitter::TS_FLV) ) {
    LOG_ERROR << "Cannot open file: [" << out_file_tmp_ << "]";
    return -1;
  }
  if ( reader.splitter()->type() != TagSplitter::TS_FLV ) {
    LOG_ERROR << "Not a FLV file: [" << out_file_tmp_ << "]";
    return -1;
  }
  const int64 file_size = io::GetFileSize(out_file_tmp_);
  if ( file_size < 0 ) {
    LOG_ERROR << "io::GetFileSize failed, file: [" << out_file_tmp_ << "]";
    return -1;
  }

  // Start outputing the good file !

  // set flags before opening the file! Because Open() immediately writes
  // the header. Also, keep in mind that the input files flags may be wrong!!
  writer_.SetFlags(video_tag_count_ > 0, audio_tag_count_ > 0);
  if ( !writer_.Open(out_file_) ) {
    LOG_ERROR << "Failed to open output file: [" << out_file_ << "]";
    return -1;
  }

  int64 duration_ms = last_out_timestamp_ms_;
  if ( !keep_all_metadata_ ) {
    ///////////////////////////
    // Write metadata (include cuepoints)

    // cues_ now contain position inside the tmp file, which has no metadata
    // and no cues. We need to predict the cues positions inside the output
    // file which will contain both metadata & cue points.

    // Prepare metadata with the wrong cue positions.
    PrepareMetadataValues(duration_ms/1000.0, file_size, cues_, &metadata_);

    // Measure metadata
    scoped_ref<FlvTag> tmp_metadata(new FlvTag(0, kDefaultFlavourMask, 0,
              new FlvTag::Metadata(kOnMetaData, metadata_)));
    uint32 metadata_size = FlvTagSerializer::EncodingSize(tmp_metadata.get());

    // Measure a cue point
    scoped_ref<FlvTag> tmp_cuepoint(new FlvTag(0, kDefaultFlavourMask, 0,
              new FlvTag::Metadata()));
    tmp_cuepoint->mutable_metadata_body().mutable_name()->set_value(kOnCuePoint);
    PrepareCuePoint(0,0,0,tmp_cuepoint->mutable_metadata_body().mutable_values());
    uint32 cuepoint_size = FlvTagSerializer::EncodingSize(tmp_cuepoint.get());

    // Update positions inside cues_
    for ( uint32 i = 0; i < cues_.size(); i++ ) {
      cues_[i].position_ += metadata_size + i * cuepoint_size;
    }

    // Prepare metadata again, using the correct positions.
    PrepareMetadataValues(duration_ms/1000.0, file_size, cues_, &metadata_);
    scoped_ref<FlvTag> metadata_tag(new FlvTag(0, kDefaultFlavourMask, 0,
              new FlvTag::Metadata(kOnMetaData, metadata_)));

    // Finally write metadata
    uint64 start_pos = writer_.Position();
    writer_.Write(metadata_tag);
    uint64 encoded_size = writer_.Position() - start_pos;
    CHECK_EQ(metadata_size, encoded_size);
  }

  // Read tags from temp output, write to final output.
  LOG_INFO << "Starting post-processing of: " << out_file_tmp_;
  CHECK_EQ(reader.Position(), 0);
  int crt_cue = 0;
  while ( true ) {
    scoped_ref<streaming::Tag> media_tag;
    streaming::TagReadStatus err = reader.Read(&media_tag);
    if ( err == streaming::READ_EOF ) {
      LOG_INFO << "EOF at position: " << reader.Position()
               << ", useless bytes: " << reader.Remaining();
      break;
    }
    if ( err == streaming::READ_SKIP ) {
      continue;
    }

    if ( err != streaming::READ_OK ) {
      // Bad error (corruption) in the input file..
      LOG_ERROR << "Error reading next tag from [" << out_file_tmp_
                << "], file position: " << reader.Position()
                << ", error: " << TagReadStatusName(err);

      duration_ms = -1;
      goto done;
    }
    CHECK_EQ(err, streaming::READ_OK);
    CHECK_NOT_NULL(media_tag.get());
    if ( media_tag->type() != Tag::TYPE_FLV ) {
      LOG_WARNING << "Ignoring NON FLV tag: " << media_tag->ToString();
      continue;
    }

    streaming::FlvTag* flv_tag = const_cast<streaming::FlvTag*>(
      static_cast<const streaming::FlvTag*>(media_tag.get()));
    if ( !keep_all_metadata_ ) {
      // in the tmp file there should be NO metadata.
      if (flv_tag->body().type() == FLV_FRAMETYPE_METADATA &&
          flv_tag->metadata_body().name().value() == kOnMetaData ) {
        LOG_ERROR << "Unexpected metadata tag in [" << out_file_tmp_
                  << "], file position: " << reader.Position();

        duration_ms = -1;
        goto done;
      }
    }

    // maybe it is time to insert a cue-point ...
    if ( crt_cue < cues_.size() &&
         writer_.Position() >= cues_[crt_cue].position_ ) {
      LOG_DEBUG << "Inserting cue point, index: " << crt_cue
               << ", ts: " << cues_[crt_cue].timestamp_ << " ms"
                  ", position: " << cues_[crt_cue].position_
               << ", before tag: " << flv_tag->ToString();
      CHECK_EQ(cues_[crt_cue].timestamp_, flv_tag->timestamp_ms());
      CHECK_EQ(cues_[crt_cue].position_, writer_.Position());
      if ( has_video_ ) {
        // a cue point always comes before a keyframe (except first cue_point
        // which is at the beginning of the file)
        CHECK(flv_tag->is_video_tag());
        CHECK(flv_tag->can_resync());
      }
      WriteCuePoint(crt_cue, cues_[crt_cue].position_, cues_[crt_cue].timestamp_);
      crt_cue++;
    }
    // write tag
    writer_.Write(*flv_tag);
  }
done:
  LOG_INFO << "Done, output file: [" << out_file_ << "]"
              ", stats: " << writer_.StrStats();
  writer_.Close();
  // reader automatically closes on EOF. It's safe to rm out_file_tmp_.

  io::Rm(out_file_tmp_);
  out_file_tmp_ = "";

  return duration_ms;
}

void FlvJoinProcessor::PrepareCuePoint(int index, double position,
    uint32 timestamp_ms, rtmp::CMixedMap* out) {
  out->Set("time", new rtmp::CNumber(timestamp_ms / 1000.0));
  rtmp::CMixedMap* param_map = new rtmp::CMixedMap();
  param_map->Set("pos", new rtmp::CNumber(position));
  out->Set("parameters", param_map);
  out->Set("name", new rtmp::CString(
      strutil::StringPrintf("point_%06d", index)));
  out->Set("type", new rtmp::CString("navigation"));
}
void FlvJoinProcessor::PrepareCuePoints(const vector<CuePoint>& cues,
    rtmp::CMixedMap* out) {
  for ( int i = 0; i < cues.size(); ++i ) {
    rtmp::CMixedMap* cue_point = new rtmp::CMixedMap();
    PrepareCuePoint(i, cues[i].position_, cues[i].timestamp_, cue_point);
    out->Set(strutil::StringPrintf("%06d", i), cue_point);
  }
}
void FlvJoinProcessor::PrepareMetadataValues(double file_duration,
    int64 file_size, const vector<CuePoint>& cues, rtmp::CMixedMap* out) {
  out->Set("canSeekToEnd", new rtmp::CBoolean(true));
  out->Set("unseekable", new rtmp::CBoolean(false));
  out->Set("datasize", new rtmp::CNumber(file_size));
  out->Set("filesize", new rtmp::CNumber(file_size));
  out->Set("duration", new rtmp::CNumber(file_duration));

  rtmp::CMixedMap* cue_map = new rtmp::CMixedMap();
  PrepareCuePoints(cues, cue_map);
  out->Set("cuePoints", cue_map);
}

//////////////////////////////////////////////////////////////////////

}
