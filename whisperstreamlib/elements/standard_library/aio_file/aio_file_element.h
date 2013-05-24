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
// Author: Catalin Popescu

#ifndef __MEDIA_BASE_MEDIA_AIO_FILE_SOURCE_H__
#define __MEDIA_BASE_MEDIA_AIO_FILE_SOURCE_H__

#include <map>
#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/cache.h>
#include <whisperlib/common/io/file/aio_file.h>
#include <whisperlib/common/io/file/buffer_manager.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

///////////////////////////////////////////////////////////////////////
//
// AioFileElement
//
////////////////////////////////////////////////////////////////////////////
//
// Another streaming::Element implementation (and to put it straight, one
// streaming::FileElement implementation).
//
// However this one uses the AIO interface for accessing files.
//
// General note: we are limited by disk seeks per second. We expect that most
// of our media is contiguous on the disk (this shouls be the case if things
// are allright with Linux ext3 fs). SO - if we got, say 1000 concurrent users,
// that read 1000 media files (or parts of them), we need to buffer 10 sec (or
// more) per user in order to do not gulp for each and every of them ..
//
// (fyi. @ 480 kbps -> 60kBps => 600MB / 1000 users => we need to
// read in buffers of 600 kB (one parameter that should get ..>)  ..
// well .. that's it ..
//
// However this means F.R.A.G.M.E.N.T.A.T.I.O.N,
// SO, we need to allocate all buffers BIG, from a free list, and split them
// small, because we do not have an associated free list for memory stream data
// buffer..
// Or add such an instrument :) which is what we do now ..
//
// On another note, most of the code (like element and other stuff..) is silar
// w/ streaming::FileElement.
//

class AioFileReadingStruct;

class AioFileElement : public Element {
 public:
  static const char kElementClassName[];
  AioFileElement(const string& name,
                 ElementMapper* mapper,
                 net::Selector* selector,
                 map<string, io::AioManager*>* aio_managers,
                 io::BufferManager* buf_manager,
                 const string& home_dir,
                 const string& file_pattern,
                 const string& default_index_file,
                 const string& data_key_prefix,
                 bool disable_pause,
                 bool disable_seek,
                 bool disable_duration);

  virtual ~AioFileElement();

  // Element Interface:
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  // Does all the work instead of Close(). Because of loops, Close() is not
  // allowed to send EOS.
  void InternalClose();
  // Every FRS calls this function just before deleting itself.
  void NotifyFrsClosed(AioFileReadingStruct* frs);
  // Media is something like: "a/b/file.flv"
  // Returns: "a/b/file.flv" on success (media matches the internal pattern)
  //          "" on failure (mismatching pattern)
  string FileNameFromMedia(const string& media);

 private:
  // Eveything is synchronized on this thread..
  net::Selector* const selector_;
  // This should be constructed w/ the same selector.. - manages our
  // file acceses
  map<string, io::AioManager*>* const aio_managers_;
  // We read file from under this directory
  const string home_dir_;
  // And we accept only files that match this pattern
  re::RE file_pattern_;
  // If a directory is requested we use this as index file
  const string default_index_file_;
  // If this is not empty we append it to the filename to form the
  // file data key for caching purposes. Else we use the element name.
  const string data_key_prefix_;
  // If true, we disable pausing for files served by us
  const bool disable_pause_;
  // If true, we disable seeking for files served by us
  const bool disable_seek_;
  // If true we disable duration export in-file
  const bool disable_duration_;

  // When not null - closing is upon us..
  Closure* call_on_close_;

  typedef map<const Request*, AioFileReadingStruct*> FileOpMap;
  FileOpMap file_ops_;

  // Allocates buffers for us
  io::BufferManager* const buf_manager_;

  // filename -> MediaInfo*
  util::Cache<string, const MediaInfo*> media_info_cache_;

  // permanent callback to NotifyFrsClosed
  Callback1<AioFileReadingStruct*>* notify_frs_closed_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(AioFileElement);
};
}


#endif  // __MEDIA_BASE_MEDIA_FILE_SOURCE_H__
