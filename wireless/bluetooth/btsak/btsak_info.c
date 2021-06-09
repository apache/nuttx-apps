/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_info.c
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
 * Name: btsak_info_showusage
 *
 * Description:
 *   Show usage of the security command
 *
 ****************************************************************************/

static void btsak_info_showusage(FAR const char *progname,
                                 FAR const char *cmd, int exitcode)
{
  fprintf(stderr, "%s:\tShow Bluetooth device information\n", cmd);
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "\t%s <ifname> %s [-h]\n",
          progname, cmd);
  exit(exitcode);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_info
 *
 * Description:
 *   security command
 *
 ****************************************************************************/

void btsak_cmd_info(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Check for help */

  if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
      btsak_info_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  /* Perform the IOCTL to stop advertising */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCGBTINFO, (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCGBTINFO) failed: %d\n",
                  errno);
        }
      else
        {
          printf("Device: %s\n", btsak->ifname);
          printf("BDAddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                btreq.btr_bdaddr.val[5], btreq.btr_bdaddr.val[4],
                btreq.btr_bdaddr.val[3], btreq.btr_bdaddr.val[2],
                btreq.btr_bdaddr.val[1], btreq.btr_bdaddr.val[0]);
          printf("Flags:  %04x\n", btreq.btr_flags);
          printf("Free:   %u\n", btreq.btr_num_cmd);
          printf("  ACL:  %u\n", btreq.btr_num_acl);
          printf("  SCO:  %u\n", btreq.btr_num_sco);
          printf("Max:\n");
          printf("  ACL:  %u\n", btreq.btr_max_acl);
          printf("  SCO:  %u\n", btreq.btr_max_sco);
          printf("MTU:\n");
          printf("  ACL:  %u\n", btreq.btr_acl_mtu);
          printf("  SCO:  %u\n", btreq.btr_sco_mtu);
          printf("Policy: %u\n", btreq.btr_link_policy);
          printf("Type:   %u\n", btreq.btr_packet_type);
        }
    }

  close(sockfd);
}
