/****************************************************************************
 * apps/netutils/wakeonlan/wol_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WOL_DEFAULT_PORT 9   /* Discard Protocol / Wake-on-LAN */
#define WOL_FRAME_SIZE   102 /* 6 + 16 * sizeof(struct ether_addr) */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int send_wol(FAR struct ether_addr *dest,
                    FAR struct sockaddr_in *s_in)
{
  int ret;
  unsigned int i;
  int sockfd;
  int optval = 1;
  uint8_t buffer[WOL_FRAME_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd < 0)
    {
      fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
      return -1;
    }

  ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
    (void *)&optval, sizeof(optval));
  if (ret < 0)
    {
      fprintf(stderr, "failed to set socket options: %s\n", strerror(errno));
      goto out;
    }

  for (i = 1; i <= 16; ++i)
    {
      *((FAR struct ether_addr *)&buffer[6*i]) = *dest;
    }

  ret = sendto(sockfd, buffer, WOL_FRAME_SIZE, 0,
      (FAR struct sockaddr *)s_in, sizeof(*s_in));
  if (ret < 0)
    {
      fprintf(stderr, "failed to send frame: %s\n", strerror(errno));
      goto out;
    }

out:
  close(sockfd);
  return ret ? -1 : 0;
}

static void show_usage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "Usage: %s [-i IP] [-p PORT] MAC_ADDRESS\n", progname);
  fprintf(stderr, "WakeOnLAN command:\n"
                  "  -i, destination IP address (default: 255.255.255.255)\n"
                  "  -p, destination port (default: 9)\n"
                  "\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int option;
  struct sockaddr_in s_in;
  struct ether_addr destmac;
  int exitcode = EXIT_FAILURE;

  bzero(&s_in, sizeof(s_in));
  s_in.sin_family = AF_INET;
  s_in.sin_port = htons(WOL_DEFAULT_PORT);
  s_in.sin_addr.s_addr = INADDR_BROADCAST;

  while ((option = getopt(argc, argv, "i:p:h")) != ERROR)
    {
      switch (option)
        {
          case 'i':
            ret = inet_pton(AF_INET, optarg, &s_in.sin_addr);
            if (ret <= 0)
              {
                fprintf(stderr, "invalid argument %s\n", optarg);
                goto errout_with_usage;
              }
            break;

          case 'p':
            ret = atoi(optarg);
            if (ret <= 0 || ret > UINT16_MAX)
              goto errout_with_usage;

            s_in.sin_port = htons(ret);
            break;

          case 'h':
            exitcode = EXIT_SUCCESS;
          default:
            goto errout_with_usage;
        }
    }

  if (optind >= argc ||
      ether_aton_r(argv[optind], &destmac) == NULL)
    {
      goto errout_with_usage;
    }

  if (send_wol(&destmac, &s_in))
    {
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;

errout_with_usage:
    show_usage(argv[0], exitcode);
    return exitcode;
}
