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
//
// There are 3 tests in here:
// - for every f4v tag from file:
//     .Decoding + Encoding produces the same data
//     .Decoding + Clone + Encoding produces the same data
// - Decoding in timestamp order verifies the timestamp order
// - Decoding in timestamp order + Serializing in offset order
//     => produces the same file
//

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

#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>

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
  const streaming::f4v::BaseAtom* pclone = atom.Clone();
  scoped_ptr<const streaming::f4v::BaseAtom> auto_del_clone(pclone);
  const streaming::f4v::BaseAtom& clone = *pclone;

  if ( atom.ToString() != clone.ToString() ) {
    LOG_ERROR << "Clone ToString differs: " << endl
              << "# original: " << atom.ToString() << endl
              << "# clone: " << clone.ToString() << endl;
    return false;
  }

  io::MemoryStream atom_out;
  streaming::f4v::Encoder().WriteAtom(atom_out, atom);
  const uint32 atom_data_size = atom_out.Size();
  uint8* atom_data = new uint8[atom_data_size];
  scoped_ptr<uint8> auto_del_atom_data(atom_data);
  atom_out.Read(atom_data, atom_data_size);

  io::MemoryStream clone_out;
  streaming::f4v::Encoder().WriteAtom(clone_out, clone);
  const uint32 clone_data_size = clone_out.Size();
  uint8* clone_data = new uint8[clone_data_size];
  scoped_ptr<uint8> auto_del_clone_data(clone_data);
  clone_out.Read(clone_data, clone_data_size);

  if ( atom_data_size != clone_data_size ||
       ::memcmp(atom_data, clone_data, atom_data_size) != 0 ) {
    LOG_ERROR << "Clone encoding error: " << endl
              << "# atom: " << atom.ToString() << endl
              << "# difference at position: " << DiffIndex(atom_data, atom_data_size, clone_data, clone_data_size) << endl
              //<< "# atom data buffer size: " << atom_data_size << endl
              << "# atom data buffer: "
              << strutil::PrintableDataBuffer(atom_data, atom_data_size) << endl
              //<< "# clone data buffer size: " << clone_data_size << endl;
              << "# clone tag buffer: "
              << strutil::PrintableDataBuffer(clone_data, clone_data_size) << endl;
    return false;
  }
  return true;
}
// Test that frame.Encode() and frame.Clone()->Encode() produce the same output.
bool TestClone(const streaming::f4v::Frame& frame) {
  const streaming::f4v::Frame* pclone = frame.Clone();
  scoped_ptr<const streaming::f4v::Frame> auto_del_clone(pclone);
  const streaming::f4v::Frame& clone = *pclone;

  if ( frame.ToString() != clone.ToString() ) {
    LOG_ERROR << "Clone ToString differs: " << endl
              << "# original: " << frame.ToString() << endl
              << "# clone: " << clone.ToString() << endl;
    return false;
  }

  io::MemoryStream frame_out;
  streaming::f4v::Encoder().WriteFrame(frame_out, frame);
  const uint32 frame_data_size = frame_out.Size();
  uint8* frame_data = new uint8[frame_data_size];
  scoped_ptr<uint8> auto_del_frame_data(frame_data);
  frame_out.Read(frame_data, frame_data_size);

  io::MemoryStream clone_out;
  streaming::f4v::Encoder().WriteFrame(clone_out, clone);
  const uint32 clone_data_size = clone_out.Size();
  uint8* clone_data = new uint8[clone_data_size];
  scoped_ptr<uint8> auto_del_clone_data(clone_data);
  clone_out.Read(clone_data, clone_data_size);

  if ( frame_data_size != clone_data_size ||
       ::memcmp(frame_data, clone_data, frame_data_size) != 0 ) {
    LOG_ERROR << "Clone encoding error: " << endl
              << "# frame: " << frame.ToString() << endl
              << "# difference at position: " << DiffIndex(frame_data, frame_data_size, clone_data, clone_data_size) << endl
              << "# frame data buffer size: " << frame_data_size << endl
              //<< "# expected tag buffer: " << endl
              //<< strutil::PrintableDataBuffer(expected_data, expected_data_size) << endl
              << "# clone data buffer size: " << clone_data_size << endl;
              //<< "# output tag buffer: " << endl
              //<< strutil::PrintableDataBuffer(output_data, output_data_size) << endl;
    return false;
  }
  return true;
}
bool TestClone(const streaming::F4vTag& tag) {
  if ( tag.is_atom() ) {
    return TestClone(*tag.atom());
  } else {
    CHECK(tag.is_frame());
    return TestClone(*tag.frame());
  }

}

