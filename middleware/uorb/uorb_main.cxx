//***************************************************************************
// examples/helloxx/helloxx_main.cxx
//
//   Copyright (C) 2009, 2011-2013, 2017 Gregory Nutt. All rights reserved.
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
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>
#include <fcntl.h>
#include <cstdio>
#include <stdlib.h>
#include <errno.h>
#include <debug.h>

#include "uORBDeviceNode.h"

//***************************************************************************
// Definitions
//***************************************************************************
// Configuration ************************************************************



//***************************************************************************
// Private Classes
//***************************************************************************


//***************************************************************************
// Private Data
//***************************************************************************


//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Name: helloxx_main
 ****************************************************************************/

extern "C"
{
  int main(int argc, FAR char *argv[])
 {

    uORBNodeDevice_register("/dev/topic01");

    int fd = open("/dev/topic01", O_WRONLY);
    if(fd < 0)
    {
      int errcode = errno;
      printf("uorb: ERROR: Failed to open %s: %d\n", "/dev/topic01", errcode);
      //goto errout;
    }

    const char *hola = "hola me llamo andres";
    int ret = write(fd, hola, sizeof(hola));

    return 0;

    errout:
      printf("uorb: Terminating\n");
      return EXIT_FAILURE;
  }
}
