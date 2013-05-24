// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Cosmin Tudorache

#ifndef __MEDIA_F4V_F4V_JOINER_H__
#define __MEDIA_F4V_F4V_JOINER_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/joiner.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_file_writer.h>

namespace streaming {

// Joins multiple files into a single big f4v file.
// Ideally, the input file can have any format. However, the tag format is not
// generic (yet), so this function expects FLV input files (for now).
// return: the duration of the output file in milliseconds,
//         or -1 on failure.
//         It return 0 is the files contain only metadata and no media.
int64 JoinFilesIntoF4v(const vector<string>& in_files,
                       const string& out_f4v_file);


}

#endif  // __MEDIA_F4V_F4V_JOINER_H__
