/****************************************************************************
 * apps/examples/camera/camera_fileutil.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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

