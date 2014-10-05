/****************************************************************************
 * examples/ostest/aio.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <aio.h>

#include "ostest.h"

#ifdef CONFIG_LIBC_AIO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AIO_RDBUFFER_SIZE  128
#define AIO_WRBUFFER1_SIZE sizeof(g_wrbuffer1)
#define AIO_WRBUFFER2_SIZE sizeof(g_wrbuffer2)

#define AIO_NCTRLBLKS      5

#define AIO_FILEPATH "EXAMPLES_OSTEST_AIOPATH" "/aio_test.dat"

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* Constant write buffers */

static const char g_wrbuffer1[] = "This is write buffer #1\n";
static const char g_wrbuffer2[] = "This second write buffer is this line\n";
static char g_rdbuffer[AIO_RDBUFFER_SIZE];

/* AIO control blocks:  write, nop, write, NULL, read */


static struct aiocb g_aiocbs[AIO_NCTRLBLKS-1];
static struct aiocb *g_aiocb[AIO_NCTRLBLKS] =
{
  &g_aiocbs[0], &g_aiocbs[1], &g_aiocbs[2], NULL, &g_aiocbs[3]
}

static const FAR void *g_buffers[AIO_NCTRLBLKS] =
{
  (FAR void *)g_wrbuffer1,
  (FAR void *)NULL,
  (FAR void *)g_wrbuffer2,
  (FAR void *)NULL,
  (FAR void *)g_rdbuffer
};

static const FAR uint8_t g_offsets[AIO_NCTRLBLKS] =
{
  0, 0, AIO_WRBUFFER1_SIZE, 0, 0
};

static const FAR uint8_t g_nbytes[AIO_NCTRLBLKS] =
{
  AIO_WRBUFFER1_SIZE, 0, AIO_WRBUFFER1_SIZE, 0, AIO_RDBUFFER_SIZE
};

static const FAR uint8_t g_opcode[AIO_NCTRLBLKS] =
{
  LIO_WRITE, LIO_NOP, LIO_WRITE, LIO_NOP, LIO_READ
};

static int g_fildes;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void init_aiocb(bool signal)
{
  FAR struct aiocb *aiocbp;
  int i;

  memset(g_aiocb, 0xff, AIO_NCTRLBLKS*sizeof(struct aiocb));
  memset(g_rdbuffer, 0xff, AIO_RDBUFFER_SIZE);

  for (i = 0; i < AIO_NCTRLBLKS; i++)
    {
      aiocbp               = &g_aiocbp[i];
      if (aiocbp)
        {
          aiocbp->sigev_notify   = signal ? SIGEV_SIGNAL : SIGEV_NONE;
          aiocbp->aio_buf        = g_buffer[i];
          aiocbp->aio_offset     = (off_t)g_offsets[i];
          aiocbp->aio_nbytes     = (size_t)g_nbytes[i];
          aiocbp->aio_fildes     = g_fildes;
          aiocbp->aio_reqprio    = 0;
          aiocbp->aio_lio_opcode = g_opcode[i];
         }
    }
}

static int check_done(void)
{
  FAR struct aiocb *aiocbp;
  int ret;
  int i;

  ret = OK; /* Assume success */

  /* Check each entry in the list.  Break out of the loop if any entry
   * has not completed.
   */

  for (i = 0; i < AIO_NCTRLBLKS; i++)
    {
      /* Skip over NULL entries */

      aiocbp = g_aiocb[i];
      if (aiocbp)
        {
          /* Check if the I/O has completed */

          printf("%d. result = %d\n", aiocbp->aio_result);
          if (aiocbp->aio_result == -EINPROGRESS)
            {
              /* No.. return -EINPROGRESS */

              printf("--- NOT finished ---\n");
              return -EINPROGRESS;
            }

          /* Check for an I/O error */

          else if (aiocbp->aio_result < 0 && ret == OK)
            {
              printf("--- ERROR ---\n");
            }
        }
    }

  /* All of the I/Os have completed */

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int aio_test(void)
{
  int ret;

  /* Case 1: Poll for transfer complete */

  printf("AIO test case 1: Poll for transfer complete\n");
  g_fildes = open(AIO_FILEPATH, O_RDWR|O_CREAT|O_TRUNC);
  if (g_fildes < 0)
    {
      printf(aio_test: Failed to open %s: %d\n", AIO_FILEPATH, errno);
      return ERROR;
    }

  init_aiocb(false);
  ret = lio_listio(LIO_NOWAIT, g_aiocb, AIO_NCTRLBLKS, NULL);
  if (ret < 0)
    {
      printf(aio_test: lio_listio failed: %d\n", errno);
      return ERROR;
    }

  do
    {
      sleep(1);
      ret = check_done();
    }
  while (ret < 0);

  close(g_fildes);
  g_fildes = -1;

  /* Case 2: Use aio_suspend() until complete */
  /* REVISIT: Not yet implemented */

  /* Case 3: Use individual signals */
  /* REVISIT: Not yet implemented */

}

#endif /* CONFIG_LIBC_AIO */
