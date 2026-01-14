/****************************************************************************
 * apps/sensors/netsensor_stream/netsensor_stream_main.c
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
#define DEFAULT_IP "127.0.0.1"

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
  int sock;
  int fd;
  ssize_t bsent;
  struct orb_metadata meta;
  struct pollfd pfd;
  struct sockaddr_in addr; /* TODO: IPv6 support */
  struct sockaddr_in to;
  socklen_t addrlen = sizeof(addr);
  socklen_t tolen = sizeof(to);
  void *dbuf = NULL;

  const char *topic = NULL;
  uint16_t port = DEFAULT_PORT;
  const char *ipaddr = DEFAULT_IP;
  uint32_t ip;

  /* Get parameters for operation from the command line. */

  int c;
  while ((c = getopt(argc, argv, ":a:p:h")) != -1)
    {
      switch (c)
        {
        case 'h':
          print_usage(stdout);
          return EXIT_SUCCESS;
          break;
        case 'a':
          ipaddr = optarg;
          break;
        case 'p':
          port = strtoul(optarg, NULL, 10);
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

  fd = orb_subscribe(&meta);
  if (fd < 0)
    {
      fprintf(stderr, "Could not get subscribe to topic '%s'\n", topic);
      return EXIT_FAILURE;
    }

  /* Set up socket for sending data */

  addr.sin_family = AF_INET;
  addr.sin_port = 0; /* Allow any */
  addr.sin_addr.s_addr = HTONL(INADDR_ANY);

  /* Set up address where data is sent */

  err = inet_pton(AF_INET, ipaddr, &ip);
  if (err < 0)
    {
      fprintf(stderr, "IP address %s is invalid.\n", ipaddr);
      err = EXIT_FAILURE;
      goto cleanup_topic;
    }

  to.sin_family = AF_INET;
  to.sin_port = HTONL(port); /* Allow any */
  to.sin_addr.s_addr = (in_addr_t)ip;

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

  /* Allocate memory for receiving UDP messages containing data */

  dbuf = malloc(meta.o_size);
  if (dbuf == NULL)
    {
      syslog(LOG_ERR | LOG_DAEMON,
             "Couldn't allocate send buffer memory: %d\n", errno);
      err = EXIT_FAILURE;
      goto cleanup_sock;
    }

  /* Set up polling */

  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  /* Send data from the topic forever */

  syslog(LOG_INFO | LOG_DAEMON, "Streaming from %s to %s:%u\n", topic, ipaddr,
         port);

  for (;;)
    {
      /* Get data from topic whenever it's ready (poll returns) */

      err = poll(&pfd, 1, -1);
      if (err <= 0)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Failed to poll topic: %d\n", errno);
          continue; /* Try again */
        }

      err = orb_copy(&meta, fd, dbuf);
      if (err)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Couldn't get topic data: %d\n",
                 errno);
          continue;
        }

      /* Send over UDP */

      bsent =
          sendto(sock, dbuf, meta.o_size, 0, (struct sockaddr *)&to, tolen);
      if (bsent < 0)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Couldn't send UDP data: %d\n", errno);
          continue;
        }
      else if (bsent != meta.o_size)
        {
          syslog(LOG_ERR | LOG_DAEMON, "Sent UDP data of wrong size.\n");
          continue;
        }
    }

  free(dbuf);
cleanup_sock:
  close(sock);
cleanup_topic:
  orb_unsubscribe(fd);
  return err;
}
