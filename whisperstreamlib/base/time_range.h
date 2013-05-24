// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

//
// Utilities for parsing & generating timerange filenames.
// Timerange filename e.g.: "media_20111207-151246-601_20111207-151501-577.flv"

#ifndef __MEDIA_BASE_TIME_RANGE_H__
#define __MEDIA_BASE_TIME_RANGE_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/date.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

// Valid media example:
// "aio_file/live/media_20111207-151246-601_20111207-151501-577.flv"
bool IsValidTimeRangeMedia(const string& media);

// Returns the name of a timerange file.
//  e.g. begin_ts:        07.12.2011 15:12:46.601
//       end_ts:          07.12.2011 15:15:01.577
//       media_format:    MFORMAT_FLV
//       return:          "media_20111207-151246-601_20111207-151501-577.flv"
string MakeTimeRangeFile(int64 begin_ts, int64 end_ts,
                         MediaFormat media_format);

// Extracts begin & end linux timestamps (ms from epoch) from a timerange media.
// e.g. "aio_file/media/media_20111207-151246-601_20111207-151501-577.flv"
//      => out_begin_ts: 07.12.2011 15:12:46.601
//      => out_end_ts:   07.12.2011 15:15:01.577
// Returns success.
bool ParseTimeRangeMedia(const string& stream,
                         int64* out_begin_ts, int64* out_end_ts);

// From the given 'streams' finds the media to play at the given 'play_time'.
// Returns the index of the media containing 'play_time' or -1 if nothing
// is found.
int32 GetTimeRangeMediaIndex(const vector<string>& streams, int64 play_time);

}

#endif  // __MEDIA_BASE_MEDIA_TIME_RANGE_H__
