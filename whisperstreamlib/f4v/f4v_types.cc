#include <string>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/strutil.h>
#include "f4v/f4v_types.h"

namespace streaming {
namespace f4v {

const char* TagDecodeStatusName(TagDecodeStatus status) {
  switch(status) {
    CONSIDER(TAG_DECODE_SUCCESS);
    CONSIDER(TAG_DECODE_NO_DATA);
    CONSIDER(TAG_DECODE_ERROR);
    default:
      LOG_FATAL << "Illegal TagDecodeStatus: " << status;
      return "==UNKNOWN==";
  };
}

// TODO(cosmin): probably not accurate. This value is exactly 24107 days.
//       Computed by: http://www.timeanddate.com/date/timeduration.html
const int64 kSecondsFrom1904to1970 = 2082844800LL;

timer::Date MacDate(int64 time) {
  return timer::Date((time - kSecondsFrom1904to1970) * 1000LL, true);
}

uint16 MacLanguageCode(const string& lang) {
  if ( lang.size() < 3 ) {
    LOG_ERROR << "MacLanguageCode requires 3 letters language abbreviation"
                 ", provided: [" << lang << "]";
    return 0;
  }
  uint8 c0 = static_cast<uint8>(lang[0]);
  uint8 c1 = static_cast<uint8>(lang[1]);
  uint8 c2 = static_cast<uint8>(lang[2]);
  if ( c0 < 0x60 || c1 < 0x60 || c2 < 0x60 ) {
    LOG_ERROR << "MacLanguageCode: invalid characters in language "
                 "abbreviation: [" << lang << "]";
    return 0;
  }
  return (static_cast<uint16>(c0 - 0x60) << 10) +
         (static_cast<uint16>(c1 - 0x60) <<  5) +
         (static_cast<uint16>(c2 - 0x60)      );
}
string MacLanguageName(uint16 code) {
  if ( code < 0x800 ) {
    // NOT a MAC language code
    return strutil::StringPrintf("%u", code);
  }
  return strutil::StringPrintf("%c%c%c",
     static_cast<char>(0x60 + ((code >> 10) & 0x1F)),
     static_cast<char>(0x60 + ((code >>  5) & 0x1F)),
     static_cast<char>(0x60 + ((code      ) & 0x1F)));
}


} // namespace f4v
} // namespace streaming
