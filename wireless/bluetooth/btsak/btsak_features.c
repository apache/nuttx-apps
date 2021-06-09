/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_features.c
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

#include <nuttx/wireless/bluetooth/bt_core.h>
#include <nuttx/wireless/bluetooth/bt_hci.h>
#include <nuttx/wireless/bluetooth/bt_ioctl.h>

#include "btsak.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_features_showusage
 *
 * Description:
 *   Show usage of the security command
 *
 ****************************************************************************/

static void btsak_features_showusage(FAR const char *progname,
                                 FAR const char *cmd, int exitcode)
{
  fprintf(stderr, "%s:\tShow Bluetooth device features\n", cmd);
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "\t%s <ifname> %s [-h] [le]\n\n",
          progname, cmd);
  fprintf(stderr, "Where\n");
  fprintf(stderr, "\tSelects LE features (vs BR/EDR features)\n");
  exit(exitcode);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_features
 *
 * Description:
 *   security command
 *
 ****************************************************************************/

void btsak_cmd_features(FAR struct btsak_s *btsak,
                        int argc,
                        FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int cmd;
  int ret;

  /* Check for help or LE options */

  cmd = SIOCGBTFEAT;

  if (argc > 1)
    {
      if (strcmp(argv[1], "-h") == 0)
        {
          btsak_features_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
        }
      else if (strcmp(argv[1], "le") == 0)
        {
          cmd = SIOCGBTLEFEAT;
        }
    }

  /* Perform the IOCTL to stop advertising */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, cmd, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: ioctl(%04x) failed: %d\n",
                  cmd, errno);
        }
      else
        {
          printf("Page 0:\n");
          printf("\t%02x %02x %02x %02x  %02x %02x %02x %02x\n",
                 btreq.btr_features0[0], btreq.btr_features0[1],
                 btreq.btr_features0[2], btreq.btr_features0[3],
                 btreq.btr_features0[4], btreq.btr_features0[5],
                 btreq.btr_features0[6], btreq.btr_features0[7]);
          printf("Page 1:\n");
          printf("\t%02x %02x %02x %02x  %02x %02x %02x %02x\n",
                 btreq.btr_features1[0], btreq.btr_features1[1],
                 btreq.btr_features1[2], btreq.btr_features1[3],
                 btreq.btr_features1[4], btreq.btr_features1[5],
                 btreq.btr_features1[6], btreq.btr_features1[7]);
          printf("Page 2:\n");
          printf("\t%02x %02x %02x %02x  %02x %02x %02x %02x\n",
                 btreq.btr_features2[0], btreq.btr_features2[1],
                 btreq.btr_features2[2], btreq.btr_features2[3],
                 btreq.btr_features2[4], btreq.btr_features2[5],
                 btreq.btr_features2[6], btreq.btr_features2[7]);
        }
    }

  close(sockfd);
}
