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
//
#include <string.h>
#include "whisperlib/common/base/log.h"

static const double kSqrt2 = 1.4142135623730951;
static const double kSqrt3 = 1.7320508075688772;

static const double kDaubH0 = (1.0 + kSqrt3) / (4 * kSqrt2);
static const double kDaubH1 = (3.0 + kSqrt3) / (4 * kSqrt2);
static const double kDaubH2 = (3.0 - kSqrt3) / (4 * kSqrt2);
static const double kDaubH3 = (1.0 - kSqrt3) / (4 * kSqrt2);

static const double kDaubG0 = kDaubH3;
static const double kDaubG1 = -kDaubH2;
static const double kDaubG2 = kDaubH1;
static const double kDaubG3 = -kDaubH0;

//////////////////////////////////////////////////////////////////////
//
// Haar
//
static void Haar1WaveletStep(double* x, int n, double* temp_temp) {
  CHECK_EQ(n & 1, 0);
  n >>= 1;
  for ( int i = 0; i < n; ++i) {
    temp_temp[i] = (x[2 * i] + x[2 * i + 1]) / kSqrt2;
    temp_temp[i + n] = (x[2 * i] - x[2 * i + 1]) / kSqrt2;
  }
  memcpy(x, temp_temp, 2 * n * sizeof(double));
}

void Haar1Wavelet(double* x, int n) {
  double* temp = new double[n];
  int w = n;
  while ( w > 1 ) {
    if ( (w & 1) == 1 ) {
      x[w-1] = (x[w-2] - x[w-1]) / kSqrt2;
      --w;
    }
    Haar1WaveletStep(x, n, temp);
    w >>= 1;
  }
  delete [] temp;
}

void Haar2Wavelet(double* x, int width, int height) {
  double* temp_col = new double[height];
  double* temp_temp = new double[width > height ? width : height];

  int w = width;
  int h = height;
  int* col_ndx = new int[height];
  for ( int i = 0; i < height; ++i ) {
    col_ndx[i] = width * i;
  }
#define X(i, j) \
  x[col_ndx[(j)] + (i)]

  while ( w > 1 || h > 1 ) {
    if ( w > 1 && w >= h ) {
      if ( (w & 1) == 1 ) {
        // Put a difference in the odd place ..
        for ( int i = 0; i < h; ++i ) {
          X(w-1, i) = (X(w-2, i) - X(w-1, i))  / kSqrt2;
        }
        --w;
      }
      for ( int i = 0; i < h; ++i ) {
        Haar1WaveletStep(&X(0, i), w, temp_temp);
      }
      w >>= 1;
    } else if ( h > 1 && h >= w ) {
      if ( (h & 1) == 1 ) {
        // Put a difference in the odd place ..
        for ( int i = 0; i < w; ++i ) {
          X(i, h-1) = (X(i, h -2) - X(i, h-1)) / kSqrt2;
        }
        --h;
      }
      for ( int i = 0; i < w; ++i ) {
        for ( int j = 0; j < h; ++j ) {
          temp_col[j] = X(i, j);
        }
        Haar1WaveletStep(temp_col, h, temp_temp);
        for ( int j = 0; j < h; ++j ) {
          X(i, j) = temp_col[j];
        }
      }
      h >>= 1;
    }
  }
#undef X
  delete [] col_ndx;
  delete [] temp_col;
  delete [] temp_temp;
}

//////////////////////////////////////////////////////////////////////
//
// Daubechies
//

static void Daub1WaveletStep(double* x, int n, double* tmp) {
  if ( n < 4 ) {
    return;
  }
  int i, j;
  int half = n >> 1;

  i = 0;
  for ( j = 0; j < n - 3; j = j + 2) {
    tmp[i]        = (x[j]     * kDaubH0 +
                     x[j + 1] * kDaubH1 +
                     x[j + 2] * kDaubH2 +
                     x[j + 3] * kDaubH3);
    tmp[i + half] = (x[j]     * kDaubG0 +
                     x[j + 1] * kDaubG1 +
                     x[j + 2] * kDaubG2 +
                     x[j + 3] * kDaubG3);
    i++;
  }
  tmp[i]      = (x[n-2] * kDaubH0 +
                 x[n-1] * kDaubH1 +
                 x[0]   * kDaubH2 +
                 x[1]   * kDaubH3);
  tmp[i+half] = (x[n-2] * kDaubG0 +
                 x[n-1] * kDaubG1 +
                 x[0]   * kDaubG2 +
                 x[1]   * kDaubG3);

  memcpy(x, tmp, n * sizeof(double));
}

void Daub2Wavelet(double* x, int width, int height) {
  double* temp_col = new double[height];
  double* temp_temp = new double[width > height ? width : height];

  int w = width;
  int h = height;
  int* col_ndx = new int[height];
  for ( int i = 0; i < height; ++i ) {
    col_ndx[i] = width * i;
  }
#define X(i, j) \
  x[col_ndx[(j)] + (i)]

  while ( w > 4 || h > 4 ) {
    if ( w > 4 && w >= h ) {
      if ( (w & 1) == 1 ) {
        --w;    // well this is quite approximate..
      }
      for ( int i = 0; i < h; ++i ) {
        Daub1WaveletStep(&X(0, i), w, temp_temp);
      }
      w >>= 1;
    } else if ( h > 4 && h >= w ) {
      if ( (h & 1) == 1 ) {
        --h;    // well this is quite approximate..
      }
      for ( int i = 0; i < w; ++i ) {
        for ( int j = 0; j < h; ++j ) {
          temp_col[j] = X(i, j);
        }
        Daub1WaveletStep(temp_col, h, temp_temp);
        for ( int j = 0; j < h; ++j ) {
          X(i, j) = temp_col[j];
        }
      }
      h >>= 1;
    }
  }
#undef X
  delete [] col_ndx;
  delete [] temp_col;
  delete [] temp_temp;
}

//////////////////////////////////////////////////////////////////////

void WaveletRearrangeCoef(double* s, double* d, int width, int height) {
  int w = width;
  int h = height;
  int pos = width * height - 1;

  int* col_ndx = new int[height];
  for ( int i = 0; i < height; ++i ) {
    col_ndx[i] = width * i;
  }
#define S(i, j)                                 \
  s[col_ndx[(j)] + (i)]

  while ( w > 1 || h > 1 ) {
    if ( w > 1 && w >= h ) {
      if ( (w & 1) == 1 ) {
        pos -= h;
        for ( int i = 0; i < h; ++i ) {
          d[pos + i] = S(w-1, i);
        }
        --w;
      }
      int k = 0;
      pos -= h * w / 2;
      for ( int i = 0; i < h; ++i ) {
        for ( int j = w/2; j < w; ++j ) {
          d[pos + k] = S(j, i);
          ++k;
        }
      }
      w >>= 1;
    } else if ( h > 1 && h >= w ) {
      if ( (h & 1) == 1 ) {
        pos -= w;
        for ( int i = 0; i < w; ++i ) {
          d[pos + i] = S(i, h-1);
        }
        --h;
      }
      int k = 0;
      pos -= h * w / 2;
      for ( int i = h/2; i < h; ++i ) {
        for ( int j = 0; j < w; ++j ) {
          d[pos + k] = S(j, i);
          ++k;
        }
      }
      h >>= 1;
    }
  }
  d[0] = s[0];
#undef S
}
