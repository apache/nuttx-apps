/****************************************************************************
 * camera/camera_fileutil.c
 *
 *   Copyright 2020 Sony Semiconductor Solutions Corporation
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
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "camera_fileutil.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IMAGE_FILENAME_LEN (32)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *save_dir;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: futil_initialize()
 *
 * Description:
 *   Choose strage to write a file.
 ****************************************************************************/

const char *futil_initialize(void)
{
  int ret;
  struct stat stat_buf;

  /* In SD card is available, use SD card.
   * Otherwise, use SPI flash.
   */

  ret = stat("/mnt/sd0", &stat_buf);
  if (ret < 0)
    {
      save_dir = "/mnt/spif";
    }
  else
    {
      save_dir = "/mnt/sd0";
    }

  return save_dir;
}

/****************************************************************************
 * Name: futil_writeimage()
 *
 * Description:
 *   Write a image file to selected storage.
 ****************************************************************************/

int futil_writeimage(uint8_t *data, size_t len, const char *fsuffix)
{
  static char s_fname[IMAGE_FILENAME_LEN];
  static int s_framecount = 0;

  FILE *fp;

  s_framecount++;
  if (s_framecount >= 1000)
    {
      s_framecount = 1;
    }

  memset(s_fname, 0, sizeof(s_fname));

  snprintf(s_fname,
           IMAGE_FILENAME_LEN,
           "%s/VIDEO%03d.%s",
           save_dir, s_framecount, fsuffix);

  printf("FILENAME:%s\n", s_fname);

  fp = fopen(s_fname, "wb");
  if (NULL == fp)
    {
      printf("fopen error : %d\n", errno);
      return -1;
    }

  if (len != fwrite(data, 1, len, fp))
    {
      printf("fwrite error : %d\n", errno);
    }

  fclose(fp);
  return 0;
}

