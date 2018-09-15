/****************************************************************************
 * examples/ostest/setvbuf.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifndef CONFIG_STDIO_DISABLE_BUFFERING

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * setvbuf_test
 ****************************************************************************/

int setvbuf_test(void)
{
  FILE *stream;
  char buffer[64];
  int ret;

  stream = fopen("/dev/console", "w");
  if (stream == NULL)
    {
      printf("setvbuf_test ERROR: fopen(dev/console, rw) failed\n");
      return -1;
    }

  fprintf(stream, "setvbuf_test: DEFAULT buffering\r\n");

  ret = setvbuf(stream, NULL, _IONBF, 0);
  if (ret < 0)
    {
      printf("setvbuf_test ERROR: setvbuf(stream, NULL, _IONBF, 0) failed\n");
    }

  fprintf(stream, "setvbuf_test: NO buffering\r\n");

  ret = setvbuf(stream, NULL, _IOFBF, 0);
  if (ret < 0)
    {
      printf("ssetvbuf_test ERROR: etvbuf(stream, NULL, _IONBF, 0) failed\n");
    }

  fprintf(stream, "setvbuf_test: FULL buffering, default buffer\r\n");

  ret = setvbuf(stream, NULL, _IONBF, 0);
  if (ret < 0)
    {
      printf("setvbuf_test ERROR: setvbuf(stream, NULL, _IONBF, 0) failed\n");
    }

  fprintf(stream, "setvbuf_test: NO buffering\r\n");

  ret = setvbuf(stream, NULL, _IOLBF, 64);
  if (ret < 0)
    {
      printf("setvbuf_test: ERROR: setvbuf(stream, NULL, _IOLBF, 64) failed\n");
    }

  fprintf(stream, "setvbuf_test: LINE buffering, buffer size 64\r\n");

  ret = setvbuf(stream, NULL, _IONBF, 0);
  if (ret < 0)
    {
      printf("setvbuf_test ERROR: setvbuf(stream, NULL, _IONBF, 0) failed\n");
    }

  fprintf(stream, "setvbuf_test: NO buffering\r\n");

  ret = setvbuf(stream, buffer, _IOFBF, 64);
  if (ret < 0)
    {
      printf("setvbuf_test ERROR: setvbuf(stream, NULL, _IOLBF, 64) failed\n");
    }

  fprintf(stream, "setvbuf_test: FULL buffering, pre-allocated buffer\r\n");

  ret = setvbuf(stream, NULL, _IONBF, 0);
  if (ret < 0)
    {
      printf("setvbuf_test ERROR: setvbuf(stream, NULL, _IONBF, 0) failed\n");
    }

  fprintf(stream, "setvbuf_test: NO buffering\r\n");

  fclose(stream);
  return 0;
}

#endif /* CONFIG_STDIO_DISABLE_BUFFERING */
