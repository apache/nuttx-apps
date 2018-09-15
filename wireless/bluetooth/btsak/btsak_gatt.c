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

#include <nuttx/wireless/bluetooth/bt_core.h>
#include <nuttx/wireless/bluetooth/bt_gatt.h>
#include <nuttx/wireless/bluetooth/bt_ioctl.h>

#include "btsak.h"

/****************************************************************************
 * Private data
 ****************************************************************************/

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_discover_common
 *
 * Description:
 *   gatt [-h] <discover-cmd> [-h] <addr> public|private [<uuid16>]
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
              argc - 1, argndx - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
  btreq.btr_dtype = (uint8_t)type;

  ret = btsak_str2addr(argv[1], btreq.btr_dpeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_dpeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
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
      else
        {
          printf("Discovery in progress..\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_gatt_exchange_mtu
 *
 * Description:
 *   gatt [-h] exchange_mtu [-h] <addr> public|private command
 *
 ****************************************************************************/

void btsak_cmd_gatt_exchange_mtu(FAR struct btsak_s *btsak, int argc,
                                 FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc != 3)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  Found %d expected 2\n",
              argc - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addr(argv[1], btreq.btr_expeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_expeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to start the MTU exchange */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTEXCHANGE, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTEXCHANGE) failed: %d\n",
                  errno);
        }
      else
        {
          printf("MTU exchange pending...\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_exchange_mtu_result
 *
 * Description:
 *   gatt [-h] mget [-h] command
 *
 ****************************************************************************/

void btsak_cmd_gatt_exchange_mtu_result(FAR struct btsak_s *btsak, int argc,
                                        FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Perform the IOCTL to recover the result of the MTU exchange */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTEXRESULT, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTEXRESULT) failed: %d\n",
                  errno);
        }
      else
        {
          printf("MTU Exchange Result:\n");
          if (btreq.btr_expending)
            {
              printf("\tState:  Pending\n");
              printf("\tResult: Not yet available\n");
            }
          else
            {
              printf("\tState:  Complete\n");
              printf("\tResult: %u\n", btreq.btr_exresult);
            }
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_discover
 *
 * Description:
 *   gatt [-h] discover [-h] <addr> public|private <uuid16> command
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
 *   gatt [-h] characteristic [-h] <addr> public|private command
 *
 ****************************************************************************/

void btsak_cmd_gatt_discover_characteristic(FAR struct btsak_s *btsak,
                                            int argc, FAR char *argv[])
{
  btsak_cmd_discover_common(btsak, argc, argv, GATT_DISCOVER_CHAR);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_discover_descriptor
 *
 * Description:
 *   gatt [-h] descriptor [-h] <addr> public|private command
 *
 ****************************************************************************/

void btsak_cmd_gatt_discover_descriptor(FAR struct btsak_s *btsak,
                                        int argc, FAR char *argv[])
{
  btsak_cmd_discover_common(btsak, argc, argv, GATT_DISCOVER_DESC);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_discover_get
 *
 * Description:
 *   gatt [-h] dget [-h]
 *
 ****************************************************************************/

void btsak_cmd_gatt_discover_get(FAR struct btsak_s *btsak,
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
      fprintf(stderr, "ERROR:  No arguments expected\n");
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to get the result of the discovery */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
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
 *   gatt [-h] read [-h] <addr> public|private <handle> [<offset>] command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc < 4 || argc > 5)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  Found %d expected 3 or 4\n",
              argc - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addr(argv[1], btreq.btr_rdpeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_rdpeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btreq.btr_rdhandles[0] = btsak_str2uint16(argv[3]);
  btreq.btr_rdnhandles   = 1;

  btreq.btr_rdoffset     = 0;
  if (argc > 4)
    {
      btreq.btr_rdoffset = btsak_str2uint16(argv[4]);
    }

  /* Perform the IOCTL to start the read */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTRD, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTRD) failed: %d\n",
                  errno);
        }
      else
        {
          printf("Read pending...\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read_multiple
 *
 * Description:
 *   gatt [-h] read-multiple [-h] <addr> public|private <handle> [<handle> [<handle>]..]
 *
 ****************************************************************************/

void btsak_cmd_gatt_read_multiple(FAR struct btsak_s *btsak, int argc,
                                  FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;
  int i;

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc < 4)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  Found %d expected at least 3\n",
              argc - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addr(argv[1], btreq.btr_rdpeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_rdpeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btreq.btr_rdoffset     = 0;
  btreq.btr_rdnhandles   = argc - 3;

  for (i = 0; i < btreq.btr_rdnhandles; i++)
    {
      btreq.btr_rdhandles[i] = btsak_str2uint16(argv[i + 3]);
    }

  /* Perform the IOCTL to start the read */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTRD, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTRD) failed: %d\n",
                  errno);
        }
      else
        {
          printf("Read pending...\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read_get
 *
 * Description:
 *   gatt [-h] rget [-h]
 *
 ****************************************************************************/

void btsak_cmd_gatt_read_get(FAR struct btsak_s *btsak, int argc,
                             FAR char *argv[])
{
  struct btreq_s btreq;
  uint8_t data[HCI_GATTRD_DATA];
  int sockfd;
  int ret;
  int i;
  int j;

  /* Check for help command */

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc != 1)
    {
      fprintf(stderr, "ERROR:  No arguments expected\n");
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to start the discovery */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
  btreq.btr_rdsize = HCI_GATTRD_DATA;
  btreq.btr_rddata = data;

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTRDGET, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTRDGET) failed: %d\n", errno);
        }

      /* Has the read completed? */

      else if (btreq.btr_rdpending)
        {
          printf("Read pending.  Data not yet available.\n");
        }
      else
        {
          /* Show the results that we obtained */

          printf("Read:\n");
          for (i = 0; i < btreq.btr_rdsize; i += 16)
            {
              for (j = 0; j < 16 && (i + j) < btreq.btr_rdsize; j++)
                {
                  if (j == 8)
                    {
                      putchar(' ');
                    }

                  printf(" %02x", data[i]);
                }

              putchar('\n');
            }
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_write
 *
 * Description:
 *   gatt [-h] write [-h] [-h] <addr> public|private <handle> <byte> [<byte> [<byte>]..]
 *
 ****************************************************************************/

void btsak_cmd_gatt_write(FAR struct btsak_s *btsak, int argc,
                          FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;
  int i;

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc < 5)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  Found %d expected at least 4\n",
              argc - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addr(argv[1], btreq.btr_wrpeer.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_wrpeer.type);
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btreq.btr_wrhandle = btsak_str2uint16(argv[3]);
  btreq.btr_wrnbytes = argc - 3;

  if (btreq.btr_wrnbytes > HCI_GATTWR_DATA)
    {
      fprintf(stderr, "ERROR:  Too much data.  Limit is %u bytes%s\n",
              HCI_GATTWR_DATA);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  for (i = 0; i < btreq.btr_wrnbytes; i++)
    {
      btreq.btr_wrdata[i] = btsak_str2uint8(argv[i + 3]);
    }

  /* Perform the IOCTL to start the read */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTWR, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTWR) failed: %d\n",
                  errno);
        }
      else
        {
          printf("Write pending...\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_write_get
 *
 * Description:
 *   gatt [-h] dget [-h]
 *
 ****************************************************************************/

void btsak_cmd_gatt_write_get(FAR struct btsak_s *btsak, int argc,
                              FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Perform the IOCTL to recover the result of the write operation */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTWRGET, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTWRGET) failed: %d\n",
                  errno);
        }
      else
        {
          printf("Write Result:\n");
          if (btreq.btr_wrpending)
            {
              printf("\tState:  Pending\n");
              printf("\tResult: Not yet available\n");
            }
          else
            {
              printf("\tState:  Complete\n");
              printf("\tResult: %u\n", btreq.btr_wrresult);
            }
        }

      close(sockfd);
    }
}
