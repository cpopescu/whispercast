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

#include "random_creators.h"

int Random() {
  return ::rand();
}

bool RandomBool() {
  return (Random() % 2) == 0;
}

float RandomFloat() {
  int nRand = Random();
  return 1.0f * (0x7fffffff - nRand);
}

signed long long Random64() {
  const signed long long nRand1 = Random();
  const signed long long nRand2 = Random();
  return (nRand1 << 32) | nRand2;
}
std::string RandomString(unsigned nSize) {
  if (nSize == 0) {
    return std::string("");
  }

  char* szRand = new char[nSize+1];
  CHECK(szRand);
  for (unsigned i = 0; i < nSize; i++) {
    szRand[i] = ' ' + (Random() % ('}' - ' '));
  }
  szRand[nSize] = 0; // make NULL end
  const std::string strRand(szRand);
  delete szRand;
  return strRand;
}

vector<int> RandomArrayInt(unsigned size) {
  vector<int> array(size);
  for (unsigned i = 0; i < size; i++) {
    array[i] = Random();
  }
  return array;
}

A RandomA() {
  A a;
  a.value_ = RandomFloat();
  return a;
}

B RandomB() {
  B b;
  b.bBool_ = RandomBool();
  const unsigned nBSize = (Random() % 5);
  vector<string>& names = b.names_.ref();
  names.resize(nBSize);
  for (unsigned i = 0; i < nBSize; i++) {
    names[i] = RandomString(128);
  }
  return b;
}
