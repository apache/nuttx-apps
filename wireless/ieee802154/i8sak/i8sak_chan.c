/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_chan.c
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/ioctl.h>

#include "i8sak.h"

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_chan_cmd
 *
 * Description :
 *   Try and extract data from the coordinator
 ****************************************************************************/

void i8sak_chan_cmd(FAR struct i8sak_s *i8sak, int argc, FAR char *argv[])
{
  int option;
  int fd;
  int argind;
  bool getchan = false;
  uint8_t channel;

  argind = 1;
  while ((option = getopt(argc, argv, ":hg")) != ERROR)
    {
      switch (option)
        {
          argind++;
          case 'h':
            fprintf(stderr, "Polls coordinator for data\n"
                    "Usage: %s [-h]\n"
                    "    -h = this help menu\n"
                    , argv[0]);
            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            return;
          case 'g':
            getchan = true;
            break;

          case ':':
            fprintf(stderr, "ERROR: missing argument\n");
            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
          case '?':
            fprintf(stderr, "ERROR: unknown argument\n");
            /* Must manually reset optind if we are going to exit early */

            optind = -1;
            i8sak_cmd_error(i8sak); /* This exits for us */
        }
    }

  if (!getchan)
    {
      if (argc < argind + 1)
        {
          fprintf(stderr, "ERROR: missing channel\n");
          i8sak_cmd_error(i8sak); /* This exits for us */
        }

      channel  = i8sak_str2luint8(argv[argind]);
    }

  fd = open(i8sak->devname, O_RDWR);
  if (fd < 0)
    {
      printf("cannot open %s, errno=%d\n", i8sak->devname, errno);
      i8sak_cmd_error(i8sak);
    }

  if (getchan)
    {
      ieee802154_getchan(fd, &channel);
      printf("i8sak: Channel: %d\n", (int)channel);
    }
  else
    {
      printf("i8sak: Setting Channel: %d\n", (int)channel);
      ieee802154_setchan(fd, channel);
    }

  close(fd);
}
