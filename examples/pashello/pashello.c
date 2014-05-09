/****************************************************************************
 * examples/pashello/pashello.c
 *
 *   Copyright (C) 2008-2009, 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <debug.h>

#include <apps/interpreters/prun.h>

#include "pashello.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_PASHELLO_VARSTACKSIZE
# define CONFIG_EXAMPLES_PASHELLO_VARSTACKSIZE 1024
#endif

#ifndef CONFIG_EXAMPLES_PASHELLO_STRSTACKSIZE
# define CONFIG_EXAMPLES_PASHELLO_STRSTACKSIZE 128
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * pashello_main
 ****************************************************************************/

int pashello_main(int argc, FAR char *argv[])
{
  FAR struct pexec_s *st;
  int exitcode = EXIT_SUCCESS;
  int ret;

  /* Register the /dev/hello driver */

  hello_register();

  /* Execute the POFF file */

  ret = prun("/dev/hello", CONFIG_EXAMPLES_PASHELLO_VARSTACKSIZE,
             CONFIG_EXAMPLES_PASHELLO_STRSTACKSIZE);
  if (ret < 0)
    {
      fprintf(stderr, "pashello_main: ERROR: Execution failed\n");
      exitcode = EXIT_FAILURE;
    }

  printf("pashello_main: Interpreter terminated");
  return exitcode;
}
