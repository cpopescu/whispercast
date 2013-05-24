// Copyright (c) 2013, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/sync/process.h>

// #declared in unistd.h
// Points to an array of strings called the 'environment'.
// By convention these strings have the form 'name=value'.

// """
// is initialized as a pointer to an array of character pointers to
// the environment strings. The argv and environ arrays are each
// terminated by a null pointer. The null pointer terminating  the
// argv array is not counted in argc.
// """

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char** environ;
#endif
#include <stdarg.h>

namespace process {

namespace {
void MakeStringVector(const char* const argv[], vector<string>* out) {
  if ( argv ) {
    for ( int i = 0; argv[i] != NULL; i++ ) {
      out->push_back(string(argv[i]));
    }
  }
}
// The result is dynamically allocated, but the elements are referenced;
// just call: delete[] result;
char** MakeCharArray(const char* first, const vector<string>& v) {
  char** out = new char*[v.size() + 2];
  uint32 out_index = 0;
  if ( first != NULL ) {
    out[out_index++] = const_cast<char*>(first);
  }
  for ( uint32 i = 0; i < v.size(); i++ ) {
    out[out_index++] = const_cast<char*>(v[i].c_str());
  }
  out[out_index] = NULL;
  return out;
}
template <typename T>
void RunCallback(Callback1<T>* callback, T result) {
  callback->Run(result);
}
template <typename T>
void RunRefCallback(Callback1<const T&>* callback, T result) {
  callback->Run(result);
}
}

const pid_t Process::kInvalidPid = pid_t(-1);

Process::Process(const string& exe, const vector<string>& argv,
                 const vector<string>* envp)
  : exe_(exe),
    argv_(argv),
    pid_(kInvalidPid),
    exit_status_(0),
    runner_(NULL),
    runner_start_(false, true),
    runner_error_(false),
    stdout_reader_(NULL),
    auto_delete_stdout_reader_(false),
    stderr_reader_(NULL),
    auto_delete_stderr_reader_(false),
    exit_callback_(NULL) {
  MakeStringVector(environ, &envp_);
  if ( envp != NULL ) {
    std::copy(envp->begin(), envp->end(), std::back_inserter(envp_));
  }
}
Process::~Process() {
  Kill();
  delete runner_;
  runner_ = NULL;
  if ( auto_delete_stdout_reader_ ) {
    delete stdout_reader_;
    stdout_reader_ = NULL;
  }
  if ( auto_delete_stderr_reader_ ) {
    delete stderr_reader_;
    stderr_reader_ = NULL;
  }
}

bool Process::Start(Callback1<const string&>* stdout_reader,
                    bool auto_delete_stdout_reader,
                    Callback1<const string&>* stderr_reader,
                    bool auto_delete_stderr_reader,
                    Callback1<int>* exit_callback,
                    net::Selector* selector) {
  CHECK_NULL(runner_) << " Duplicate Start()";

  stdout_reader_ = stdout_reader;
  if ( stdout_reader_ != NULL ) {
    CHECK(stdout_reader_->is_permanent());
    auto_delete_stdout_reader_ = auto_delete_stdout_reader;
  }

  stderr_reader_ = stderr_reader;
  if ( stderr_reader_ != NULL ) {
    CHECK(stderr_reader_->is_permanent());
    auto_delete_stderr_reader_ = auto_delete_stderr_reader;
  }

  exit_callback_ = exit_callback;
  selector_ = selector;

  runner_ = new thread::Thread(NewCallback(this, &Process::Runner));
  runner_->Start();
  if ( !runner_start_.Wait(5000) ) {
    LOG_ERROR << "Runner() thread failed to start";
    return false;
  }
  return !runner_error_;
}

bool Process::Signal(int signum) {
  LOG_ERROR << "TODO(cosmin): implement";
  return false;
}

// Kills the process.
void Process::Kill() {
  if ( !IsRunning() ) { return; }
  if ( ::kill(-pid_, SIGKILL) != 0 ) {
    LOG_ERROR << "::kill(" << pid_ << ", SIGKILL) failed"
              << ", error=" << GetLastSystemErrorDescription();
  } else {
    LOG_WARNING << "Process [pid=" << pid_ << "] killed.";
  }
  pid_ = kInvalidPid;
}

// Detaches from the executing process. So you can delete this
// object and the process keeps running.
void Process::Detach() {
  LOG_ERROR << "TODO(cosmin): implement";
}

bool Process::Wait(uint32 timeout_ms, int* exit_status) {
  if ( !IsRunning() ) {
    return false;
  }
  CHECK_NOT_NULL(runner_);
  runner_->Join();
  if ( exit_status != NULL ) {
    *exit_status = exit_status_;
  }
  return true;
}

bool Process::IsRunning() const {
  return pid_ != kInvalidPid;
}

namespace {
struct Pipe {
  Pipe() : read_fd_(INVALID_FD_VALUE), write_fd_(INVALID_FD_VALUE) {}
  ~Pipe() { Close(); }
  bool Open() {
    int p[2] = {0,};
    if ( 0 != ::pipe(p) ) {
      LOG_ERROR << "::pipe() failed: " << GetLastSystemErrorDescription();
      return false;
    }
    read_fd_ = p[0];
    write_fd_ = p[1];
    return true;
  }
  void Close() {
    CloseRead();
    CloseWrite();
  }
  void CloseRead() {
    if ( read_fd_ != INVALID_FD_VALUE ) {
      if ( -1 == ::close(read_fd_) ) {
        LOG_ERROR << "::close failed: " << GetLastSystemErrorDescription();
      }
      read_fd_ = INVALID_FD_VALUE;
    }
  }
  void CloseWrite() {
    if ( write_fd_ != INVALID_FD_VALUE ) {
      if ( -1 == ::close(write_fd_) ) {
        LOG_ERROR << "::close failed: " << GetLastSystemErrorDescription();
      }
      write_fd_ = INVALID_FD_VALUE;
    }
  }
  bool IsReadOpen() const { return read_fd_ != INVALID_FD_VALUE; }
  bool IsWriteOpen() const { return write_fd_ != INVALID_FD_VALUE; }
  bool IsOpen() const { return IsReadOpen() || IsWriteOpen(); }
  int read_fd_;
  int write_fd_;
} __attribute__((__packed__));
}

void Process::Runner() {
  // Create simple pipes for stdout & stderr.
  // The child is going to write to them, the parent is going to read them.
  Pipe stdout_pipe, stderr_pipe;
  if ( (stdout_reader_ != NULL && !stdout_pipe.Open()) ||
       (stderr_reader_ != NULL && !stderr_pipe.Open()) ) {
    runner_error_ = true;
    runner_start_.Signal();
    return;
  }

  LOG_WARNING << "Executing process: " << exe_
              << ", argv: " << strutil::ToString(argv_);

  pid_ = ::fork();
  if ( pid_ == 0 ) {
    ////////////////////////////////
    // child process
    ::setsid(); // set us as a process group leader
                // (kill us -> kill all under us)
    // close pipes read end
    stdout_pipe.CloseRead();
    stderr_pipe.CloseRead();
    // redirect stdout & stderr to pipe's write end
    if ( (stdout_pipe.IsWriteOpen() && -1 == ::dup2(stdout_pipe.write_fd_, 1)) ||
         (stderr_pipe.IsWriteOpen() && -1 == ::dup2(stderr_pipe.write_fd_, 2)) ) {
      LOG_ERROR << "::dup2 failed: " << GetLastSystemErrorDescription();
      common::Exit(1);
    }

    // execute the external program
    scoped_array<char*> argv(MakeCharArray(exe_.c_str(), argv_));
    scoped_array<char*> envp(MakeCharArray(NULL, envp_));
    int result = ::execve(exe_.c_str(), argv.get(), envp.get());
    LOG_ERROR << "::execve(), exe: [" << exe_ << "]"
                 ", failed: " << GetLastSystemErrorDescription();
    _exit(result);
  }
  ////////////////////////////////
  // parent process
  runner_error_ = false;
  runner_start_.Signal();

  // close pipes write end
  stdout_pipe.CloseWrite();
  stderr_pipe.CloseWrite();

  // read pipes until EOF
  io::MemoryStream stdout_ms;
  io::MemoryStream stderr_ms;
  while ( true ) {
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    fd_set input;
    FD_ZERO(&input);
    int maxfd = 0;
    if ( stdout_pipe.IsOpen() ) {
      FD_SET(stdout_pipe.read_fd_, &input);
      maxfd = std::max(maxfd, stdout_pipe.read_fd_);
    }
    if ( stderr_pipe.IsOpen() ) {
      FD_SET(stderr_pipe.read_fd_, &input);
      maxfd = std::max(maxfd, stderr_pipe.read_fd_);
    }
    if ( maxfd == 0 ) {
      // both pipes are closed
      break;
    }

    const int n = ::select(1 + maxfd, &input, NULL, NULL, &timeout);
    if ( n < 0 ) {
      LOG_ERROR << "::select failed: " << GetLastSystemErrorDescription();
      Kill();
      RunExitCallback(1);
      return;
    }
    if ( n == 0 ) {
      // timeout
      continue;
    }
    char read_buf[256];
    // read child's stdout
    if ( stdout_pipe.IsReadOpen() && FD_ISSET(stdout_pipe.read_fd_, &input) ) {
      ssize_t size = ::read(stdout_pipe.read_fd_, read_buf, sizeof(read_buf));
      if ( size == -1 ) {
        LOG_ERROR << "::read failed: " << GetLastSystemErrorDescription();
        Kill();
        RunExitCallback(1);
        return;
      }
      if ( size == 0 ) {
        // EOF
        stdout_pipe.Close();
      }
      stdout_ms.Write(read_buf, size);
    }
    // read child's stderr
    if ( stderr_pipe.IsReadOpen() && FD_ISSET(stderr_pipe.read_fd_, &input) ) {
      ssize_t size = ::read(stderr_pipe.read_fd_, read_buf, sizeof(read_buf));
      if ( size == -1 ) {
        LOG_ERROR << "::read failed: " << GetLastSystemErrorDescription();
        Kill();
        RunExitCallback(1);
        return;
      }
      if ( size == 0 ) {
        // EOF
        stderr_pipe.Close();
      }
      stderr_ms.Write(read_buf, size);
    }

    // report lines
    string line;
    while ( stdout_ms.ReadLine(&line) ) { RunStdoutReader(line); }
    while ( stderr_ms.ReadLine(&line) ) { RunStderrReader(line); }
  }

  // wait for child to terminate (since we received EOF on both pipes,
  //  it should have already ended)
  int status = 0;
  const pid_t result = ::waitpid(pid_, &status, 0);
  if ( result == -1 ) {
    LOG_ERROR << "::waitpid [pid=" << pid_ << "] failed: "
              << GetLastSystemErrorDescription();
    Kill();
    RunExitCallback(1);
    return;
  }
  CHECK_EQ(result, pid_);
  exit_status_ = WEXITSTATUS(status);
  LOG_WARNING << "Process [pid=" << pid_ << "] terminated: exe_='"
              << exe_ << "', exit_status_=" << exit_status_;
  pid_ = kInvalidPid;

  RunExitCallback(exit_status_);
}
void Process::RunExitCallback(int exit_code) {
  if ( exit_callback_ == NULL ) {
    return;
  }
  if ( selector_ != NULL ) {
    selector_->RunInSelectLoop(
        NewCallback(&RunCallback, exit_callback_, exit_code));
    return;
  }
  exit_callback_->Run(exit_code);
}
void Process::RunStdoutReader(const string& line) {
  if ( stdout_reader_ == NULL ) { return; }
  if ( selector_ != NULL ) {
    selector_->RunInSelectLoop(
        NewCallback(&RunRefCallback, stdout_reader_, line));
    return;
  }
  stdout_reader_->Run(line);
}
void Process::RunStderrReader(const string& line) {
  if ( stderr_reader_ == NULL ) { return; }
  if ( selector_ != NULL ) {
    selector_->RunInSelectLoop(
        NewCallback(&RunRefCallback, stderr_reader_, line));
    return;
  }
  stderr_reader_->Run(line);
}

}
