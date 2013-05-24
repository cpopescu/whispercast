// (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
#ifndef __MEDIA_BASE_TAG_SERIALIZER_H__
#define __MEDIA_BASE_TAG_SERIALIZER_H__

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/consts.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////
//
// General interface for serializing a tag into some media format.
//

class TagSerializer {
 public:
  TagSerializer(MediaFormat media_format)
    : media_format_(media_format) { }
  virtual ~TagSerializer() { }

  MediaFormat media_format() const { return media_format_; }
  const char* media_format_name() const {
    return MediaFormatName(media_format_);
  }

  // If any starting things are necessary to be serialized before the
  // actual tags, this is the moment :)
  virtual void Initialize(io::MemoryStream* out) = 0;
  // The main interface function - puts "tag" into "out".
  // It mainly calls serialize internal, but breaking composed tags
  bool Serialize(const Tag* tag, int64 timestamp_ms, io::MemoryStream* out) {
    if ( tag->type() == Tag::TYPE_COMPOSED ) {
      /*
      const ComposedTag* ctd = static_cast<const ComposedTag*>(tag);
      if ( timestamp_ms < 0 ) {
        timestamp_ms = tag->timestamp_ms();
      } else {
        timestamp_ms = timestamp_ms - tag->timestamp_ms();
      }
      for ( int i = 0; is_ok && i < ctd->tags().size(); ++i ) {
        SerializeInternal(ctd->tags().tag(i).get(), timestamp_ms, out);
      }
      */
      return false;
    }
    return SerializeInternal(tag, timestamp_ms, out);
  }
  // If any finishing touches things are necessary to be serialized after the
  // actual tags, this is the moment :)
  virtual void Finalize(io::MemoryStream* out) = 0;

 protected:
  // Override this to do your serialization
  virtual bool SerializeInternal(const Tag* tag,
                                 int64 timestamp_ms,
                                 io::MemoryStream* out) = 0;
 private:
  // Media format = serializer type
  // (there's just 1 serializer implementation per media format)
  const MediaFormat media_format_;

  DISALLOW_EVIL_CONSTRUCTORS(TagSerializer);
};

TagSerializer* CreateSerializer(MediaFormat media_format);

} // namespace streaming

#endif /* __MEDIA_BASE_TAG_SERIALIZER_H__ */
