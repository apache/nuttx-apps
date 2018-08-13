/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_advertise.c
 * Bluetooth Swiss Army Knife -- Advertise command
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
 * Name: btsak_advertise_showusage
 *
 * Description:
 *   Show usage of the advertise command
 *
 ****************************************************************************/

static void btsak_advertise_showusage(FAR const char *progname,
                                      FAR const char *cmd, int exitcode)
{
  fprintf(stderr, "%s:  Advertise commands:\n", cmd);
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "\t%s <ifname> %s [-h] <start|stop>\n",
          progname, cmd);
  fprintf(stderr, "\nWhere the options do the following:\n\n");
  fprintf(stderr, "\tstart\t- Starts advertising (type ADV_IND).\n");
  fprintf(stderr, "\tstop\t- Stops advertising\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: btsak_cmd_advertisestart
 *
 * Description:
 *   Advertise start command
 *
 ****************************************************************************/

static void btsak_cmd_advertisestart(FAR struct btsak_s *btsak, FAR char *cmd,
                                     int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  struct bt_eir_s ad[2];             /* Data for advertisement packets */
  struct bt_eir_s sd[2];             /* Data for scan response packets */
  int sockfd;
  int ret;

  /* REVISIT:  Should support all advertising type.  Only ADV_IND is
   * supported:
   *
   * ADV_IND
   *   Known as Advertising Indications (ADV_IND), where a peripheral device
   *   requests connection to any central device (i.e., not directed at a
   *   particular central device).
   *   Example:  A smart watch requesting connection to any central device.
   * ADV_DIRECT_IND
   *   Similar to ADV_IND, yet the connection request is directed at a
   *   specific central device.
   *   Example: A smart watch requesting connection to a specific central
   *   device.
   * ADV_NONCONN_IND
   *   Non connectible devices, advertising information to any listening
   *   device.
   *   Example:  Beacons in museums defining proximity to specific exhibits.
   * ADV_SCAN_IND
   *   Similar to ADV_NONCONN_IND, with the option additional information via
   *   scan responses.
   *   Example:  A warehouse pallet beacon allowing a central device to
   *   request additional information about the pallet.
   */

  /* The data for advertisement and response packets are provided as arrays
   * terminated by an entry with len=2.
   *
   * REVISIT:  To be useful for anything other than testing, there must
   * be some mechanism to specify the advertise and response data.
   */

  memset(&ad, 0, 2 * sizeof(struct bt_eir_s));
  ad[0].len     = 2;
  ad[0].type    = BT_EIR_FLAGS;
  ad[0].data[0] = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

  memset(&sd, 0, 2 * sizeof(struct bt_eir_s));
  sd[1].len         = sizeof("btsak");
  sd[1].type        = BT_EIR_NAME_COMPLETE;
  strcpy((FAR char *)sd[1].data, "btsak");

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);
  btreq.btr_advtype = BT_LE_ADV_IND;
  btreq.btr_advad   = ad;
  btreq.btr_advsd   = sd;

  /* Perform the IOCTL to start advertising */

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTADVSTART,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTADVSTART) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}

/****************************************************************************
 * Name: btsak_cmd_advertisestop
 *
 * Description:
 *   Advertise stop command
 *
 ****************************************************************************/

static void btsak_cmd_advertisestop(FAR struct btsak_s *btsak, FAR char *cmd,
                                    int argc, FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Perform the IOCTL to stop advertising */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTADVSTOP,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTADVSTOP) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_advertise
 *
 * Description:
 *   advertise [-h] <start [-d] |get|stop> command
 *
 ****************************************************************************/

void btsak_cmd_advertise(FAR struct btsak_s *btsak, int argc, FAR char *argv[])
{
  int argind;

  /* Verify that an option was provided */

  argind = 1;
  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing advertise command\n");
      btsak_advertise_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* Check for command */

  if (strcmp(argv[argind], "-h") == 0)
    {
      btsak_advertise_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }
  else if (strcmp(argv[argind], "start") == 0)
    {
      btsak_cmd_advertisestart(btsak, argv[0], argc - argind, &argv[argind]);
    }
  else if (strcmp(argv[argind], "stop") == 0)
    {
      btsak_cmd_advertisestop(btsak, argv[0], argc - argind, &argv[argind]);
    }
  else
    {
      fprintf(stderr, "ERROR:  Unrecognized advertise command: %s\n", argv[argind]);
      btsak_advertise_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }
}
