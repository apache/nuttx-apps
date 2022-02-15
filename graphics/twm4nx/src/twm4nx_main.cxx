/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/twm4nx_main.cxx
// Twm4Nx main entry point
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/boardctl.h>

#include "netutils/netinit.h"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"

// Applications

#include "graphics/twm4nx/apps/ccalibration.hxx"
#include "graphics/twm4nx/apps/cnxterm.hxx"
#include "graphics/twm4nx/apps/cclock.hxx"

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Suppress name-mangling

extern "C" int main(int argc, FAR char *argv[]);

/////////////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/////////////////////////////////////////////////////////////////////////////
// Name: main/twm4nx_main
//
// Description:
//    Start of TWM
//
/////////////////////////////////////////////////////////////////////////////

int main(int argc, FAR char *argv[])
{
  int display = 0;

  for (int i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-')
        {
          switch (argv[i][1])
            {
            case 'd':          // -display <number>
              if (std::strcmp(&argv[i][1], "display"))
                {
                  goto usage;
                }

              if (++i >= argc)
                {
                  goto usage;
                }

              display = atoi(argv[i]);
              continue;
            }
        }

    usage:
      twmerr("Usage:  %s [-display <number>]\n", argv[0]);
      return EXIT_FAILURE;
    }

  int ret;

#if defined(CONFIG_TWM4NX_ARCHINIT) && defined(CONFIG_BOARDCTL) && \
   !defined(CONFIG_BOARD_LATE_INITIALIZE)
  // Should we perform board-specific initialization?  There are two ways
  // that board initialization can occur:  1) automatically via
  // board_late_initialize() during bootup if CONFIG_BOARD_LATE_INITIALIZE, or
  // 2) here via a call to boardctl() if the interface is enabled
  // (CONFIG_BOARDCTL=y).  board_early_initialize() is also possibility,
  // although less likely.

  ret = boardctl(BOARDIOC_INIT, 0);
  if (ret < 0)
    {
      twmerr("ERROR: boardctl(BOARDIOC_INIT) failed: %d\n", errno);
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TWM4NX_NETINIT
  /* Bring up the network */

  ret = netinit_bringup();
  if (ret < 0)
    {
      twmerr("ERROR: netinit_bringup() failed: %d\n", ret);
      return EXIT_FAILURE;
    }
#endif

  UNUSED(ret);

  /* Create an instance of CTwm4Nx and initialize it */

  FAR CTwm4Nx *twm4nx = new CTwm4Nx(display);
  if (twm4nx == (FAR CTwm4Nx *)0)
    {
      twmerr("ERROR: Failed to instantiate CTwm4Nx\n");
      return EXIT_FAILURE;
    }

  bool success = twm4nx->initialize();
  if (!success)
    {
      twmerr(" ERROR:  Failed to initialize CTwm4Nx\n");
      return EXIT_FAILURE;
    }

  // Twm4Nx is fully initialized and we may now register applications
  // Revisit.  This is currently hard-coded here for testing.  There
  // needs to be a more flexible method if adding applications at run
  // time.

#ifdef CONFIG_TWM4NX_CALIBRATION
  CCalibrationFactory calibFactory;
  success = calibFactory.initialize(twm4nx);
  if (!success)
    {
      twmerr(" ERROR:  Failed to initialize CNxTermFactory\n");
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TWM4NX_CLOCK
  CClockFactory clockFactory;
  success = clockFactory.initialize(twm4nx);
  if (!success)
    {
      twmerr(" ERROR:  Failed to initialize CClockFactory\n");
      return EXIT_FAILURE;
    }
#endif

#ifdef CONFIG_TWM4NX_NXTERM
  CNxTermFactory nxtermFactory;
  success = nxtermFactory.initialize(twm4nx);
  if (!success)
    {
      twmerr(" ERROR:  Failed to initialize CNxTermFactory\n");
      return EXIT_FAILURE;
    }
#endif

  // Start the Twm4Nx event loop

  success = twm4nx->eventLoop();
  if (!success)
    {
      twmerr(" ERROR: Event loop terminating due to failure\n");
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
