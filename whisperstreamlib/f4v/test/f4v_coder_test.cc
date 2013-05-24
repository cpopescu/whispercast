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
// Authors: Cosmin Tudorache

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/base/media_file_writer.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(f4v_path,
              "",
              "A regular f4v/mp4 file to test decoding & encoding on."
              " It MUST be smaller than 2GB!!.");

//////////////////////////////////////////////////////////////////////

// returns the first index 'i' where a[i] != b[i]
uint32 DiffIndex(const uint8* a, uint32 a_size, const uint8* b, uint32 b_size) {
  uint32 i = 0;
  for ( i = 0; i < min(a_size, b_size) && a[i] == b[i]; i++ ) {
  }
  return i;
}
// compare two memory streams
bool Compare(const io::MemoryStream& const_a, const io::MemoryStream& const_b) {
  io::MemoryStream& a = const_cast<io::MemoryStream&>(const_a);
  io::MemoryStream& b = const_cast<io::MemoryStream&>(const_b);
  if ( a.Size() != b.Size() ) {
    LOG_ERROR << "Stream size differ: " << a.Size() << " != " << b.Size();
    return false;
  }

  a.MarkerSet();
  b.MarkerSet();
  int32 index = 0;
  while ( !a.IsEmpty() && !b.IsEmpty() ) {
    uint8 a_buf[128] = {0};
    int32 a_read = a.Read(a_buf, sizeof(a_buf));
    uint8 b_buf[128] = {0};
    int32 b_read = b.Read(b_buf, sizeof(b_buf));
    CHECK_EQ(a_read, b_read);
    if ( ::memcmp(a_buf, b_buf, a_read) != 0 ) {
      LOG_ERROR << "Stream difference"
                << ", at index: "
                << (index + DiffIndex(a_buf, a_read, b_buf, b_read));
                //<< ", a: " << a.DumpContentHex()
                //<< ", b: " << b.DumpContentHex();
      a.MarkerRestore();
      b.MarkerRestore();
      return false;
    }
    index += a_read;
  }
  a.MarkerRestore();
  b.MarkerRestore();
  return true;
}
// Test that atom.Encode() and atom.Clone()->Encode() produce the same output.
bool TestClone(const streaming::f4v::BaseAtom& atom) {
  vector<const streaming::f4v::BaseAtom*> subatoms;
  atom.GetSubatoms(subatoms);
  for ( uint32 i = 0; i < subatoms.size(); i++ ) {
    if ( !TestClone(*subatoms[i]) ) {
      return false;
    }
  }
  streaming::f4v::BaseAtom* clone = atom.Clone();
  scoped_ptr<streaming::f4v::BaseAtom> auto_del_clone(clone);
  if ( !atom.Equals(*clone) ) {
    LOG_ERROR << "Clone error:" << endl
        << "# Original: " << atom.ToString() << endl
        << "# Clone: " << clone->ToString();
    return false;
  }
  return true;
}
// Test that frame.Encode() and frame.Clone()->Encode() produce the same output.
bool TestClone(const streaming::f4v::Frame& frame) {
  const streaming::f4v::Frame* clone = frame.Clone();
  scoped_ptr<const streaming::f4v::Frame> auto_del_clone(clone);

  if ( !frame.Equals(*clone) ) {
    LOG_ERROR << "Clone error: " << endl
              << "# original: " << frame.ToString() << endl
              << "# clone: " << clone->ToString() << endl;
    return false;
  }
  return true;
}
bool TestClone(const streaming::F4vTag& tag) {
  if ( tag.is_atom() ) {
    return TestClone(*tag.atom());
  }
  CHECK(tag.is_frame());
  return TestClone(*tag.frame());
}

