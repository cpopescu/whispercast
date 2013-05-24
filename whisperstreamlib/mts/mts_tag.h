#ifndef __MEDIA_MTS_MTS_TAG_H__
#define __MEDIA_MTS_MTS_TAG_H__

#include <whisperlib/common/base/ref_counted.h>
#include <whisperstreamlib/mts/mts_consts.h>

namespace streaming {

class MtsTag : public Tag {
 public:
  MtsTag() : Tag(TYPE_MTS, 0, kDefaultFlavourMask) {
  }
  MtsTag(const MtsTag& other) : Tag(TYPE_MTS, 0, kDefaultFlavourMask) {
  }
  virtual ~MtsTag() {}

  /////////////////////////////////////////////////////////////////////
  // Tag methods
  virtual int64 duration_ms() const { LOG_FATAL << "Not implemented"; return 0; }
  virtual int64 composition_offset_ms() const { LOG_FATAL << "Not implemented"; return 0; }
  virtual uint32 size() const { LOG_FATAL << "Not implemented"; return 0; }
  virtual const io::MemoryStream* Data() const { LOG_FATAL << "Not implemented"; return NULL; }
  virtual Tag* Clone()  const { return new MtsTag(*this); }
};

}

#endif // __MEDIA_MTS_MTS_TAG_H__