// Encode 'tag' and compare output data to 'expected_data'
//
bool TestEncode(streaming::f4v::Encoder& encoder,
                const string& tag_name,
                const streaming::F4vTag& tag,
                const io::MemoryStream& expected_data) {
  // encode tag and get the data buffer
  //
  io::MemoryStream out;
  encoder.WriteTag(out, tag);

  if ( Compare(out, expected_data) ) {
    return true;
  }
  LOG_ERROR << tag_name << " Encoding error: " << endl
            << "# tag: " << tag.ToString() << endl
            << "#encoded as: " << out.DumpContentHex() << endl
            << "#expected: " << expected_data.DumpContentHex();
  return false;
}
// serializes tag using the "serializer" and writes output data into "file"
void SerializeToFile(const streaming::f4v::Tag& tag,
                     streaming::f4v::Serializer& serializer,
                     io::File& file) {
  io::MemoryStream ms;
  bool success = serializer.Serialize(tag, &ms);
  CHECK(success);
  while ( !ms.IsEmpty() ) {
    uint8 tmp[512] = {0};
    int32 read = ms.Read(tmp, sizeof(tmp));
    int32 write = file.Write(tmp, read);
    CHECK_EQ(read, write);
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  if ( FLAGS_f4v_path.empty() ) {
    LOG_ERROR << "Please specify f4v_path parameter !";
    common::Exit(1);
  }
  if ( !io::IsReadableFile(FLAGS_f4v_path) ) {
    LOG_ERROR << "No such file: " << FLAGS_f4v_path;
    common::Exit(1);
  }
  const int64 kMaxFileSize = (1LL << 31); // 2GB
  if ( io::GetFileSize(FLAGS_f4v_path) >= kMaxFileSize ) {
    LOG_ERROR << "File too big: " << FLAGS_f4v_path
              << ", size: " << io::GetFileSize(FLAGS_f4v_path) << "(bytes)"
                 " is larger than kMaxFileSize: " << kMaxFileSize;
    common::Exit(1);
  }

  ///////////////////////////////////////////////////////////////////////
  // TEST 1:
  //  - read frames in offset order
  //  - test offset order
  //  - test encoding: data->decode->encode == data
  //  - test clone: tag->encode == tag->clone->encode for every tag in file
  //                This test should be done with various tags, not just with
  //                file tags.
  {
    streaming::f4v::FileReader reader;
    if ( !reader.Open(FLAGS_f4v_path) ) {
      LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
      common::Exit(1);
    }
    // we want frames in physical offset order
    reader.decoder().set_order_frames_by_timestamp(false);
    streaming::f4v::Encoder original_encoder; // for original tags
    streaming::f4v::Encoder clone_encoder; // for cloned tags

    streaming::f4v::FrameHeader prev_frame_header; // offset 0
    uint32 frame_index = 0;
    while ( true ) {
      scoped_ref<streaming::F4vTag> tag;
      io::MemoryStream tag_data;
      uint64 file_pos = reader.Position();

      // read tag and tag binary data from stream
      streaming::f4v::TagDecodeStatus result = reader.Read(&tag, &tag_data);
      if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
        LOG_ERROR << "Failed to decode tag, at file_pos: " << file_pos;
        common::Exit(1);
      }
      if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
        if ( reader.Remaining() != 0 ) {
          LOG_ERROR << "Leftover data at file end: " << reader.Remaining()
                    << " bytes, at file_pos: " << file_pos;
          common::Exit(1);
        }
        break;
      }
      CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);
      CHECK_NOT_NULL(tag.get());

      if ( tag->is_atom() ) {
        streaming::f4v::BaseAtom* atom = tag->atom();
        LOG_INFO << "@" << file_pos << " [tag size: " << tag_data.Size() << "]"
                    " Checking atom: " << atom->base_info();
      } else {
        CHECK(tag->is_frame());
        streaming::f4v::Frame* frame = tag->frame();
        LOG_INFO << "@" << file_pos << " [tag size: " << tag_data.Size() << "]"
                    " Checking frame: " << frame->ToString();

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

      if ( !TestClone(*tag) ) {
        common::Exit(1);
      }

      // make a clone of the original tag
      //
      scoped_ref<const streaming::F4vTag> clone(
          static_cast<const streaming::F4vTag*>(tag->Clone(-1)));

      // test tag->encode == original data
      // test tag->clone->encode == original data
      if ( !TestEncode(original_encoder, "ORIGINAL", *tag, tag_data) ||
           !TestEncode(clone_encoder, "CLONE", *clone, tag_data) ) {
        common::Exit(1);
      }
    }
  }


  ///////////////////////////////////////////////////////////////////////
  // TEST 2:
  //  - read frames in timestamp order
  //  - verify timestamp order
  //  - verify that f4v::Serializer writes frames in offset order producing
  //    the same output as the original file
  //
  {
    streaming::f4v::FileReader reader;
    if ( !reader.Open(FLAGS_f4v_path) ) {
      LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
      common::Exit(1);
    }
    streaming::f4v::Serializer serializer;
    // the serializer will write the output in this file
    io::File tmp_file;
    const string tmp_filename = "/tmp/f4v_coder_test_serialized_" + strutil::Basename(FLAGS_f4v_path);
    if ( !tmp_file.Open(tmp_filename.c_str(), io::File::GENERIC_READ_WRITE, io::File::CREATE_ALWAYS) ) {
      LOG_ERROR << "Failed to open tmp file: [" << tmp_filename << "]";
      common::Exit(1);
    }

    streaming::f4v::FrameHeader prev_frame_header; // timestamp 0
    uint32 frame_index = 0;
    while ( true ) {
      scoped_ref<streaming::F4vTag> tag;

      // read tag
      streaming::f4v::TagDecodeStatus result = reader.Read(&tag);
      if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
        LOG_ERROR << "Failed to decode tag";
        common::Exit(1);
      }
      if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
        if ( reader.Remaining() != 0 ) {
          LOG_ERROR << "Leftover data at file end: " << reader.Remaining() << " bytes";
          common::Exit(1);
        }
        break;
      }
      CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);
      CHECK_NOT_NULL(tag.get());

      // serialize tag to file
      SerializeToFile(*tag, serializer, tmp_file);

      // check frames come in timestamp order
      if ( tag->is_frame() ) {
        streaming::f4v::Frame* frame = tag->frame();
        LOG_INFO << "#" << frame_index
                 << " Checking frame: " << frame->ToString();

        if ( prev_frame_header.timestamp() > frame->header().timestamp() ) {
          LOG_ERROR << "Out of order #" << frame_index
                    << " frame: " << frame->header().ToString()
                    << ", prev frame: " << prev_frame_header.ToString();
          common::Exit(1);
        }
        prev_frame_header = frame->header();
        frame_index++;
      }
    }
    tmp_file.Close();
    reader.Close();

    // check serializer's output file is identical to original file
    {
      LOG_INFO << "Checking serializer output file..";
      streaming::f4v::FileReader org_reader;
      CHECK(org_reader.Open(FLAGS_f4v_path));
      streaming::f4v::FileReader tmp_reader;
      CHECK(tmp_reader.Open(tmp_filename));
      while ( true ) {
        scoped_ref<streaming::f4v::Tag> org_tag;
        io::MemoryStream org_data;
        streaming::f4v::TagDecodeStatus result =
            org_reader.Read(&org_tag, &org_data);
        if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
          break;
        }
        CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);

        scoped_ref<streaming::f4v::Tag> tmp_tag;
        io::MemoryStream tmp_data;
        result = tmp_reader.Read(&tmp_tag, &tmp_data);
        CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);

        if ( !Compare(org_data, tmp_data) ) {
          LOG_ERROR << "Serializing error, original tag: " << org_tag->ToString()
                    << " differs from serialized tag: " << tmp_tag->ToString();
          common::Exit(1);
        }
      }
      CHECK_EQ(org_reader.Remaining(), 0);
      CHECK_EQ(tmp_reader.Remaining(), 0);
    }
  }


  LOG_WARNING << "Pass";
  common::Exit(0);
}
