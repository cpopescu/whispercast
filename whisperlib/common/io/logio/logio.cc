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

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/logio/logio.h>
#include <whisperlib/common/io/ioutil.h>

namespace {
//////////////////////////////////////////////////////////////////////

const string ComposeFileName(const string& log_dir,
                             const string& file_base,
                             int32 block_size,
                             int32 file_num) {
  return strutil::StringPrintf("%s/%s_%010d_%010d",
      log_dir.c_str(), file_base.c_str(), block_size, file_num);
}
}

//////////////////////////////////////////////////////////////////////

namespace io {

LogWriter::LogWriter(const string& log_dir,
                     const string& file_base,
                     int32 block_size,
                     int32 blocks_per_file,
                     bool temporary_incomplete_file,
                     bool deflate)
  : log_dir_(log_dir),
    file_base_(file_base),
    block_size_(block_size),
    blocks_per_file_(blocks_per_file),
    temporary_incomplete_file_(temporary_incomplete_file),
    deflate_(deflate),
    re_(strutil::StringPrintf(
        "^%s_%010d_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$",
        file_base_.c_str(), block_size)),
    file_num_(-1),
    recorder_(block_size, deflate) {
  CHECK(io::IsDir(log_dir)) << " No a directory: [" << log_dir << "]";
}
LogWriter::~LogWriter() {
  Close();
  CHECK(buf_.IsEmpty());
  CHECK_EQ(recorder_.leftover(), 0);
}

bool LogWriter::Initialize() {
  //////////////////////////////////////////////////////////////////////
  // exclusion mechanism, to prevent multiple LogWriters from writing
  // the same file:
  // - create a new .lock file, write the pid inside => SUCCESS
  // - if the .lock file exists, read the pid inside:
  //      .if the pid process is running => FAIL
  //      .else: overwrite the lock file => SUCCESS
  // NOT Race proof!! Nevertheless protective.
  string lock_filename = strutil::JoinPaths(log_dir_, file_base_ + ".lock");
  io::File lock_file;
  if ( !lock_file.Open(lock_filename,
                       io::File::GENERIC_READ_WRITE,
                       io::File::OPEN_ALWAYS) ) {
    LOG_ERROR << "Failed to open lock file: [" << lock_filename << "]";
    return false;
  }
  char tmp[64] = {0,};
  lock_file.Read(tmp, sizeof(tmp)-1);
  pid_t pid = ::atoi(tmp);
  if ( pid != 0 ) {
    if ( io::Exists(strutil::StringPrintf("/proc/%d/cmdline", pid)) ) {
      LOG_ERROR << "LogWriter already in use! by pid: " << pid
                << " , lock file: " << lock_filename;
      return false;
    }
  }
  // write current process pid
  lock_file.SetPosition(0, io::File::FILE_SET);
  string crt_pid = strutil::StringPrintf("%d", getpid());
  lock_file.Write(crt_pid.c_str(), crt_pid.size());
  lock_file.Truncate();
  lock_file.Close();
  DLOG_DEBUG << "Created lock file: [" << lock_filename << "]";
  /////////////////////////////////////////////////////////////////////

  file_num_ = io::GetLastNumberedFile(log_dir_, &re_, 10);
  if ( file_num_ == -2 ) {
    LOG_ERROR << "io::GetLastNumberedFile failed for log_dir: [" << log_dir_
              << "], and re_: " << re_.regex();
    return false;
  }
  if ( file_num_  == -1 ) {
    file_num_ = 0;
  }
  if ( temporary_incomplete_file_ ) {
    string tmp_dir = strutil::JoinPaths(log_dir_, "temp");
    if ( !io::Mkdir(tmp_dir) ) {
      LOG_ERROR << "Failed to create temporary directory: [" << tmp_dir << "]";
      return false;
    }
  }

  return OpenNextLog();
}

bool LogWriter::WriteRecord(io::MemoryStream* in) {
  if ( !file_.is_open() && !OpenNextLog() ) {
    return false;
  }
  if ( recorder_.AppendRecord(in, &buf_) ) {
    return WriteBuffer(false);
  }
  return true;
}

bool LogWriter::WriteRecord(const char* buffer, int32 size) {
  if ( !file_.is_open() && !OpenNextLog() ) {
    return false;
  }
  if ( recorder_.AppendRecord(buffer, size, &buf_) ) {
    return WriteBuffer(false);
  }
  return true;
}

bool LogWriter::Flush() {
  recorder_.FinalizeContent(&buf_);
  if ( buf_.IsEmpty() ) {
    return true;
  }
  return WriteBuffer(true);
}

string LogWriter::ComposeFileName(bool temp) const {
  CHECK_GE(file_num_, 0);
  return ::ComposeFileName(temp ? strutil::JoinPaths(log_dir_, "temp")
                                : log_dir_,
                           file_base_, block_size_, file_num_);
}

LogPos LogWriter::Tell() const {
  CHECK(file_.is_open()) << "Did you Initialize()?";
  CHECK(file_.Position() % block_size_ == 0) << "Illegal file position: "
      << file_.Position() << ", block_size_: " << block_size_;
  if ( file_.Position()  == blocks_per_file_ * block_size_ ) {
    // current file is full
    return LogPos(file_num_ + 1, 0, recorder_.PendingRecordCount());
  }
  return LogPos(file_num_, file_.Position() / block_size_,
                recorder_.PendingRecordCount());
}

void LogWriter::Close() {
  if ( !file_.is_open() ) {
    return;
  }
  // finalize recorder
  recorder_.FinalizeContent(&buf_);
  WriteBuffer(false);

  // close last file
  CloseLog();

  // remove lock file
  io::Rm(strutil::JoinPaths(log_dir_, file_base_ + ".lock"));
}

//////////////////////////////////////////////////////////////////////

bool LogWriter::WriteBuffer(bool force_flush) {
  CHECK(file_.is_open());

  while ( !buf_.IsEmpty() ) {
    // check that the file is correct
    int64 pos = file_.Position();
    if ( pos % block_size_ != 0 ||
         pos > block_size_ * blocks_per_file_ ) {
      LOG_ERROR << "Bad file size: " << pos
                << ", block_size_: " << block_size_
                << ", blocks_per_file_: " << blocks_per_file_;
      return false;
    }
    CHECK(pos < block_size_ * blocks_per_file_) << "File already full";

    // check that the buf_ is correct
    CHECK(buf_.Size() % block_size_ == 0)
      << endl << " - buf_.Size(): " << buf_.Size()
      << endl << " - block_size_: " << block_size_;

    // WARNING!!!: buf_ may contain more than 1 block! Don't write blindly
    //             the whole buf_ to current file. Instead: possibly split buf_
    //             between current & next file.

    // write to file
    buf_.MarkerSet();
    const int to_write = min(block_size_ * blocks_per_file_ - (int32)pos,
                             (int32)buf_.Size());
    const int cb = file_.Write(buf_, to_write);
    if ( cb < 0 ) {
      LOG_ERROR << "Write failed, restoring data.";
      buf_.MarkerRestore();
      return false;
    }
    if ( cb != to_write ) {
      // restore buffer
      buf_.MarkerRestore();
      // cut off the written data
      file_.Truncate(pos);
      LOG_ERROR << "Write partial failed, written: " << cb << " out of "
                << to_write << ", remaining in buf_: " << buf_.Size();
      return false;
    }
    buf_.MarkerClear();

    // maybe go to the next log file
    if ( file_.Position() == block_size_ * blocks_per_file_ ) {
      LOG_INFO << "Log file complete: [" << file_name() << "]"
                  ", creating a new log";
      if ( !OpenNextLog() ) {
        LOG_ERROR << "OpenNextLog failed";
        return false;
      }
    }
  }

  // flush last file, previous files were already closed.. no need to flush
  if ( force_flush ) {
    file_.Flush();
  }

  return true;
}

bool LogWriter::OpenNextLog() {
  CHECK(file_num_ >= 0) << " LogWriter not initialized";
  if ( file_.is_open() ) {
    CloseLog();
    ++file_num_;
  }
  const string filename = ComposeFileName(temporary_incomplete_file_);
  if ( io::IsReadableFile(filename) ) {
    DLOG_INFO << "Continue log file: " << filename
              << " size: " << io::GetFileSize(filename);
  } else {
    DLOG_INFO << "New log file: " << filename;
  }
  if ( !file_.Open(filename, io::File::GENERIC_WRITE, io::File::OPEN_ALWAYS) ) {
    LOG_ERROR << "Error opening file : [" << filename << "]";
    return false;
  }
  if ( file_.Size() % block_size_ != 0 ) {
    LOG_ERROR << "Invalid file size: " << file_.Size() << " truncating to "
                 "the closest multiple of block_size: " << block_size_
              << ", possible data loss..";
    file_.Truncate((file_.Size() / block_size_) * block_size_);
  }
  CHECK(file_.Size() % block_size_ == 0) << " Hmm, file size should be correct";
  if ( file_.SetPosition(0, io::File::FILE_END) == -1 ) {
    LOG_ERROR << "Error seeking to file end, for file : [" << filename << "]";
    return false;
  }

  // maybe go to the next log file
  if ( file_.Position() == block_size_ * blocks_per_file_ ) {
    return OpenNextLog();
  }

  return true;
}

void LogWriter::CloseLog() {
  if ( !file_.is_open() ) {
    return;
  }
  string filename = file_.filename();

  // close file
  file_.Close();
  if ( temporary_incomplete_file_ ) {
    const string final_file_name = ComposeFileName(false);
    if ( final_file_name != filename ) {
      if ( !io::Rename(filename, final_file_name, false) ) {
        LOG_ERROR << "Error renaming temporary file: [" << filename << "]"
                     " to: [" << final_file_name << "]";
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////


LogReader::LogReader(const string& log_dir,
                     const string& file_base,
                     int32 block_size,
                     int32 blocks_per_file)
  : log_dir_(log_dir),
    file_base_(file_base),
    re_(strutil::StringPrintf(
        "^%s_%010d_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$",
        file_base_.c_str(), block_size)),
    block_size_(block_size),
    blocks_per_file_(blocks_per_file),
    file_(),
    file_num_(-1),
    record_num_(0),
    reader_(block_size),
    num_errors_(0),
    buf_() {
}

LogReader::~LogReader() {
  CloseInternal(false);
}

bool LogReader::Seek(const LogPos& log_pos) {
  if ( log_pos.IsNull() ) {
    Rewind();
    return true;
  }
  if ( log_pos.block_num_ == 0 && log_pos.record_num_ == 0 &&
       log_pos.file_num_ > 0 &&
       io::IsReadableFile(ComposeFileName(log_pos.file_num_ - 1)) ) {
    // seeking to the beginning of a file. The previous file exists or current
    // file is 0. Current file may not exist (i.e. log_pos is log end)
    file_num_ = log_pos.file_num_;
    record_num_ = 0;
    return true;
  }
  if ( !OpenFile(log_pos.file_num_) ) {
    LOG_ERROR << "Seek failed, cannot open file_num: " << log_pos.file_num_;
    return false;
  }
  if ( !AdvanceToPosInFile(log_pos) ) {
    LOG_ERROR << "Seek failed, cannot advance to pos: " << log_pos.ToString();
    return false;
  }
  return true;
}

bool LogReader::GetNextRecord(io::MemoryStream* out) {
  // if pos_.file_num_ == -1 it means we're not initialized. Rewind will
  // search for the first file (not necessarily file_num 0, it could start
  // with e.g. 7)
  if ( file_num_ == -1 ) {
    Rewind();
    // still cannot find first file? => fail
    if ( file_num_ == -1 ) {
      return false;
    }
  }
  // current file not open? re-try open. This situation happens when we read
  // all existing files, and next file does not exist yet.
  if ( !file_.is_open() &&
       !OpenFile(file_num_) ) {
    return false;
  }
  while ( true ) {
    RecordReader::ReadResult res = reader_.ReadRecord(&buf_, out);
    if ( res == RecordReader::READ_NO_DATA ) {
      // go to next block
      record_num_ = 0;
      if ( !ReadBlock() ) {
        // current file has no more data, or next file does not exist
        return false;
      }
      continue;
    }
    if ( res != RecordReader::READ_OK ) {
      LOG_ERROR << "Log error: " << RecordReader::ReadResultName(res) << ", in ["
                << file_.filename() << "], pos: " << Tell().ToString();
      num_errors_++;
    }
    record_num_++;
    return true;
  }
  return false;
}

void LogReader::Rewind() {
  // go to uninitialized state (file_num = -1)
  CloseInternal(false);

  // scan for files
  vector<string> files;
  // TODO(cpopescu): well .. this is almost there ..
  //                 as we don't count in the block_size
  if ( !DirList(log_dir_, LIST_FILES, &re_, &files) ) {
    LOG_ERROR << "DirList() failed for dir: [" << log_dir_ << "]";
    return;
  }
  if ( files.empty() ) {
    // Not an error though ..
    LOG_INFO << "No suitable files in log_dir_: [" << log_dir_ << "]"
                ", file_base_: [" << file_base_ << "]";
    return;
  }
  sort(files.begin(), files.end());

  // The RE should enforce the format: <base>_??????????_??????????
  CHECK_GT(files.front().size(), 10);

  // set current file_num.
  file_num_ = ::strtol(
    files.front().c_str() + files.front().size() - 10, NULL, 10);
  // The RE should make sure that the last 10 chars are digits.
  CHECK_GE(file_num_, 0);

  // it's not necessary to OpenFile() here. The next GetNextRecord() will do it.
}

string LogReader::ComposeFileName(int32 file_num) const {
  CHECK_GE(file_num, 0);
  return ::ComposeFileName(log_dir_, file_base_, block_size_, file_num);
}

void LogReader::CloseInternal(bool on_error) {
  file_.Close();
  file_num_ = -1;
  record_num_ = 0;
  if ( on_error ) {
    num_errors_++;
  }
  reader_.Clear();
  buf_.Clear();
}

bool LogReader::OpenFile(int32 file_num) {
  // go to uninitialized state
  CloseInternal(false);
  // file_num_ is set to current file. If we don't succeed, next
  // GetNextRecord() will try again to open this file..
  file_num_ = file_num;
  const string filename = ComposeFileName(file_num);
  // early check file existence (just to avoid the LOG_ERROR in file_.Open())
  if ( !io::IsReadableFile(filename) ) {
    LOG_DEBUG << "Cannot open, no such file: [" << filename << "]";
    return false;
  }
  if ( !file_.Open(filename, io::File::GENERIC_READ,
                             io::File::OPEN_EXISTING ) ) {
    LOG_ERROR << "Error opening file : [" << filename << "]";
    return false;
  }
  CHECK_EQ(file_.Position(), 0);
  return true;
}

bool LogReader::AdvanceToPosInFile(const LogPos& pos) {
  CHECK(file_.is_open());
  CHECK_EQ(file_num_, pos.file_num_);

  reader_.Clear();
  buf_.Clear();

  // seek to current block, we read from this position
  int64 seek_pos = pos.block_num_ * block_size_;
  if ( file_.SetPosition(seek_pos, io::File::FILE_SET) == -1 ) {
    LOG_ERROR << "Cannot seek to " << seek_pos;
    return false;
  }

  // read the block containing the seek, only if record_num > 0
  // (if record_num == 0, the block may not exist yet, which is not
  // a seek error)
  if ( pos.record_num_ == 0 ) {
    return true;
  }
  if ( !ReadBlock() ) {
    LOG_ERROR << "Cannot seek, failed to read the block containing"
                 " the seek record";
    return false;
  }
  CHECK_EQ(buf_.Size(), block_size_);
  // skip records, until current record_num
  record_num_ = 0;
  while ( record_num_ < pos.record_num_ ) {
    RecordReader::ReadResult res = reader_.ReadRecord(&buf_, NULL);
    if ( res != RecordReader::READ_OK ) {
      LOG_ERROR << "Cannot seek to record: " << pos.record_num_
                << ", error: " << RecordReader::ReadResultName(res)
                << ", at record: " << record_num_;
      return false;
    }
    record_num_++;
  }
  return true;
}

bool LogReader::ReadBlock() {
  CHECK(file_.is_open());
  if ( file_.Position() == block_size_ * blocks_per_file_ &&
       !OpenFile(file_num_ + 1) ) {
    return false;
  }
  CHECK(buf_.IsEmpty());
  const int64 file_pos = file_.Position();
  const int32 cb = file_.Read(&buf_, block_size_);
  if ( cb < block_size_ ) {
    // restore file pointer & clear partial read from buf_
    file_.SetPosition(file_pos, io::File::FILE_SET);
    buf_.Clear();
    LOG_DEBUG << "Not enough data for the next block"
              << ", in file: " << file_.filename()
              << ", at position: " << file_.Position()
              << ", last read: " << cb << " is less than 1 block size: "
              << block_size_;
    return false;
  }
  CHECK_EQ(buf_.Size(), block_size_);
  return true;
}

//////////////////////////////////////////////////////////////////////

uint32 CleanLog(const string& log_dir, const string& file_base,
                LogPos first_pos, int32 block_size) {
  re::RE re(strutil::StringPrintf("^%s_%010d"
      "_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$",
      file_base.c_str(), block_size));
  vector<string> files;
  if ( !DirList(log_dir, LIST_FILES, &re, &files) ) {
    return 0;
  }
  if ( files.empty() ) {
    return 0;
  }
  sort(files.begin(), files.end());

  uint32 num_deleted = 0;
  for ( int32 i = 0; i < files.size(); ++i ) {
    CHECK_GT(files[i].size(), 10);
    errno = 0;  // essential as strtol would not set a 0 errno
    const int32 file_num = strtol(files[i].c_str() + files[i].size() - 10,
                                  NULL, 10);
    if ( errno || file_num < 0 ) {
      LOG_ERROR << "FileNum: " << file_num <<  " for [" << files[i] << "]";
      continue;
    }
    if ( file_num >= first_pos.file_num_ ) {
      return num_deleted;
    }
    const string filename = ComposeFileName(log_dir, file_base,
                                            block_size, file_num);
    if ( !io::Rm(filename) ) {
      LOG_ERROR << "Cannot delete file: [" << filename << "]";
      continue;
    }
    ++num_deleted;
  }
  return num_deleted;
}

bool DetectLogSettings(const string& log_dir, string* out_file_base,
    int32* out_block_size, int32* out_blocks_per_file) {
  // NOTE: returned filenames are relative to dir !!
  vector<string> files;
  re::RE re("_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]"
            "_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$");
  if ( !io::DirList(log_dir, LIST_FILES, &re, &files) ) {
    LOG_ERROR << "Cannot list directory: [" << log_dir << "]";
    return false;
  }
  sort(files.begin(), files.end());
  if ( files.empty() ) {
    LOG_INFO << "No files found";
    return false;
  }

  // detect file_base
  string filename = files[0];
  CHECK(filename.size() >= 22) << " For file: [" << filename << "]";
  *out_file_base = filename.substr(0, filename.size() - 22);

  // detect block_size
  string str_block_size = filename.substr(filename.size() - 21, 10).c_str();
  *out_block_size = ::atoi(str_block_size.c_str());
  if ( *out_block_size == 0 ) {
    LOG_ERROR << "Failed to detect block size, from file: [" << filename << "]";
    return false;
  }

  // detect blocks_per_file
  int64 filesize = io::GetFileSize(strutil::JoinPaths(log_dir, filename));
  CHECK(filesize > 0) << "Failed to get file size"
                         ", for file: [" << filename << "]"
                         ", err: " << GetLastSystemErrorDescription();

  if ( filesize % (*out_block_size) != 0 ) {
    LOG_ERROR << "Erroneous file size, file: [" << filename
              << "], size: " << filesize << " is not a multiple of"
                 " block_size: " << (*out_block_size);
  }
  *out_blocks_per_file = filesize / (*out_block_size);
  if ( *out_blocks_per_file == 0 ) {
    LOG_ERROR << "Cannot detect blocks_per_file, from file: [" << filename
              << "], block_size: " << (*out_block_size);
    return false;
  }
  return true;
}

}
