/////////////////////////////////////////////////////////////////////////////
// apps/graphics/slcd/slcd.hxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APPS_GRAPHICS_SLCD_SLCD_H
#define __APPS_GRAPHICS_SLCD_SLCD_H

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <fixedmath.h>

#include "graphics/slcd.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor definitions
/////////////////////////////////////////////////////////////////////////////

// Segment encoding:
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

#define SEGMENT_1               (1 << 0)
#define SEGMENT_2               (1 << 1)
#define SEGMENT_3               (1 << 2)
#define SEGMENT_4               (1 << 3)
#define SEGMENT_5               (1 << 4)
#define SEGMENT_6               (1 << 5)
#define SEGMENT_7               (1 << 6)

// Number of segments

#define NSEGMENTS               7

// Segment image aspect ratio width:height

#define SLCD_ASPECT             0.577381L // Width / Height
#define SLCD_ASPECT_B16         37577     // Width / Height

// Number of miscellaneous character mappings

#define NMISC_MAPPINGS          5

namespace SLcd
{

/////////////////////////////////////////////////////////////////////////////
// Public Types
/////////////////////////////////////////////////////////////////////////////

  // Each trapezoid is represented by a sequence of runs.  The first run
  // provides the top of the first trapezoid; the following provide the
  // bottom of each successive trapezoid.  The top of each subsequent
  // trapezoid is the bottom of the trapezoid above it

  struct SLcdTrapezoidRun
  {
    b16_t leftx;      // Left X position of run
    b16_t rightx;     // Right X position of run
    b16_t y;          // Y position of run
  };

  // Used to implement a sparse array that matches a few miscellaneous
  // ASCII characters to an LCD segment set.

  struct SSLcdCharset
  {
    char ch;          // ASCII character to be mapped
    uint8_t segments; // Segment set that represents that character
  };

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

  // An arrays of runs.  The number of runs in each array is equal to the
  // number of trapezoids + 1

  extern const struct SLcdTrapezoidRun
    GTop_Runs[NTOP_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GTopLeft_Runs[NTOPLEFT_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GTopRight_Runs[NTOPRIGHT_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GMiddle_Runs[NMIDDLE_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GBottomLeft_Runs[NBOTTOMLEFT_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GBottomRight_Runs[NBOTTOMRIGHT_TRAPEZOIDS + 1];
  extern const struct SLcdTrapezoidRun
    GBottom_Runs[NBOTTOM_TRAPEZOIDS + 1];

  // ASCII to segment set mapping arrays

  extern const uint8_t GSLcdDigits[10];
  extern const uint8_t GSLcdLowerCase[26];
  extern const uint8_t GSLcdUpperCase[26];
  extern const struct SSLcdCharset SSLcdMisc[NMISC_MAPPINGS];
}

#endif /* __APPS_GRAPHICS_SLCD_SLCD_H */
