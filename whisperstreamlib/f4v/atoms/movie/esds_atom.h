
//
// WARNING !! This is Auto-Generated code by generate_atoms.py !!!
//            do not modify this code
//
#ifndef __MEDIA_F4V_ATOMS_MOVIE_ESDS_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_ESDS_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

namespace streaming {
namespace f4v {

class EsdsAtom : public VersionedAtom {
 public:
  static const AtomType kType = ATOM_ESDS;
  static const int MP4ESDescrTag = 0x03;
  static const int MP4DecConfigDescrTag = 0x04;
  static const int MP4DecSpecificDescrTag = 0x05;
  static const int MP4UnknownTag = 0x06;
 public:
  EsdsAtom();
  EsdsAtom(const EsdsAtom& other);
  virtual ~EsdsAtom();

  const io::MemoryStream& extra_data() const;

  void set_mp4_es_desc_tag_id(uint16 mp4_es_desc_tag_id);
  void set_mp4_es_desc_tag_priority(uint8 mp4_es_desc_tag_priority);
  void set_mp4_dec_config_desc_tag_object_type_id(uint8 mp4_dec_config_desc_tag_object_type_id);
  void set_mp4_dec_config_desc_tag_stream_type(uint8 mp4_dec_config_desc_tag_stream_type);
  void set_mp4_dec_config_desc_tag_buffer_size_db(uint32 mp4_dec_config_desc_tag_buffer_size_db);
  void set_mp4_dec_config_desc_tag_max_bit_rate(uint32 mp4_dec_config_desc_tag_max_bit_rate);
  void set_mp4_dec_config_desc_tag_avg_bit_rate(uint32 mp4_dec_config_desc_tag_avg_bit_rate);
  void set_extra_data(const io::MemoryStream& extra_data);
  void set_extra_data(const void* extra_data, uint32 size);

 private:
  static TagDecodeStatus ReadTagTypeAndLength(io::MemoryStream& in,
      uint8& type, uint32& length);
  static void WriteTagTypeAndLength(io::MemoryStream& out, uint8 type,
      uint32 length);

  ///////////////////////////////////////////////////////////////////////////

  // Methods from BaseAtom
  virtual bool EqualsVersionedBody(const VersionedAtom& other) const;
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size,
                                              io::MemoryStream& in,
                                              Decoder& decoder);
  virtual void EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const;
  virtual uint64 MeasureVersionedBodySize() const;
  virtual string ToStringVersionedBody(uint32 indent) const;

 private:
  // contains the full body. It's safer and easier, the decoding is a mess.
  io::MemoryStream raw_data_;

  uint16 mp4_es_desc_tag_id_;
  uint8 mp4_es_desc_tag_priority_;

  uint8 mp4_dec_config_desc_tag_object_type_id_;
  uint8 mp4_dec_config_desc_tag_stream_type_;
  uint32 mp4_dec_config_desc_tag_buffer_size_db_;
  uint32 mp4_dec_config_desc_tag_max_bit_rate_;
  uint32 mp4_dec_config_desc_tag_avg_bit_rate_;

  io::MemoryStream extra_data_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_ESDS_ATOM_H__
