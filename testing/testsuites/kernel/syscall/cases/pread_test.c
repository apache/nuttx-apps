/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/cases/pread_test.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "SyscallTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define K1 32
#define K2 (K1 * 2)
#define K3 (K1 * 3)
#define K4 (K1 * 4)
#define K5 (K1 * 5)

/****************************************************************************
 * Private data Prototypes
 ****************************************************************************/

char pread01_filename[64] = "";
int pread01_fildes;         /* file descriptor for tempfile */
char *pread01_write_buf[4]; /* buffer to hold data to be written */
char *pread01_read_buf[4];  /* buffer to hold data read from file */

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

void pread01_setup(void);   /* Main setup function of test */
void pread01_cleanup(void); /* cleanup function for the test */
void pread01_l_seek(int, off_t, int,
                    off_t); /* function to call lseek() */
void pread01_init_buffers(
    void); /* function to initialize/allocate buffers */
int pread01_compare_bufers(
    void); /* function to compare o/p of pread/pwrite */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pread01_init_buffers
 ****************************************************************************/

/*  init_buffers - allocates both write_buf and read_buf arrays.
 *  Allocate the read and write buffers.
 *  Fill the write buffer with the following data like,
 *    write_buf[0] has 0's, write_buf[1] has 1's, write_buf[2] has 2's
 *    write_buf[3] has 3's.
 */

void pread01_init_buffers(void)
{
  int count; /* counter variable for loop */

  /* Allocate and Initialize read/write buffer */

  for (count = 0; count < 4; count++)
    {
      pread01_write_buf[count] = (char *)malloc(K1);
      pread01_read_buf[count] = (char *)malloc(K1);

      if ((pread01_write_buf[count] == NULL) ||
          (pread01_read_buf[count] == NULL))
        {
          syslog(LOG_ERR, "malloc() failed on read/write buffers\n");
        }

      memset(pread01_write_buf[count], count, K1);
    }
}

/****************************************************************************
 * Name: pread01_setup
 ****************************************************************************/

/* setup() - performs all ONE TIME setup for this test.
 *
 *  Initialize/allocate read/write buffers.
 *  Create a temporary directory and a file under it and
 *  write know data at different offset positions.
 */

void pread01_setup(void)
{
  int nwrite = 0; /* no. of bytes written by pwrite() */

  /* Allocate/Initialize the read/write buffer with know data */

  pread01_init_buffers();

  sprintf(pread01_filename, "%s_tstfile", __func__);

  /* Creat a temporary file used for mapping */

  if ((pread01_fildes = open(pread01_filename, O_RDWR | O_CREAT, 0666)) <
      0)
    {
      syslog(LOG_ERR, "FAIL, open() on %s failed, errno=%d : %s\n",
             pread01_filename, errno, strerror(errno));
    }

  if (pread01_fildes > 0)
    {
      /* pwrite() K1 of data (0's) at offset 0 of temporary file */

      if ((nwrite = pwrite(pread01_fildes, pread01_write_buf[0], K1,
                           0)) != K1)
        {
          syslog(LOG_ERR,
                 "pwrite() failed to write on %s, "
                 "errno=%d : %s\n",
                 pread01_filename, errno, strerror(errno));
        }

      /* We should still be at offset 0. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, 0);

      /* Now, lseek() to a non K boundary, just to be different. */

      pread01_l_seek(pread01_fildes, K1 / 2, SEEK_SET, K1 / 2);

      /* Again, pwrite() K1 of data (2's) at offset K2 of temporary file
       */

      if ((nwrite = pwrite(pread01_fildes, pread01_write_buf[2], K1,
                           K2)) != K1)
        {
          syslog(LOG_ERR,
                 "FAIL, pwrite() failed to write at %d off. "
                 "on %s, errno=%d : %s\n",
                 K2, pread01_filename, errno, strerror(errno));
        }

      /* We should still be at our non K boundary. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, K1 / 2);

      /* lseek() to an offset of K3. */

      pread01_l_seek(pread01_fildes, K3, SEEK_SET, K3);

      /* Using write(), write of K1 of data (3's) which should take
       * place at an offset of K3, moving the file pointer to K4.
       */

      if ((nwrite = write(pread01_fildes, pread01_write_buf[3], K1)) !=
          K1)
        {
          syslog(LOG_ERR,
                 "write() failed: nwrite=%d, errno=%d "
                 ": %s\n",
                 nwrite, errno, strerror(errno));
        }

      /* We should be at offset K4. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, K4);

      /* Again, pwrite() K1 of data (1's) at offset K1. */

      if ((nwrite = pwrite(pread01_fildes, pread01_write_buf[1], K1,
                           K1)) != K1)
        {
          syslog(LOG_ERR,
                 "pwrite() failed to write at %d off. "
                 "on %s, errno=%d : %s\n",
                 K1, pread01_filename, errno, strerror(errno));
        }
    }
}

