/****************************************************************************
 * apps/testing/drivers/ser/ser.c
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

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <nuttx/serial/serial.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct serial_icounter_s rcount;
  struct serial_icounter_s scount;

  int fd;
  int ret;

  fd = open(CONFIG_TESTING_SERIAL_PORT, O_RDONLY);
  if (fd < 0)
    {
      ret = -ENODEV;
      goto out;
    }

  ret = ioctl(fd, TIOCGICOUNT, &scount);
  if (ret < 0)
    {
      goto out_close;
    }

  while (1)
    {
      ret = ioctl(fd, TIOCGICOUNT, &rcount);

      if (rcount.frame > scount.frame)
        {
          printf("ERROR: framing %d\n", rcount.frame - scount.frame);
        }

      if (rcount.overrun > scount.overrun)
        {
          printf("ERROR: overrun %d\n", rcount.overrun - scount.overrun);
        }

      if (rcount.parity > scount.parity)
        {
          printf("ERROR: parity %d\n", rcount.parity - scount.parity);
        }

      if (rcount.brk > scount.brk)
        {
           printf("ERROR: break %d\n", rcount.brk - scount.brk);
        }

      if (rcount.buf_overrun > scount.buf_overrun)
        {
          printf("ERROR: buffer overrun %d\n",
                 rcount.buf_overrun - scount.buf_overrun);
        }

      scount = rcount;
      usleep(1000000);
    }

out_close:
  close(fd);
out:

  return ret;
}
