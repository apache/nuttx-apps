/////////////////////////////////////////////////////////////////////////////
// apps/graphics/slcd/slcd_trapezoids.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include "graphics/slcd.hxx"
#include "slcd.hxx"

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

namespace SLcd
{
  // Top horizontal segment

  const struct SLcdTrapezoidRun GTop_Runs[NTOP_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 11703,
      .rightx = 31208,
      .y      = 0
    },
    [1]       =
    {
      .leftx  = 8972,
      .rightx = 27323,
      .y      = 780
    },
    [2]       =
    {
      .leftx  = 9655,
      .rightx = 34328,
      .y      = 1560
    },
    [3]       =
    {
      .leftx  = 14434,
      .rightx = 28867,
      .y      = 7022
    }
  };

  // Top-left vertical segment

  const struct SLcdTrapezoidRun GTopLeft_Runs[NTOPLEFT_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 7022,
      .rightx = 7022,
      .y      = 2341
    },
    [1]       =
    {
      .leftx  = 5461,
      .rightx = 9362,
      .y      = 4681
    },
    [2]       =
    {
      .leftx  = 5098,
      .rightx = 12873,
      .y      = 8192
    },
    [3]       =
    {
      .leftx  = 3193,
      .rightx = 10923,
      .y      = 26526
    },
    [4]       =
    {
      .leftx  = 3121,
      .rightx = 9986,
      .y      = 27307
    },
    [5]       =
    {
      .leftx  = 6242,
      .rightx = 6242,
      .y      = 30427
    }
  };

  // Top-right vertical segment

  const struct SLcdTrapezoidRun GTopRight_Runs[NTOPRIGHT_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 37059,
      .rightx = 37059,
      .y      = 3901
    },
    [1]       =
    {
      .leftx  = 33997,
      .rightx = 37839,
      .y      = 6242
    },
    [2]       =
    {
      .leftx  = 30427,
      .rightx = 37591,
      .y      = 8972
    },
    [3]       =
    {
      .leftx  = 28477,
      .rightx = 35924,
      .y      = 27307
    },
    [4]       =
    {
      .leftx  = 28518,
      .rightx = 35889,
      .y      = 27697
    },
    [5]       =
    {
      .leftx  = 32378,
      .rightx = 32378,
      .y      =30818
    }
  };

  // Middle horizontal segment

  const struct SLcdTrapezoidRun GMiddle_Runs[NMIDDLE_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 12483,
      .rightx = 27307,
      .y      = 28867
    },
    [1]       =
    {
      .leftx  = 8192,
      .rightx = 30427,
      .y      = 32378
    },
    [2]       =
    {
      .leftx  = 11703,
      .rightx = 25746,
      .y      = 36279
    }
  };

  // Bottom-left vertical segment

  const struct SLcdTrapezoidRun GBottomLeft_Runs[NBOTTOMLEFT_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 5851,
      .rightx = 5851,
      .y      = 33938
    ,
    },
    [1]       =
    {
      .leftx  = 1950,
      .rightx = 9752,
      .y      = 37449
    },
    [2]       =
    {
      .leftx  = 370,
      .rightx = 8192,
      .y      = 55784
    },
    [3]       =
    {
      .leftx  = 0,
      .rightx = 3364,
      .y      = 60075
    },
    [4]       =
    {
      .leftx  = 1170,
      .rightx = 1170,
      .y      = 62025
    }
  };

  // Bottom-right vertical segment

  const struct SLcdTrapezoidRun GBottomRight_Runs[NBOTTOMRIGHT_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 31988,
      .rightx = 31988,
      .y      = 34328
    },
    [1]       =
    {
      .leftx  = 28769,
      .rightx = 35109,
      .y      = 37059
    },
    [2]       =
    {
      .leftx  = 27307,
      .rightx = 34996,
      .y      = 38229
    },
    [3]       =
    {
      .leftx  = 25746,
      .rightx = 33183,
      .y      = 56954
    },
    [4]       =
    {
      .leftx  = 30324,
      .rightx = 32768,
      .y      = 61245
    },
    [5]       =
    {
      .leftx  = 31988,
      .rightx = 31988,
      .y      = 62805
    }
  };

  // Bottom horizontal segment

  const struct SLcdTrapezoidRun GBottom_Runs[NBOTTOM_TRAPEZOIDS + 1] =
  {
    [0]       =
    {
      .leftx  = 9362,
      .rightx = 24576,
      .y      = 58124
    },
    [1]       =
    {
      .leftx  = 2731,
      .rightx = 31988,
      .y      = 63976
    },
    [2]       =
    {
      .leftx  = 4681,
      .rightx = 28477,
      .y      = 65536
    }
  };
}