/****************************************************************************
 * Name: pread01_l_seek
 ****************************************************************************/

/* l_seek() - local front end to lseek().
 *
 *  "checkoff" is the offset at which we believe we should be at.
 *  Used to validate pread/pwrite don't move the offset.
 */

void pread01_l_seek(int fdesc, off_t offset, int whence, off_t checkoff)
{
  off_t offloc; /* offset ret. from lseek() */

  if ((offloc = lseek(fdesc, offset, whence)) != checkoff)
    {
      syslog(LOG_WARNING,
             "return = %" PRId64 " , expected %" PRId64 "\n",
             (int64_t)offloc, (int64_t)checkoff);
      syslog(LOG_ERR, "lseek() on %s failed\n", pread01_filename);
    }
}

/****************************************************************************
 * Name: pread01_compare_bufers
 ****************************************************************************/

/* compare_bufers() - Compare the contents of read buffer aganist the
 *                    write buffer contents.
 *
 *  The contents of the index of each buffer should be as follows:
 *  [0] has 0's, [1] has 1's, [2] has 2's, and [3] has 3's.
 *
 *  This function does memcmp of read/write buffer and display message
 *  about the functionality of pread().
 */

int pread01_compare_bufers(void)
{
  int count;       /* index for the loop */
  int err_flg = 0; /* flag to indicate error */

  for (count = 0; count < 4; count++)
    {
      if (memcmp(pread01_write_buf[count], pread01_read_buf[count],
                 K1) != 0)
        {
          syslog(LOG_ERR, "FAIL, read/write buffer data mismatch\n");
          err_flg++;
        }
    }

  /* If no erros, Test successful */

  if (!err_flg)
    {
      /* syslog(LOG_INFO, "PASS, Functionality of pread() is correct\n"); */

      return 0;
    }

  else
    {
      return -1;
    }
}

/****************************************************************************
 * Name: pread01_cleanup
 ****************************************************************************/

void pread01_cleanup(void)
{
  int count;

  /* Free the memory allocated for the read/write buffer */

  for (count = 0; count < 4; count++)
    {
      free(pread01_write_buf[count]);
      free(pread01_read_buf[count]);
    }

  /* Close the temporary file */

  close(pread01_fildes);

  unlink(pread01_filename);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_pread01
 ****************************************************************************/

void test_nuttx_syscall_pread01(FAR void **state)
{
  int lc;
  int nread; /* no. of bytes read by pread() */

  pread01_setup();

  for (lc = 0; lc < 1; lc++)
    {
      /* Call pread() of K1 data (should be 2's) at offset K2.
       */

      nread = pread(pread01_fildes, pread01_read_buf[2], K1, K2);

      /* Check for the return value of pread() */

      assert_int_equal(nread, K1);

      /* We should still be at offset K4,
       * which we were at the end of block 0.
       */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, K4);

      /* Now lseek() to offset 0. */

      pread01_l_seek(pread01_fildes, 0, SEEK_SET, 0);

      /* pread() K1 of data (should be 3's) at offset K3. */

      nread = pread(pread01_fildes, pread01_read_buf[3], K1, K3);
      assert_int_equal(nread, K1);

      /* We should still be at offset 0. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, 0);

      /* Do a normal read() of K1 data (should be 0's)
       * which should take place at offset 0 and move the
       * file pointer to an offset of K1.
       */

      nread = read(pread01_fildes, pread01_read_buf[0], K1);
      assert_int_equal(nread, K1);

      /* We should now be at an offset of K1. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, K1);

      /* pread() of K1 data (should be 1's) at offset K1. */

      nread = pread(pread01_fildes, pread01_read_buf[1], K1, K1);
      assert_int_equal(nread, K1);

      /* We should still be at offset K1. */

      pread01_l_seek(pread01_fildes, 0, SEEK_CUR, K1);

      /* Compare the read buffer data read
       * with the data written to write buffer
       * in the setup.
       */

      assert_int_equal(pread01_compare_bufers(), 0);

      /* reset our location to offset K4 in case we are looping */

      pread01_l_seek(pread01_fildes, K4, SEEK_SET, K4);
    }

  pread01_cleanup();
}
