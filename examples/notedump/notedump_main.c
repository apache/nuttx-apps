/*****************************************************************************
 * apps/examples/notedump/notedump_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
 *****************************************************************************/

/*****************************************************************************
 * Included Files
 *****************************************************************************/

#include "notedump.h"

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <fcntl.h>

#define NOTE_MSG_SIZE CONFIG_SYSTEM_NOTE_BUFFERSIZE

/*****************************************************************************
 * Public Data
 *****************************************************************************/

/*****************************************************************************
 * Private Data
 *****************************************************************************/

static uint32_t g_udpserver_ipv4;
static uint32_t g_udpserver_port;
static uint32_t note_sent_bytes;

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/*****************************************************************************
 * show_usage
 *****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s [<server-addr>] [<server-port>]\n", progname);
  exit(1);
}

/*****************************************************************************
 * udp_cmdline
 *****************************************************************************/

bool udp_cmdline(int argc, char **argv)
{
  int ret;
  long port;
  char *endptr;

  /* Init the default IP address */

  g_udpserver_ipv4 = HTONL(0x01020304);
  g_udpserver_port = HTONS(1234);

  /* Currently only a single command line option is supported:  The server
   * IP address. Used to override default.
   */

  if (argc == 3)
    {
      /* Convert the <server-addr> argument into a binary address */

      ret = inet_pton(AF_INET, argv[1], &g_udpserver_ipv4);
      if (ret == 1)
        {
          fprintf(stderr, "Server address: %s\n", argv[1]);
        }
      else
        {
          fprintf(stderr, "ERROR: Invalid server address: %s\n", argv[1]);
          show_usage(argv[0]);
          return false;
        }

      port = strtol(argv[2], &endptr, 10);
      if (*endptr != '\0' || port < 1 || port > 65535)
        {
          fprintf(stderr, "ERROR: Invalid port: %s\n", argv[2]);
          show_usage(argv[0]);
          return false;
        }

      g_udpserver_port = (uint32_t)port;
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      show_usage(argv[0]);
      return false;
    }

  return true;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

static int create_socket(void)
{
  socklen_t addrlen;
  int sockfd;

  struct sockaddr_in addr;

  /* Create a new IPv4 UDP socket */

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("client ERROR: client socket failure %d\n", errno);
      return -1;
    }

  /* Bind the UDP socket to a IPv4 port */

  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(CONFIG_EXAMPLES_NOTEDUMP_SERVER_PORTNO);
  addr.sin_addr.s_addr = HTONL(INADDR_ANY);
  addrlen              = sizeof(struct sockaddr_in);

  if (bind(sockfd, (FAR struct sockaddr *)&addr, addrlen) < 0)
    {
      printf("client ERROR: Bind failure: %d\n", errno);
      return -1;
    }

  return sockfd;
}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************
 * main
 *****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  struct sockaddr_in server;
  unsigned char outbuf[NOTE_MSG_SIZE];
  socklen_t addrlen;
  int sockfd;
  int nbytes;
  int metric_freq = 0;

  /* Parse any command line options */

  if (udp_cmdline(argc, argv) == false)
    {
      fprintf(stderr, "ERROR: Invalid arguments\n");
      return -1;
    }

  /* Create a UDP socket */

  sockfd = create_socket();
  if (sockfd < 0)
    {
      printf("client ERROR: create_socket failed\n");
      exit(1);
    }

    int note_fd;

  /* Open the note driver */

  printf("notedump_daemon: Opening /dev/note/ram\n");
  note_fd = open("/dev/note/ram", O_RDONLY);
  if (note_fd < 0)
    {
      int errcode = errno;
      printf("notedump_daemon: ERROR: Failed to open /dev/note/ram: %d\n",
             errcode);
      return -1;
    }

  server.sin_family      = AF_INET;
  server.sin_port        = HTONS(CONFIG_EXAMPLES_NOTEDUMP_SERVER_PORTNO);
  server.sin_addr.s_addr = (in_addr_t)g_udpserver_ipv4;
  addrlen                = sizeof(struct sockaddr_in);

  printf("Sending note info to %s %ld\n", argv[1], g_udpserver_port);

  while (true)
    {
      /* Read from note to fill the output buffer */

      int note_bytes = read(note_fd, outbuf, CONFIG_SYSTEM_NOTE_BUFFERSIZE);

      /* Send the note message */

      nbytes = sendto(sockfd, outbuf, note_bytes, 0,
                      (struct sockaddr *)&server, addrlen);

      if (nbytes < 0)
        {
          printf("client: sendto failed: %d\n", errno);
          goto errout;
        }
      else if (nbytes != note_bytes)
        {
          printf("client: Bad send length: %d Expected: %d\n",
                 nbytes, note_bytes);
          goto errout;
        }

      note_sent_bytes += nbytes;
      metric_freq++;

      if (metric_freq == CONFIG_EXAMPLES_NOTEDUMP_METRIC_FREQ)
        {
          metric_freq = 0;
          printf("Sent %ld bytes\n", note_sent_bytes);
        }

      /* Now, sleep a bit. */

      sleep(1);
    }

errout:
  syslog(LOG_INFO, "note_daemon: Terminating\n");
  close(note_fd);
  close(sockfd);

  return 0;
}