// Encode 'tag' and compare output data to 'expected_data'
//
bool TestEncode(streaming::f4v::Encoder& encoder,
                const streaming::F4vTag& tag,
                const io::MemoryStream& expected_data) {
  // encode tag and get the data buffer
  //
  io::MemoryStream out;
  encoder.WriteTag(out, tag);

  if ( Compare(out, expected_data) ) {
    return true;
  }
  LOG_ERROR << "Encoding error: " << endl
            << "# tag: " << tag.ToString() << endl
            << "#encoded as: " << out.DumpContent() << endl
            << "#expected: " << expected_data.DumpContent();
  return false;
}

// Encode 'atom' to memory, decode from memory,
// and compare the result with the original 'atom'
bool TestEncode(const streaming::f4v::BaseAtom& atom) {
  // exclude MDAT atom, because it requires a previously serialized MOOV
  if ( atom.type() == streaming::f4v::ATOM_MDAT ) { return true; }

  // recursively test subatoms
  vector<const streaming::f4v::BaseAtom*> subatoms;
  atom.GetSubatoms(subatoms);
  for ( uint32 i = 0; i < subatoms.size(); i++ ) {
    if ( !TestEncode(*subatoms[i]) ) {
      return false;
    }
  }

  io::MemoryStream ms;
  streaming::f4v::Encoder().WriteAtom(ms, atom);
  ms.MarkerSet();
  scoped_ref<streaming::F4vTag> result;
  streaming::f4v::TagDecodeStatus status =
      streaming::f4v::Decoder().ReadTag(ms, &result);
  ms.MarkerRestore();
  if ( status != streaming::f4v::TAG_DECODE_SUCCESS ) {
    LOG_ERROR << "Atom: " << atom.ToString() << " was encoded to: "
              << ms.DumpContentHex() << " but the decoder cannot decode it;"
                 " result: " << streaming::f4v::TagDecodeStatusName(status);
    return false;
  }
  if ( !result->is_atom() ) {
    LOG_ERROR << "Encoded an atom: " << atom.ToString()
              << ", decoded something else: " << result->ToString();
    return false;
  }
  if ( !atom.Equals(*result->atom()) ) {
    LOG_ERROR << " Encoding error: " << endl
              << "# Original atom: " << atom.ToString() << endl
              << "# Decoded atom: " << result->atom()->ToString();
    return false;
  }
  return true;
}

