/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_info.c
 * Bluetooth Swiss Army Knife -- Info
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
