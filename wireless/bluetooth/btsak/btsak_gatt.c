/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_gatt.c
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

/* Based loosely on the i8sak IEEE 802.15.4 program by Anthony Merlino and
 * Sebastien Lorquet.  Commands inspired from btshell example in the
 * Intel/Zephyr Arduino 101 package (BSD license).
 */

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
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
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
  FAR struct bt_discresonse_s *rsp;
  struct bt_discresonse_s result[CONFIG_BLUETOOTH_MAXDISCOVER];
  int i;

  /* Check for help command */

  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  argndx = (type == GATT_DISCOVER) ? 4 : 3;
  if (argc < argndx)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  "
              "Found %d expected at least %u\n",
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
  if (ret < 0)
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

  btreq.btr_gnrsp = CONFIG_BLUETOOTH_MAXDISCOVER;
  btreq.btr_grsp  = result;
  btreq.btr_indx  = 0;

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTDISCOVER,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTDISCOVER) failed: %d\n",
                  errno);
        }
      else
        {
          /* Show the results that we obtained */

          printf("Discovered %d handles:\n", btreq.btr_gnrsp);
          for (i = 0; i < btreq.btr_gnrsp; i++)
            {
              rsp = &result[i];
              printf("%d.\thandle 0x%04x perm: %02x\n",
                     i, rsp->dr_handle, rsp->dr_perm);
            }
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_connect_common
 *
 * Description:
 *   Function used by the connect and disconnect commands.
 *
 ****************************************************************************/

static void btsak_cmd_connect_common(FAR struct btsak_s *btsak, int argc,
                                     FAR char *argv[], int cmd)
{
  static  struct btreq_s btreq;
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

  ret = btsak_str2addr(argv[1], &btreq.btr_rmtpeer.val[0]);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for <addr>: %s\n", argv[1]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  ret = btsak_str2addrtype(argv[2], &btreq.btr_rmtpeer.type);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to start/end the connection */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, cmd, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBT%sCONNECT) failed: %d\n",
                  cmd == SIOCBTCONNECT ? "" : "DIS", errno);
        }
      else
        {
          printf("Connect pending...\n");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_read_common
 *
 * Description:
 *   Function used by the read and read_multiple commands.
 *
 ****************************************************************************/

static void btsak_cmd_read_common(FAR struct btsak_s *btsak, int argc,
                                     FAR char *argv[], bool multiple)
{
  int i;
  int j;
  int ret;
  int sockfd;
  uint8_t data[HCI_GATTRD_DATA];
  struct btreq_s btreq;

  memset(&btreq, 0, sizeof(struct btreq_s));

  if (argc < 4 || argc > 5)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  "
              "Found %d expected 3 or 4\n",
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
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btreq.btr_rdoffset     = 0;
  btreq.btr_rdnhandles   = multiple ? argc - 3 : 1;

  if (!multiple && argc > 4)
    {
      btreq.btr_rdoffset = btsak_str2uint16(argv[4]);
    }

  for (i = 0; i < btreq.btr_rdnhandles; i++)
    {
      btreq.btr_rdhandles[i] = btsak_str2uint16(argv[i + 3]);
    }

  /* Perform the IOCTL to start the read */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
  btreq.btr_rdsize = HCI_GATTRD_DATA;
  btreq.btr_rddata = data;

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTGATTRD, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTGATTRD) failed: %d\n", errno);
        }
      else
        {
          /* Show the results that we obtained */

          printf("Read %d bytes:\n", btreq.btr_rdsize);
          for (i = 0; i < btreq.btr_rdsize; i += 16)
            {
              for (j = 0; j < 16 && (i + j) < btreq.btr_rdsize; j++)
                {
                  if (j == 8)
                    {
                      putchar(' ');
                    }

                  printf(" %02x", data[i + j]);
                }

              putchar('\n');
            }
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
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Perform the IOCTL to start the MTU exchange */

  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTEXCHANGE,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTEXCHANGE) failed: %d\n",
                  errno);
        }
      else
        {
          printf("MTU Exchange %s\n",
                 btreq.btr_exresult == 0 ? "succeeded" : "failed");
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

void btsak_cmd_discover(FAR struct btsak_s *btsak, int argc,
                        FAR char *argv[])
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
 * Name: btsak_cmd_gatt_read
 *
 * Description:
 *   gatt [-h] read [-h] <addr> public|private <handle> [<offset>] command
 *
 ****************************************************************************/

void btsak_cmd_gatt_read(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  btsak_cmd_read_common(btsak, argc, argv, false);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_read_multiple
 *
 * Description:
 *   gatt [-h] read-multiple [-h] <addr> public|private <handle>
 *        [<handle> [<handle>]..]
 *
 ****************************************************************************/

void btsak_cmd_gatt_read_multiple(FAR struct btsak_s *btsak, int argc,
                                  FAR char *argv[])
{
  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  if (argc < 4)
    {
      fprintf(stderr,
              "ERROR:  Invalid number of arguments.  "
              "Found %d expected at least 3\n",
              argc - 1);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btsak_cmd_read_common(btsak, argc, argv, true);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_write
 *
 * Description:
 *   gatt [-h] write [-h] [-h] <addr> public|private <handle> <byte>
 *        [<byte> [<byte>]..]
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
              "ERROR:  Invalid number of arguments.  "
              "Found %d expected at least 4\n",
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
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Bad value for address type: %s\n", argv[2]);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  btreq.btr_wrhandle = btsak_str2uint16(argv[3]);
  btreq.btr_wrnbytes = argc - 4;

  if (btreq.btr_wrnbytes > HCI_GATTWR_DATA)
    {
      fprintf(stderr, "ERROR:  Too much data.  Limit is %u bytes\n",
              HCI_GATTWR_DATA);
      btsak_gatt_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  for (i = 0; i < btreq.btr_wrnbytes; i++)
    {
      btreq.btr_wrdata[i] = btsak_str2uint8(argv[i + 4]);
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
          printf("Write %s\n",
                 btreq.btr_wrresult == 0 ? "succeeded" : "failed");
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_gatt_connect
 *
 * Description:
 *   gatt [-h] connect [-h] <addr> public|private
 *
 ****************************************************************************/

void btsak_cmd_connect(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
  btsak_cmd_connect_common(btsak, argc, argv, SIOCBTCONNECT);
}

/****************************************************************************
 * Name: btsak_cmd_gatt_connect
 *
 * Description:
 *   gatt [-h] disconnect [-h] <addr> public|private
 *
 ****************************************************************************/

void btsak_cmd_disconnect(FAR struct btsak_s *btsak, int argc,
                         FAR char *argv[])
{
  btsak_cmd_connect_common(btsak, argc, argv, SIOCBTDISCONNECT);
}
