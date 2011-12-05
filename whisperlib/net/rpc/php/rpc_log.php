<?php

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
// Author: Cosmin Tudorache

define(LOG_LEVEL_DEBUG, 4);
define(LOG_LEVEL_INFO, 3);
define(LOG_LEVEL_WARNING, 2);
define(LOG_LEVEL_ERROR, 1);
define(LOG_LEVEL_FATAL, 0);

$g_log_level = LOG_LEVEL_INFO;
$g_log_prefix = array("F", "E", "W", "I", "D");

$sapi_type = php_sapi_name();
$g_line_end = ((substr($sapi_type, 0, 3) == 'cgi' ||
                substr($sapi_type, 0, 3) == 'cli') ? "\n" : "<br/>");

function InitializeLog($log_level) {
  global $g_log_level;
  $g_log_level = $log_level;
}

function LOG_LINE($level, $text) {
  global $g_log_level;
  global $g_log_prefix;
  global $g_line_end;
  if ( $level > $g_log_level ) {
    return;
  }
  $prefix = $g_log_prefix[$level];
  echo "[" . $prefix . "] " . $text . $g_line_end;
  if ( $level === LOG_LEVEL_FATAL ) {
    throw new Exception("Fatal error: " . $text);
  }
}

function LOG_DEBUG($text) { LOG_LINE(LOG_LEVEL_DEBUG, $text); }
function LOG_INFO($text) { LOG_LINE(LOG_LEVEL_INFO, $text); }
function LOG_WARNING($text) { LOG_LINE(LOG_LEVEL_WARNING, $text); }
function LOG_ERROR($text) { LOG_LINE(LOG_LEVEL_ERROR, $text); }
function LOG_FATAL($text) { LOG_LINE(LOG_LEVEL_FATAL, $text); }

function CHECK($condition) {
  if ( $condition === true ) {
    return;
  }
  LOG_FATAL("CHECK failed.");
}

function CHECK_EQ($a, $b) {
  if ( $a === $b ) {
    return;
  }
  LOG_FATAL("CHECK_EQ failed: " .  $a . " === " . $b);
}

?>