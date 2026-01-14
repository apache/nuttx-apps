/****************************************************************************
 * apps/sensors/netsensor/netsensor_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for asyslogditional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with the
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

#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <syslog.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <getopt.h>

#include <uORB/uORB.h>

#include "helptext.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_PORT (5555)
#define DEFAULT_QLEN (5)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(FILE *sink) { fprintf(sink, HELP_TEXT); }

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv)
{
  int err;
  int netfd;
  int sock;
  ssize_t brecvd;
  const struct orb_metadata *meta;
  struct sockaddr_in addr; /* TODO: IPv6 support */
  socklen_t addrlen = sizeof(addr);
  int sockoptval;
  void *dbuf = NULL;

  const char *topic = NULL;
  uint16_t port = DEFAULT_PORT;
  uint16_t queue_len = DEFAULT_QLEN;
  int devno = 0;
  bool usedevno = false;
  bool restamp = false;

  /* Get parameters for operation from the command line. */

  int c;
  while ((c = getopt(argc, argv, ":p:q:d:th")) != -1)
    {
      switch (c)
        {
        case 'h':
          print_usage(stdout);
          return EXIT_SUCCESS;
          break;
        case 'p':
          port = strtoul(optarg, NULL, 10);
          break;
        case 'd':
          usedevno = true;
          devno = atoi(optarg);
          break;
        case 'q':
          queue_len = strtoul(optarg, NULL, 10);
          break;
        case 't':
          restamp = true;
          break;
        case '?':
          fprintf(stderr, "Unknown option -%c\n", optopt);
          return EXIT_FAILURE;
          break;
        }
    }

  /* Get topic name */

  if (optind >= argc)
    {
      fprintf(stderr, "Missing topic.\n");
      print_usage(stderr);
      return EXIT_FAILURE;
    }

  topic = argv[optind];

  /* Get topic metadata */

  meta = orb_get_meta(topic);
  if (meta == NULL)
    {
      fprintf(stderr, "Could not get metadata for topic '%s'\n", topic);
      return EXIT_FAILURE;
    }

  /* Set up topic advertisement */

  if (usedevno)
    {
      netfd = orb_advertise_multi_queue(meta, NULL, &devno, queue_len);
    }
  else
    {
      netfd = orb_advertise_multi_queue(meta, NULL, NULL, queue_len);
    }

  if (netfd < 0)
    {
      fprintf(stderr, "Could not advertise topic %s: %d\n", topic, errno);
      return EXIT_FAILURE;
    }

  /* Set up socket for receiving data */

  addr.sin_family = AF_INET;
  addr.sin_port = HTONS(port);
  addr.sin_addr.s_addr = HTONL(INADDR_ANY);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      fprintf(stderr, "Couldn't create UDP socket: %d\n", errno);
      err = EXIT_FAILURE;
      goto cleanup_topic;
    }

  if (bind(sock, (struct sockaddr *)&addr, addrlen) < 0)
    {
      fprintf(stderr, "Couldn't bind UDP socket to port %u: %d\n", port,
              errno);
      err = EXIT_FAILURE;
      goto cleanup_sock;
    }

  sockoptval = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockoptval,
                 sizeof(sockoptval)) < 0)
    {
      fprintf(stderr, "setsockopt SO_REUSEADDR failure: %d\n", errno);
      err = EXIT_FAILURE;
      goto cleanup_sock;
    }

  /* Allocate memory for receiving UDP messages containing data */

  dbuf = malloc(meta->o_size);
  if (dbuf == NULL)
    {
      syslog(LOG_ERR | LOG_DAEMON,
             "Couldn't allocate recv buffer memory: %d\n", errno);
      err = EXIT_FAILURE;
      goto cleanup_sock;
    }

  /* Receive and publish data forever */

  syslog(LOG_INFO | LOG_DAEMON, "Starting netsensor %s on port %u\n", topic,
         port);
  for (;;)
    {
      brecvd = recv(sock, dbuf, meta->o_size, 0);
      if (brecvd < 0)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Could not receive data: %d\n", errno);
          continue;
        }

      if (brecvd != meta->o_size)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Received incorrect data size.\n");
          continue;
        }

      syslog(LOG_INFO | LOG_DAEMON, "Got data: %ld bytes\n", brecvd);

      /* If we're overwriting the timestamp, do that. We know that all uORB
       * topics start with a `uint64_t timestamp` member.
       */

      if (restamp)
        {
          *((uint64_t *)(dbuf)) = orb_absolute_time();
        }

      /* Publish the data to the topic */

      err = orb_publish(meta, netfd, dbuf);
      if (err < 0)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Could not publish data: %d\n", errno);
        }
    }

  free(dbuf);
cleanup_sock:
  close(sock);
cleanup_topic:
  orb_unadvertise(netfd);
  return err;
}
