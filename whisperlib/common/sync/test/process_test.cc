// Copyright (c) 2013, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache

#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/sync/process.h>
#include <whisperlib/common/io/file/file_input_stream.h>

#define LOG_TEST LOG(INFO)

DEFINE_string(exe, "", "Simple_executable that:\n"
    " - accepts exactly 3 string arguments\n"
    " - outputs some messages to stdout & stderr, including the given"
    " arguments\n"
    " - produces the same output each time\n"
    " - takes some time in between messages, so we can verify that our"
    " Process() reads the output in realtime");

class ProcessReaderTest {
public:
  ProcessReaderTest(const vector<string>& expected_stdout,
                    const vector<string>& expected_stderr)
    : expected_stdout_(expected_stdout.begin(), expected_stdout.end()),
      expected_stderr_(expected_stderr.begin(), expected_stderr.end()) {
  }
  void HandleStdout(const string& line) {
    LOG_TEST << "HandleStdout('" << line << "')";
    if ( expected_stdout_.empty() ) {
      LOG_ERROR << "Unexpected stdout line: " << line;
      common::Exit(1);
    }
    if ( expected_stdout_.front() != line ) {
      LOG_ERROR << "Expected stdout: [" << expected_stdout_.front() << "]"
                   ", but received: [" << line << "]";
      common::Exit(1);
    }
    expected_stdout_.pop_front();
  }
  void HandleStderr(const string& line) {
    LOG_TEST << "HandleStderr('" << line << "')";
    if ( expected_stderr_.empty() ) {
      LOG_ERROR << "Unexpected stderr line: " << line;
      common::Exit(1);
    }
    if ( expected_stderr_.front() != line ) {
      LOG_ERROR << "Expected stderr: [" << expected_stderr_.front() << "]"
                   ", but received: [" << line << "]";
      common::Exit(1);
    }
    expected_stderr_.pop_front();
  }
  void HandleTerminated(int exit_code) {
    LOG_TEST << "ProcessTerminatedCallback exit_status = " << exit_code;
    if ( !expected_stdout_.empty() || !expected_stderr_.empty() ) {
      LOG_ERROR_IF(!expected_stdout_.empty())
          << "Still expecting some stdout lines: "
          << strutil::ToString(expected_stdout_);
      LOG_ERROR_IF(!expected_stderr_.empty())
          << "Still expecting some stderr lines: "
          << strutil::ToString(expected_stderr_);
      common::Exit(1);
    }
  }
private:
  list<string> expected_stdout_;
  list<string> expected_stderr_;
};

int main(int argc, char** argv) {
  common::Init(argc, argv);

  if ( FLAGS_exe == "" ) {
    LOG_ERROR << "Missing 'exe' parameter";
    common::Exit(1);
  }

  vector<string> args;
  args.push_back("my_first_argument");
  args.push_back("the_123_argument");
  args.push_back("last_argument");

  vector<string> stdout_data;
  vector<string> stderr_data;

  /////////////////////////////////////////
  // Run the executable with stdout/stderr redirected to files.
  // Read the generated files, and fill in stdout_data/stderr_data.
  //
  const string stdout_file = "/tmp/test_stdout.txt";
  const string stderr_file = "/tmp/test_stderr.txt";

  string cmd = FLAGS_exe + " " + strutil::JoinStrings(args, " ") +
               " > " + stdout_file + " 2> " + stderr_file;
  LOG_TEST << "system() command: [" << cmd << "]";
  int result = ::system(cmd.c_str());
  if ( result == -1 ) {
    LOG_ERROR << "system() failed, cmd: [" << cmd << "] error: "
              << GetLastSystemErrorDescription();
    common::Exit(1);
  }
  LOG_TEST << "system() command successful";

  strutil::SplitString(io::FileInputStream::ReadFileOrDie(stdout_file),
      "\n", &stdout_data);
  strutil::SplitString(io::FileInputStream::ReadFileOrDie(stderr_file),
      "\n", &stderr_data);
  /////////////////////////////////////////

  /////////////////////////////////////////
  // Now run the executable through our Process class.
  // Verify stderr/stdout lines (reported through callbacks), against
  // the expected: stdout_data/stderr_data.
  //
  ProcessReaderTest r(stdout_data, stderr_data);

  LOG_TEST << "Running Process: " << FLAGS_exe;
  process::Process p(FLAGS_exe, args, NULL);
  if ( !p.Start(NewPermanentCallback(&r, &ProcessReaderTest::HandleStdout),
                true,
                NewPermanentCallback(&r, &ProcessReaderTest::HandleStderr),
                true,
                NewPermanentCallback(&r, &ProcessReaderTest::HandleTerminated),
                NULL) ) {
    LOG_ERROR << "Failed to start process: " << GetLastSystemErrorDescription();
    common::Exit(1);
  }
  p.Wait(0xffffffff);
  /////////////////////////////////////////

  LOG_TEST << "Pass.";

  common::Exit(0);
}
