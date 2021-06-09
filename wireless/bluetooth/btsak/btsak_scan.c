/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_scan.c
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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>

#include <nuttx/wireless/bluetooth/bt_ioctl.h>

#include "btsak.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_scan_showusage
 *
 * Description:
 *   Show usage of the scan command
 *
 ****************************************************************************/

static void btsak_scan_showusage(FAR const char *progname,
                                 FAR const char *cmd, int exitcode)
{
  fprintf(stderr, "%s:  Scan commands:\n", cmd);
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "\t%s <ifname> %s [-h] <start [-d]|get|stop>\n",
          progname, cmd);
  fprintf(stderr, "\nWhere the options do the following:\n\n");
  fprintf(stderr,
          "\tstart\t- Starts scanning.  The -d option enables duplicate\n");
  fprintf(stderr, "\t\t  filtering.\n");
  fprintf(stderr, "\tget\t- Shows new accumulated scan results\n");
  fprintf(stderr, "\tstop\t- Stops scanning\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: btsak_cmd_scanstart
 *
 * Description:
 *   Scan start command
 *
 ****************************************************************************/

static void btsak_cmd_scanstart(FAR struct btsak_s *btsak, FAR char *cmd,
                                int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  int argind;
  int sockfd;
  int ret;

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  /* Check if an option was provided */

  argind = 1;
  btreq.btr_dupenable = false;

  if (argc > 1)
    {
      if (strcmp(argv[argind], "-d") == 0)
        {
          btreq.btr_dupenable = true;
        }
      else
        {
          fprintf(stderr, "ERROR:  Unrecognized option: %s\n",
                  argv[argind]);
          btsak_scan_showusage(btsak->progname, cmd, EXIT_FAILURE);
        }
    }

  /* Perform the IOCTL to start scanning */

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTSCANSTART,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTSCANSTART) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}

/****************************************************************************
 * Name: btsak_cmd_scanget
 *
 * Description:
 *   Scan get command
 *
 ****************************************************************************/

static void btsak_cmd_scanget(FAR struct btsak_s *btsak, FAR char *cmd,
                              int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  struct bt_scanresponse_s result[5];
  int sockfd;
  int ret;

  /* Perform the IOCTL to get the scan results so far */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
  btreq.btr_nrsp = 5;
  btreq.btr_rsp  = result;

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTSCANGET,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTSCANGET) failed: %d\n",
                  errno);
        }

      /* Show scan results */

      else
        {
          FAR struct bt_scanresponse_s *rsp;
          int i;
          int j;
          int k;

          printf("Scan result:\n");
          for (i = 0; i < btreq.btr_nrsp; i++)
            {
              rsp = &result[i];
              printf("%2d.\taddr:           "
                     "%02x:%02x:%02x:%02x:%02x:%02x type: %d\n",
                     i + 1,
                     rsp->sr_addr.val[5], rsp->sr_addr.val[4],
                     rsp->sr_addr.val[3], rsp->sr_addr.val[2],
                     rsp->sr_addr.val[1], rsp->sr_addr.val[0],
                     rsp->sr_addr.type);
              printf("\trssi:            %d\n", rsp->sr_rssi);
              printf("\tresponse type:   %u\n", rsp->sr_type);
              printf("\tadvertiser data:");

              for (j = 0; j < rsp->sr_len; j += 16)
                {
                  if (j > 0)
                    {
                      printf("\t                ");
                    }

                  for (k = 0; k < 16 && (j + k) < rsp->sr_len; k++)
                    {
                      printf(" %02x", rsp->sr_data[j + k]);
                    }

                  printf("\n");
                }
            }
        }

      close(sockfd);
    }
}

/****************************************************************************
 * Name: btsak_cmd_scanstop
 *
 * Description:
 *   Scan stop command
 *
 ****************************************************************************/

static void btsak_cmd_scanstop(FAR struct btsak_s *btsak, FAR char *cmd,
                              int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Perform the IOCTL to stop scanning and flush any buffered responses. */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTSCANSTOP,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTSCANSTOP) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_scan
 *
 * Description:
 *   scan [-h] <start [-d] |get|stop> command
 *
 ****************************************************************************/

void btsak_cmd_scan(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  int argind;

  /* Verify that an option was provided */

  argind = 1;
  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing scan command\n");
      btsak_scan_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Check for command */

  if (strcmp(argv[argind], "-h") == 0)
    {
      btsak_scan_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }
  else if (strcmp(argv[argind], "start") == 0)
    {
      btsak_cmd_scanstart(btsak, argv[0], argc - argind, &argv[argind]);
    }
  else if (strcmp(argv[argind], "get") == 0)
    {
      btsak_cmd_scanget(btsak, argv[0], argc - argind, &argv[argind]);
    }
  else if (strcmp(argv[argind], "stop") == 0)
    {
      btsak_cmd_scanstop(btsak, argv[0], argc - argind, &argv[argind]);
    }
  else
    {
      fprintf(stderr,
              "ERROR:  Unrecognized scan command: %s\n", argv[argind]);
      btsak_scan_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }
}
