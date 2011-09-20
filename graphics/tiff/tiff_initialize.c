/****************************************************************************
 * apps/graphics/tiff/tiff_initialize.c
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

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <apps/tiff.h>

#include "tiff_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_writeheader
 *
 * Description:
 *   Setup to create a new TIFF file.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static inline int tiff_writeheader(FAR struct tiff_info_s *info)
{
  struct tiff_header_s hdr;
  int ret;

  /* 0-1: Byte order */

#ifdef CONFIG_ENDIAN_BIG
  hdr.order[0] = 'M';  /* "MM"=big endian */
  hdr.order[1] = 'M';
#else
  hdr.order[0] = 'I';  /* "II"=little endian */
  hdr.order[1] = 'I';
#endif

  /* 2-3: 42 in appropriate byte order */

  tiff_put16(hdr.magic, 42);

  /* 4-7: Offset to the first IFD */
  
  tiff_put16(hdr.offset, sizeof(struct tiff_header_s));

  /* Write the header to the output file */

  ret = tiff_write(info->outfd, &hdr, sizeof(struct tiff_header_s));
  if (ret == OK)
    {
      info->outsize = sizeof(struct tiff_header_s);
    }
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_initialize
 *
 * Description:
 *   Setup to create a new TIFF file.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_initialize(FAR struct tiff_info_s *info)
{
  int ret = -EINVAL;

  DEBUGASSERT(info && info->outfile && info->tmpfile1 && info->tmpfile2);

  /* Open all output files */

  info->outfd = open(info->outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->outfd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->outfile, errno);
      goto errout;
    }

  info->tmp1fd = open(info->tmpfile1, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->tmp1fd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->tmpfile1, errno);
      goto errout_with_outfd;
    }

  info->tmp2fd = open(info->tmpfile1, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->tmp2fd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->tmpfile1, errno);
      goto errout_with_tmp1fd;
    }

  /* Write the TIFF header data to the outfile */

  ret = tiff_writeheader(info);
  if (ret < 0)
    {
      goto errout_with_tmp2fd;
    }

  /* Write the IFD data to the outfile */
#warning "Missing Logic"
  return OK;

errout_with_tmp2fd:
  (void)close(info->tmp2fd);
  info->tmp2fd = -1;
errout_with_tmp1fd:
  (void)close(info->tmp1fd);
  info->tmp1fd = -1;
errout_with_outfd:
  (void)close(info->outfd);
  info->outfd = -1;
errout:
  return ret;
}

