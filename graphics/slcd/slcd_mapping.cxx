/////////////////////////////////////////////////////////////////////////////
// apps/graphics/slcd/slcd_mapping.cxx
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

// Segment
//
//  11111111
// 2        3
// 2        3
// 2        3
//  44444444
// 5        6
// 5        6
// 5        6
//  77777777

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdint>

#include "graphics/slcd.hxx"
#include "slcd.hxx"

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

namespace SLcd
{
  const uint8_t GSLcdDigits[10] =
  {
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_5 |  // 0
    SEGMENT_6 | SEGMENT_7,
    SEGMENT_3 | SEGMENT_6,                           // 1
    SEGMENT_1 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // 2
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_3 | SEGMENT_4 | SEGMENT_6 |  // 3
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_6,   // 4
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_6 |  // 5
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5 |  // 6
    SEGMENT_6 | SEGMENT_7,                           // 7
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_6,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // 8
    SEGMENT_5 | SEGMENT_6 | SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // 9
    SEGMENT_6 | SEGMENT_7
  };

  const uint8_t GSLcdLowerCase[26] =
  {
    SEGMENT_1 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // a
    SEGMENT_6 | SEGMENT_7,
    SEGMENT_2 | SEGMENT_4 | SEGMENT_5 | SEGMENT_6 |  // b
    SEGMENT_7,
    SEGMENT_4 | SEGMENT_5 | SEGMENT_7,               // c
    SEGMENT_3 | SEGMENT_4 | SEGMENT_5 | SEGMENT_6 |  // d
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // e
    SEGMENT_5 | SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5,   // f
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // g
    SEGMENT_6 | SEGMENT_7,
    SEGMENT_2 | SEGMENT_4 | SEGMENT_5 | SEGMENT_6,   // h
    SEGMENT_5,                                       // i
    SEGMENT_6 | SEGMENT_7,                           // j
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5 |  // k
    SEGMENT_6,
    SEGMENT_3 | SEGMENT_6,                           // l
    SEGMENT_5 | SEGMENT_6,                           // m
    SEGMENT_4 | SEGMENT_5 | SEGMENT_6,               // n
    SEGMENT_4 | SEGMENT_5 | SEGMENT_6 | SEGMENT_7,   // o
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // p
    SEGMENT_5,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // q
    SEGMENT_6,
    SEGMENT_4 | SEGMENT_5,                           // r
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_6 |  // s
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_4 | SEGMENT_5 | SEGMENT_7,   // t
    SEGMENT_5 | SEGMENT_6 | SEGMENT_7,               // u
    SEGMENT_5 | SEGMENT_6 | SEGMENT_7,               // v
    SEGMENT_5 | SEGMENT_6,                           // w
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // x
    SEGMENT_6,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_6 |  // y
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // z
    SEGMENT_7
  };

  const uint8_t GSLcdUpperCase[26] =
  {
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // A
    SEGMENT_5 | SEGMENT_6,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // B
    SEGMENT_5 | SEGMENT_6 | SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_5 | SEGMENT_7,   // C
    SEGMENT_3 | SEGMENT_4 | SEGMENT_5 | SEGMENT_6 |  // D
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5 |  // E
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5,   // F
    SEGMENT_1 | SEGMENT_2 | SEGMENT_5 | SEGMENT_6 |  // G
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // H
    SEGMENT_6,
    SEGMENT_3 | SEGMENT_6,                           // I
    SEGMENT_3 | SEGMENT_5 | SEGMENT_6 | SEGMENT_7,   // J
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_5 |  // K
    SEGMENT_6,
    SEGMENT_2 | SEGMENT_5 | SEGMENT_7,               // L
    SEGMENT_1 | SEGMENT_5 | SEGMENT_6,               // M
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_5 |  // N
    SEGMENT_6,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_5 |  // O
    SEGMENT_6 | SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // P
    SEGMENT_5,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_4 |  // Q
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_2 | SEGMENT_3 | SEGMENT_5,   // R
    SEGMENT_1 | SEGMENT_2 | SEGMENT_4 | SEGMENT_6 |  // S
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_4 | SEGMENT_5 | SEGMENT_7,   // T
    SEGMENT_2 | SEGMENT_3 | SEGMENT_5 | SEGMENT_6 |  // U
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_5 | SEGMENT_6 |  // V
    SEGMENT_7,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_7,               // W
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // X
    SEGMENT_6,
    SEGMENT_2 | SEGMENT_3 | SEGMENT_4 | SEGMENT_6 |  // Y
    SEGMENT_7,
    SEGMENT_1 | SEGMENT_3 | SEGMENT_4 | SEGMENT_5 |  // Z
    SEGMENT_7
  };

  const struct SSLcdCharset SSLcdMisc[NMISC_MAPPINGS] =
  {
    {
      .ch       = ' ',
      .segments = 0
    },
    {
      .ch       = '-',
      .segments = SEGMENT_4
    },
    {
      .ch       = '_',
      .segments = SEGMENT_7
    },
    {
      .ch       = '"',
      .segments = SEGMENT_3 | SEGMENT_3
    },
    {
      .ch       = ',',
      .segments = SEGMENT_5
    }
  };
}
