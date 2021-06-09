/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_advertise.c
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

static void btsak_cmd_advertisestart(FAR struct btsak_s *btsak,
                                     FAR char *cmd,
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

void btsak_cmd_advertise(FAR struct btsak_s *btsak,
                         int argc, FAR char *argv[])
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
      fprintf(stderr,
              "ERROR:  Unrecognized advertise command: %s\n", argv[argind]);
      btsak_advertise_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }
}
