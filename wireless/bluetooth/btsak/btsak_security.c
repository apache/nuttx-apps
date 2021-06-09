/****************************************************************************
 * apps/wireless/bluetooth/btsak/btsak_security.c
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
 * Name: btsak_security_showusage
 *
 * Description:
 *   Show usage of the security command
 *
 ****************************************************************************/

static void btsak_security_showusage(FAR const char *progname,
                                     FAR const char *cmd, int exitcode)
{
  fprintf(stderr, "%s:\tEnable security (encryption) for a connection:\n",
          cmd);
  fprintf(stderr,
          "\tIf device is paired, key encryption will be enabled. If the "
          "link\n");
  fprintf(stderr,
          "\tis already encrypted with sufficiently strong key this "
          "function\n");
  fprintf(stderr,
          "\tdoes nothing.\n\n");
  fprintf(stderr,
          "\tIf the device is not paired pairing will be initiated. If the "
          "device\n");
  fprintf(stderr,
          "\tis paired and keys are too weak but input output capabilities "
          "allow\n");
  fprintf(stderr,
          "\tfor strong enough keys pairing will be initiated.\n\n");
  fprintf(stderr,
          "\tThis function may return error if required level of security "
          "is not\n");
  fprintf(stderr,
          "\tpossible to achieve due to local or remote device limitation "
          "(eg input\n");
  fprintf(stderr,
          "\toutput capabilities).\n\n");
  fprintf(stderr, "Usage:\n\n");
  fprintf(stderr, "\t%s <ifname> %s [-h] <addr> public|private <level>\n",
          progname, cmd);
  fprintf(stderr,
          "\nWhere:\n\n");
  fprintf(stderr,
          "\t<addr>\t- The 6-byte address of the connected peer\n");
  fprintf(stderr,
          "\t<level>\t- Security level, on of:\n\n");
  fprintf(stderr,
          "\t\tlow\t- No encryption and no authentication\n");
  fprintf(stderr,
          "\t\tmedium\t- Encryption and no authentication (no MITM)\n");
  fprintf(stderr,
          "\t\thigh\t- Encryption and authentication (MITM)\n");
  fprintf(stderr,
          "\t\tfips\t- Authenticated LE secure connections and "
          "encryption\n");
  exit(exitcode);
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * Name: btsak_cmd_security
 *
 * Description:
 *   security [-h] <start [-d] |get|stop> command
 *
 ****************************************************************************/

void btsak_cmd_security(FAR struct btsak_s *btsak, int argc,
                        FAR char *argv[])
{
  struct btreq_s btreq;
  int sockfd;
  int ret;

  /* Check for help */

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Missing required arguments\n");
      btsak_security_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  if (strcmp(argv[1], "-h") == 0)
    {
      btsak_security_showusage(btsak->progname, argv[0], EXIT_SUCCESS);
    }

  /* Verify that all required arguments were provided */

  if (argc < 4)
    {
      fprintf(stderr, "ERROR:  Missing required arguments\n");
      btsak_security_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* The first argument must be an address of the form xx:xx:xx:xx:xx:xx */

  memset(&btreq, 0, sizeof(struct btreq_s));
  strncpy(btreq.btr_name, btsak->ifname, IFNAMSIZ);

  ret = btsak_str2addr(argv[1], btreq.btr_secaddr.val);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Invalid address string: %s\n", argv[1]);
      btsak_security_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* The second address is the address type, either "public" or "random" */

  ret = btsak_str2addrtype(argv[2], &btreq.btr_secaddr.type);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR:  Invalid address type: %s\n", argv[2]);
      btsak_security_showusage(btsak->progname, argv[0], EXIT_FAILURE);
    }

  /* The third argument is the security level */

  ret = btsak_str2seclevel(argv[3], &btreq.btr_seclevel);

  /* Perform the IOCTL to stop advertising */

  sockfd = btsak_socket(btsak);
  if (sockfd >= 0)
    {
      ret = ioctl(sockfd, SIOCBTSECURITY,
                  (unsigned long)((uintptr_t)&btreq));
      if (ret < 0)
        {
          fprintf(stderr, "ERROR:  ioctl(SIOCBTSECURITY) failed: %d\n",
                  errno);
        }
    }

  close(sockfd);
}
