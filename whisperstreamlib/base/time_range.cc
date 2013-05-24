#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperstreamlib/base/time_range.h>

namespace streaming {
bool IsValidTimeRangeFile(const string& str) {
  static re::RE regex("^media_"
                      "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-"
                      "[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]_"
                      "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-"
                      "[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9].*");
  return regex.Matches(str);
}
bool IsValidTimeRangeMedia(const string& str) {
  return IsValidTimeRangeFile(strutil::SplitLast(str, "/").second);
}

string MakeTimeRangeFile(int64 begin_ts, int64 end_ts, MediaFormat format) {
  return strutil::StringPrintf("media_%s_%s.%s",
      timer::Date(begin_ts, true).ToShortString().c_str(),
      timer::Date(end_ts, true).ToShortString().c_str(),
      MediaFormatToExtension(format));
}

// e.g. "20111207-151246-601"
int64 ParseTimeRangeTime(const string& s) {
  timer::Date d;
  if ( !d.FromShortString(s, true) ) {
    LOG_ERROR << "Illegal timerange time: [" << s << "]";
    return 0;
  }
  return d.GetTime();
}
bool ParseTimeRangeFile(const string& filename,
                        int64* out_begin_ts, int64* out_end_ts) {
  if ( !IsValidTimeRangeFile(filename) ) {
    LOG_ERROR << "Invalid Filename: [" << filename << "]";
    return false;
  }
  vector<string> a;
  strutil::SplitString(strutil::CutExtension(filename), "_", &a);
  CHECK(a.size() == 3) << "Illegal filename: [" << filename << "]";
  *out_begin_ts = ParseTimeRangeTime(a[1]);
  *out_end_ts = ParseTimeRangeTime(a[2]);
  return true;
}
bool ParseTimeRangeMedia(const string& media,
                         int64* out_begin_ts, int64* out_end_ts) {
  return ParseTimeRangeFile(strutil::SplitLast(media, "/").second,
            out_begin_ts, out_end_ts);
}

int32 GetTimeRangeMediaIndex(const vector<string>& streams, int64 play_ts) {
  if ( streams.empty() ) {
    return -1; // not found
  }
  ///////////////////////////////////////////////////////////////////
  // Binary Search
  // NOTE: Streams may have gaps between them! If 'play_stream' is between
  //       streams[k] and streams[k+1] then 'streams[k+1]' is returned.
  //
  int32 start_index = 0;
  int32 end_index = streams.size() - 1;
  while ( true ) {
    if ( start_index > end_index ) {
      if ( end_index == -1 || start_index == streams.size() ) {
        return -1; // not found: the 'play_ts' is outside the streams array
      }
      // 'play_ts' is inside streams array, but between 2 consecutive
      // streams: stream[end_index] < play_ts < stream[start_index]
      return start_index;
    }
    uint32 index = (end_index + start_index) / 2;
    int64 begin_ts, end_ts;
    ParseTimeRangeMedia(streams[index], &begin_ts, &end_ts);
    if ( begin_ts <= play_ts && play_ts <= end_ts ) {
      return index; // found
    }
    // shorten the interval
    if ( play_ts < begin_ts ) {
      end_index = index - 1;
      continue;
    }
    CHECK_GT(play_ts, end_ts);
    start_index = index + 1;
    continue;
  }
}

}