// Encode 'atom' and compare the encoding size to atom->MeasureSize()
//
bool TestMeasureSize(const streaming::f4v::BaseAtom& atom) {
  io::MemoryStream ms;
  streaming::f4v::Encoder().WriteAtom(ms, atom);
  if ( ms.Size() != atom.MeasureSize() ) {
    LOG_ERROR << "WRONG MeasureSize(), for atom: " << atom.ToString() << "\n"
                 " - MeasureSize(): " << atom.MeasureSize() << "\n"
                 " - encoding size: " << ms.Size();
    return false;
  }
  // recursively check subatoms
  vector<const streaming::f4v::BaseAtom*> subatoms;
  atom.GetSubatoms(subatoms);
  for ( uint32 i = 0; i < subatoms.size(); i++ ) {
    if ( !TestMeasureSize(*subatoms[i]) ) {
      return false;
    }
  }
  return true;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  if ( FLAGS_f4v_path.empty() ) {
    LOG_ERROR << "Please specify f4v_path parameter !";
    common::Exit(1);
  }

  /*
  ///////////////////////////////////////////////////////////////////////
  // TEST 1:
  //  - read f4v tags (using direct decoding, rather than generic tags)
  //  - check that they really come in offset order
  //  - check atom->MeasureSize() == atom's encoding size
  //  - check encoding: data->decode->encode == data
  //                    atom->encode->decode == atom
  //  - check clone: tag->clone == tag. For every tag in file. This test should
  //                 be done with various tags, not just with file tags.
  {
    LOG_WARNING << "Checking the atoms and frames of original file";
    streaming::MediaFileReader reader;
    if ( !reader.Open(FLAGS_f4v_path) ) {
      LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
      common::Exit(1);
    }
    CHECK_EQ(reader.splitter()->media_format(), streaming::MFORMAT_F4V);
    reader.splitter()->set_generic_tags(false);
    streaming::f4v::Encoder encoder; // for original tags

    streaming::f4v::FrameHeader prev_frame_header; // timestamp 0
    uint32 frame_index = 0;
    while ( true ) {
      scoped_ref<streaming::Tag> tag;
      int64 ts = 0;
      io::MemoryStream tag_data;
      uint64 file_pos = reader.Position();

      // read tag
      streaming::TagReadStatus result = reader.Read(&tag, &ts, &tag_data);
      if ( result == streaming::READ_SKIP ) { continue; }
      if ( result == streaming::READ_EOF ) {
        LOG_INFO << "EOF";
        if ( reader.Remaining() != 0 ) {
          LOG_ERROR << "Leftover data at file end: " << reader.Remaining() << " bytes";
          common::Exit(1);
        }
        break;
      }
      CHECK_EQ(result, streaming::READ_OK);
      CHECK_NOT_NULL(tag.get());
      // because generic tags are disabled, we should receive only F4V tags
      CHECK_EQ(tag->type(), streaming::Tag::TYPE_F4V);

      const streaming::F4vTag& f4v_tag =
          *static_cast<const streaming::F4vTag*>(tag.get());

      if ( !TestClone(f4v_tag) ) {
        LOG_ERROR << "TestClone failed";
        common::Exit(1);
      }

      // check encoding: tag->encode == original data
      if ( !TestEncode(encoder, f4v_tag, tag_data) ) {
        LOG_ERROR << "TestEncode failed";
        common::Exit(1);
      }

      if ( f4v_tag.is_atom() ) {
        const streaming::f4v::BaseAtom* atom = f4v_tag.atom();
        LOG_INFO << "@" << file_pos << " [tag size: " << tag_data.Size() << "]"
                    " Checking atom: " << atom->base_info();
        // check atom->MeasureSize() == atom's encoding size
        if ( !TestMeasureSize(*atom) ) {
          LOG_ERROR << "TestMeasureSize failed";
          common::Exit(1);
        }
        // check encoding: atom->encode->decode == atom
        if ( !TestEncode(*atom) ) {
          LOG_ERROR << "TestEncode failed";
          common::Exit(1);
        }
      } else {
        CHECK(f4v_tag.is_frame());
        const streaming::f4v::Frame* frame = f4v_tag.frame();
        //LOG_INFO << "@" << file_pos << " [tag size: " << tag_data.Size() << "]"
        //            " Checking frame: " << frame->ToString();

        // check frames come in offset order
        if ( prev_frame_header.offset() >= frame->header().offset() ) {
          LOG_ERROR << "Out of order #" << frame_index
                    << " frame: " << frame->header().ToString()
                    << ", prev frame: " << prev_frame_header.ToString()
                    << ", at file_pos: " << file_pos;
          common::Exit(1);
        }

        prev_frame_header = frame->header();
        frame_index++;
      }
    }
    reader.Close();
    LOG_WARNING << "Test1 done";
  }
  */

  ///////////////////////////////////////////////////////////////////////
  // TEST 2:
  //  - read generic frames
  //  - write to a temporary file
  //  - verify that the temporary file contains the same frames
  {
    streaming::MediaFileReader reader;
    if ( !reader.Open(FLAGS_f4v_path) ) {
      LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
      common::Exit(1);
    }
    CHECK_EQ(reader.splitter()->media_format(), streaming::MFORMAT_F4V);
    streaming::f4v::Encoder encoder; // for original tags

    const string tmp_filename = "/tmp/f4v_coder_test_serialized_" + strutil::Basename(FLAGS_f4v_path);
    streaming::MediaFileWriter writer;
    if ( !writer.Open(tmp_filename, streaming::MFORMAT_F4V) ) {
      LOG_ERROR << "Failed to open tmp file: [" << tmp_filename << "]";
      common::Exit(1);
    }

    LOG_WARNING << "Serializing original file into: " << tmp_filename;
    while ( true ) {
      scoped_ref<streaming::Tag> tag;
      int64 ts = 0;

      // read tag
      streaming::TagReadStatus result = reader.Read(&tag, &ts);
      if ( result == streaming::READ_SKIP ) { continue; }
      if ( result == streaming::READ_EOF ) {
        LOG_INFO << "EOF";
        if ( reader.Remaining() != 0 ) {
          LOG_ERROR << "Leftover data at file end: " << reader.Remaining() << " bytes";
          common::Exit(1);
        }
        break;
      }
      CHECK_EQ(result, streaming::READ_OK);
      CHECK_NOT_NULL(tag.get());

      // serialize tag to file
      writer.Write(tag.get(), ts);
    }
    writer.Finalize(NULL, NULL);
    reader.Close();
    LOG_WARNING << "Tmp file done";

    // check serializer's output file is identical to original file
    {
      LOG_WARNING << "Checking serializer Tmp Output file..";
      streaming::MediaFileReader org_reader;
      CHECK(org_reader.Open(FLAGS_f4v_path));
      streaming::MediaFileReader tmp_reader;
      CHECK(tmp_reader.Open(tmp_filename));
      while ( true ) {
        scoped_ref<streaming::Tag> org_tag;
        int64 org_ts = 0;
        io::MemoryStream org_data;
        streaming::TagReadStatus result =
            org_reader.Read(&org_tag, &org_ts, &org_data);
        if ( result == streaming::READ_SKIP ) { continue; }
        if ( result == streaming::READ_EOF ) { break; }
        CHECK_EQ(result, streaming::READ_OK);

        scoped_ref<streaming::Tag> tmp_tag;
        io::MemoryStream tmp_data;
        int64 tmp_ts = 0;
        result = tmp_reader.Read(&tmp_tag, &tmp_ts, &tmp_data);
        CHECK(result == streaming::READ_OK)
            << " Result: " << streaming::TagReadStatusName(result)
            << ", org_tag: " << org_tag->ToString();

        // check that MediaInfo is the same
        if ( org_tag->type() == streaming::Tag::TYPE_MEDIA_INFO ) {
          CHECK_EQ(tmp_tag->type(), streaming::Tag::TYPE_MEDIA_INFO);
          const streaming::MediaInfo& org_info =
              static_cast<streaming::MediaInfoTag*>(org_tag.get())->info();
          const streaming::MediaInfo& tmp_info =
              static_cast<streaming::MediaInfoTag*>(tmp_tag.get())->info();
          if ( org_info.ToString() != tmp_info.ToString() ) {
            LOG_ERROR << "MediaInfo differs: \n"
                      << "Original: " << org_info.ToString() << "\n"
                      << "Output: " << tmp_info.ToString();
            common::Exit(1);
          }
        }

        // check that audio/video frames are the same
        if ( org_tag->is_video_tag() || org_tag->is_audio_tag() ) {
          CHECK_NOT_NULL(org_tag->Data());
          CHECK_EQ(org_tag->is_video_tag(), tmp_tag->is_video_tag());
          CHECK_EQ(org_tag->is_audio_tag(), tmp_tag->is_audio_tag());
          if ( !Compare(*org_tag->Data(), *tmp_tag->Data()) ) {
            LOG_ERROR << "Serializing error:"
                      << "\n - original tag: " << org_tag->ToString()
                      << "\n - original data: " << org_tag->Data()->DumpContentInline()
                      << "\n - serialized tag: " << tmp_tag->ToString()
                      << "\n - serialized data: " << tmp_tag->Data()->DumpContentInline();
            common::Exit(1);
          }
        }
      }
      CHECK_EQ(org_reader.Remaining(), 0);
      org_reader.Close();
      CHECK_EQ(tmp_reader.Remaining(), 0);
      tmp_reader.Close();
      LOG_WARNING << "Tmp Output file is identical to original";
    }
    LOG_WARNING << "Test2 done";
  }

  LOG_WARNING << "Pass";
  common::Exit(0);
}
