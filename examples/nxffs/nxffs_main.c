/****************************************************************************
 * examples/nxffs/nxffs_main.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <nuttx/mtd.h>
#include <nuttx/nxffs.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* This must exactly match the default configuration in drivers/mtd/rammtd.c */

#ifndef CONFIG_RAMMTD_BLOCKSIZE
#  define CONFIG_RAMMTD_BLOCKSIZE 512
#endif

#ifndef CONFIG_RAMMTD_ERASESIZE
#  define CONFIG_RAMMTD_ERASESIZE 4096
#endif

#ifndef CONFIG_EXAMPLES_NXFFS_NEBLOCKS
#  define CONFIG_EXAMPLES_NXFFS_NEBLOCKS (32)
#endif

#undef CONFIG_EXAMPLES_NXFFS_BUFSIZE
#define CONFIG_EXAMPLES_NXFFS_BUFSIZE \
  (CONFIG_RAMMTD_ERASESIZE * CONFIG_EXAMPLES_NXFFS_NEBLOCKS)

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* Pre-allocated simulated flash */

static uint8_t g_simflash[CONFIG_EXAMPLES_NXFFS_BUFSIZE];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * user_start
 ****************************************************************************/

int user_start(int argc, char *argv[])
{
  FAR struct mtd_dev_s *mtd;
  int ret;

  /* Create and initialize a RAM MTD device instance */

  mtd = rammtd_initialize(g_simflash, CONFIG_EXAMPLES_NXFFS_BUFSIZE);
  if (!mtd)
    {
      fprintf(stderr, "ERROR: Failed to create RAM MTD instance\n");
      exit(1);
    }

  /* Initialize to provide NXFFS on an MTD interface */

  ret = nxffs_initialize(mtd);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: NXFFS initialization failed: %d\n", -ret);
      exit(2);
    }

  return 0;
}

