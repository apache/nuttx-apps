/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_gatt.c
 * Bluetooth Swiss Army Knife -- GATT commands
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author:  Gregory Nutt <gnutt@nuttx.org>
 *
 * Based loosely on the i8sak IEEE 802.15.4 program by Anthony Merlino and
 * Sebastien Lorquet.  Commands inspired for btshell example in the
 * Intel/Zephyr Arduino 101 package (BSD license).
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

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include <nuttx/wireless/bt_core.h>
#include <nuttx/wireless/bt_gatt.h>
#include <nuttx/wireless/bt_ioctl.h>

#include "btsak.h"

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_discover_common
 *
 * Description:
 *   gatt [-h] <discover-cmd> [-h] <addr> <addr-type> [<uuid16>]
 *
 ****************************************************************************/

static void btsak_cmd_discover_common(FAR struct btsak_s *btsak,
                                      int argc, FAR char *argv[],
                                      enum bt_gatt_discover_e type)
{
  struct btreq_s btreq;
  unsigned int argndx;
  int sockfd;
  int ret;

  /* Check for help command */

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  argndx = (type == GATT_DISCOVER) ? 4 : 3;
  if (argc < argndx)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  Found %d expected at least %u\n",
              argc, argndx);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  strncpy(btreq.btr_name, btsak->ifname, HCI_DEVNAME_SIZE);
  btreq.btr_dtype = (uint8_t)type;

  ret = btsak_str2addr(argv[1], btreq.btr_dpeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_dpeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for <addr-type>: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  if (type == GATT_DISCOVER)
    {
      btreq.btr_duuid16 = btsak_str2uint16(argv[3]);
    }
  else
    {
      btreq.btr_duuid16 = 0;
    }

  btreq.btr_dstart = 0x0001;
  btreq.btr_dend   = 0xffff;

  if (argc > argndx)
    {
      btreq.btr_dstart = btsak_str2uint16(argv[argndx]);
      argndx++;

      if (argc > argndx)
        {
          btreq.btr_dend = btsak_str2uint16(argv[argndx]);
          argndx++;
        }

      if (btreq.btr_dstart > btreq.btr_dend)
        {
          fprintf(stderr, "ERROR:  Invalid handle range: %u-%u\n",
                  btreq.btr_dstart, btreq.btr_dend);
          btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
        }
    }

  /* Perform the IOCTL to start the discovery */

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTDISCOVER, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTDISCOVER) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_gatt_exchange_mtu
 *
 * Description:
 *   gatt [-h] exchange_mtu [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_exchange_mtu(FAR struct btsak_s *btsak, int argc,
                                 FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_discover
 *
 * Description:
 *   gatt [-h] discover [-h] <addr> <addr-type> <uuid16> command
 *
 ****************************************************************************/

void btsak_cmd_discover(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  btsak_cmd_discover_common(btsak, argc, argv, GATT_DISCOVER);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_discover_characteristic
 *
 * Description:
 *   gatt [-h] characteristic [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_discover_characteristic(FAR struct btsak_s *btsak,
                                            int argc, FAR char *argv[])
{
  btsak_cmd_discover_common(btsak, argc, argv, GATT_DISCOVER_CHAR);
}

/****************************************************************************
 * Name: btsak_cmd_gat_discover_descriptor
 *
 * Description:
 *   gatt [-h] descriptor [-h] <addr> <addr-type> command
 *
 ****************************************************************************/

void btsak_cmd_gat_discover_descriptor(FAR struct btsak_s *btsak,
                                       int argc, FAR char *argv[])
{
  btsak_cmd_discover_common(btsak, argc, argv, GATT_DISCOVER_DESC);
}

/****************************************************************************
 * Name: btsak_cmd_gat_discover_get
 *
 * Description:
 *   gatt [-h] dget [-h]
 *
 ****************************************************************************/

void btsak_cmd_gat_discover_get(FAR struct btsak_s *btsak,
                                int argc, FAR char *argv[])
{
  FAR struct bt_discresonse_s *rsp;
  struct bt_discresonse_s result[8];
  struct btreq_s btreq;
  int sockfd;
  int ret;
  int i;

  /* Check for help command */

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc != 1)
    {
      fprintf(stderr, "ERROR:  No arguements expected\n", argc);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to start the discovery */

  strncpy(btreq.btr_name, btsak->ifname, HCI_DEVNAME_SIZE);
  btreq.btr_gnrsp = 8;
  btreq.btr_grsp  = result;

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTDISCGET, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTDISCGET) failed: %d\n", errno);
        }
      else
        {
          /* Show the results that we obtained */

          printf("Discovered:\n");
          for (i = 0; i < btreq.btr_gnrsp; i++)
            {
              rsp = &result[i];
              printf("%d.\thandle 0x%04x perm: %02x\n",
                     rsp->dr_handle, rsp->dr_perm);
            }
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read
 *
 * Description:
 *   gatt [-h] read [-h] <addr> <addr-type> <handle> [<offset>] command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read_multiple
 *
 * Description:
 *   gatt [-h] read-multiple [-h] <addr> <addr-type> <handle> <nitems> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read_multiple(FAR struct btsak_s *btsak, int argc,
                                  FAR char *argv[])
{
# warning Missing logic
}

/****************************************************************************
 * Name: btsak_cmd_gatt_write
 *
 * Description:
 *   gatt [-h] write [-h] [-h] <addr> <addr-type> <handle> <datum> command
 *
 ****************************************************************************/

void btsak_cmd_gatt_write(FAR struct btsak_s *btsak, int argc,
                          FAR char *argv[])
{
# warning Missing logic
}
