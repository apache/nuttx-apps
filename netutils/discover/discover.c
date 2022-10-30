/****************************************************************************
 * apps/netutils/discover/discover.c
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

#include <debug.h>
#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/discover.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_DISCOVER_STACK_SIZE
#  define CONFIG_DISCOVER_STACK_SIZE 1024
#endif

#ifndef CONFIG_DISCOVER_PRIORITY
#  define CONFIG_DISCOVER_PRIORITY SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_DISCOVER_PORT
#  define CONFIG_DISCOVER_PORT 96
#endif

#ifndef CONFIG_DISCOVER_INTERFACE
#  define CONFIG_DISCOVER_INTERFACE "eth0"
#endif

#ifndef CONFIG_DISCOVER_DEVICE_CLASS
#  define CONFIG_DISCOVER_DEVICE_CLASS DISCOVER_ALL
#endif

#ifndef CONFIG_DISCOVER_DESCR
#  define CONFIG_DISCOVER_DESCR CONFIG_ARCH_BOARD
#endif

/* Internal Definitions *****************************************************/

/* Discover request packet format:
 * Byte Description
 * 0    Protocol identifier (0x99)
 * 1    Request command 0x01
 * 2    Destination device class (For querying subsets of available devices)
 *      0xff for all devices
 * 3    Checksum (Byte 0 - Byte 1 - Byte n) & 0xff
 */

/* Discover response packet format:
 * Byte Description
 * 0    Protocol identifier (0x99)
 * 1    Response command (0x02)
 * 2-33 Device description string with 0 bytes filled
 * 34   Checksum (Byte 0 - Byte 1 - Byte n) & 0xff
 */

#define DISCOVER_PROTO_ID 0x99
#define DISCOVER_REQUEST 0x01
#define DISCOVER_RESPONSE 0x02
#define DISCOVER_ALL 0xff
#define DISCOVER_REQUEST_SIZE 4
#define DISCOVER_RESPONSE_SIZE 35

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef uint8_t request_t[DISCOVER_REQUEST_SIZE];
typedef uint8_t response_t[DISCOVER_RESPONSE_SIZE];

struct discover_state_s
{
  struct discover_info_s info;
  in_addr_t serverip;
  request_t request;
  response_t response;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct discover_state_s g_state =
{
  {CONFIG_DISCOVER_DEVICE_CLASS, CONFIG_DISCOVER_DESCR}
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int discover_daemon(int argc, char *argv[]);
static inline int discover_socket(void);
static inline int discover_openlistener(void);
static inline int discover_openresponder(void);
static inline int discover_parse(request_t packet);
static inline int discover_respond(in_addr_t *ipaddr);
static inline void discover_initresponse(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void discover_initresponse(void)
{
  int chk = 0;
  int i;

  g_state.response[0] = DISCOVER_PROTO_ID;
  g_state.response[1] = DISCOVER_RESPONSE;

  strncpy((char *)&g_state.response[2], g_state.info.description,
          DISCOVER_RESPONSE_SIZE - 3);

  for (i = 0; i < DISCOVER_RESPONSE_SIZE - 1; i++)
    {
      chk -= g_state.response[i];
    }

  /* Append check sum */

  g_state.response[DISCOVER_RESPONSE_SIZE - 1] = chk & 0xff;
}

static int discover_daemon(int argc, char *argv[])
{
  int sockfd = -1;
  int nbytes;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in srcaddr;

  /* memset(&g_state, 0, sizeof(struct discover_state_s)); */

  discover_initresponse();

  ninfo("Started\n");

  for (; ; )
    {
      /* Create a socket to listen for requests from DHCP clients */

      if (sockfd < 0)
        {
          sockfd = discover_openlistener();
          if (sockfd < 0)
            {
                nerr("ERROR: Failed to create socket\n");
                break;
            }
        }

      /* Read the next packet */

      nbytes = recvfrom(sockfd, &g_state.request, sizeof(g_state.request), 0,
                        (struct sockaddr *)&srcaddr, &addrlen);
      if (nbytes < 0)
        {
          /* On errors (other EINTR), close the socket and try again */

          nerr("ERROR: recv failed: %d\n", errno);
          if (errno != EINTR)
            {
              close(sockfd);
              sockfd = -1;
            }

          continue;
        }

      if (discover_parse(g_state.request) != OK)
        {
          continue;
        }

      ninfo("Received discover from %08" PRIx32 "\n",
            srcaddr.sin_addr.s_addr);

      discover_respond(&srcaddr.sin_addr.s_addr);
    }

  return OK;
}

static inline int discover_parse(request_t packet)
{
  int i;
  uint8_t chk = 0;

  if (packet[0] != DISCOVER_PROTO_ID)
    {
      nerr("ERROR: Wrong protocol id: %d\n", packet[0]);
      return ERROR;
    }

  if (packet[1] != DISCOVER_REQUEST)
    {
      nerr("ERROR: Wrong command: %d\n", packet[1]);
      return ERROR;
    }

  if (packet[2] == 0xff || packet[2] == g_state.info.devclass)
    {
      for (i = 0; i < DISCOVER_REQUEST_SIZE - 1; i++)
        chk -= packet[i];

      if ((chk & 0xff) != packet[3])
        {
          nerr("ERROR: Checksum does not match: %d\n", packet[3]);
          return ERROR;
        }
      else
        {
          return OK;
        }
    }

  return ERROR;
}

static inline int discover_respond(in_addr_t *ipaddr)
{
  struct sockaddr_in addr;
  int sockfd;
  int ret;

  sockfd = discover_openresponder();
  if (sockfd < 0)
    {
      nerr("ERROR: discover_openresponder failed\n");
      return ERROR;
    }

  /* Then send the response to the DHCP client port at that address */

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(CONFIG_DISCOVER_PORT);
  addr.sin_addr.s_addr = *ipaddr;

  ret = sendto(sockfd, &g_state.response, sizeof(g_state.response), 0,
               (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      nerr("ERROR: Could not send discovery response: %d\n", errno);
    }

  close(sockfd);
  return ret;
}

static inline int discover_socket(void)
{
  int sockfd;
#if defined(HAVE_SO_REUSEADDR) || defined(HAVE_SO_BROADCAST)
  int optval;
  int ret;
#endif

  /* Create a socket to listen for requests from DHCP clients */

  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Configure the socket */

#ifdef HAVE_SO_REUSEADDR
  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET,
                   SO_REUSEADDR, &optval, sizeof(int));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SO_REUSEADDR failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }
#endif

#ifdef HAVE_SO_BROADCAST
  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET,
                   SO_BROADCAST, &optval, sizeof(int));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SO_BROADCAST failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }
#endif

  return sockfd;
}

static inline int discover_openlistener(void)
{
  struct sockaddr_in addr;
  struct ifreq req;
  int sockfd;
  int ret;

  /* Create a socket to listen for requests from DHCP clients */

  sockfd = discover_socket();
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Get the IP address of the selected device */

  strncpy(req.ifr_name, CONFIG_DISCOVER_INTERFACE, IFNAMSIZ);
  ret = ioctl(sockfd, SIOCGIFADDR, (unsigned long)&req);
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SIOCGIFADDR failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }

  g_state.serverip = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
  ninfo("serverip: %08" PRIx32 "\n", ntohl(g_state.serverip));

  /* Bind the socket to a local port. We have to bind to INADDRY_ANY to
   * receive broadcast messages.
   */

  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(CONFIG_DISCOVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      nerr("ERROR: bind failed, port=%d addr=%08lx: %d\n",
           addr.sin_port, (long)addr.sin_addr.s_addr, errno);
      close(sockfd);
      return ERROR;
    }

  return sockfd;
}

static inline int discover_openresponder(void)
{
  struct sockaddr_in addr;
  int sockfd;
  int ret;

  /* Create a socket for responding to discovery message */

  sockfd = discover_socket();
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Bind the socket to a local port. */

  addr.sin_family      = AF_INET;
  addr.sin_port        = 0;
  addr.sin_addr.s_addr = g_state.serverip;

  ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      nerr("ERROR: bind failed, port=%d addr=%08lx: %d\n",
           addr.sin_port, (long)addr.sin_addr.s_addr, errno);
      close(sockfd);
      return ERROR;
    }

  return sockfd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: discover_start
 *
 * Description:
 *   Start the discover daemon.
 *
 * Return:
 *   The process ID (pid) of the new discover daemon is returned on
 *   success; A negated errno is returned if the daemon was not successfully
 *   started.
 *
 ****************************************************************************/

int discover_start(struct discover_info_s *info)
{
  pid_t pid;

  if (info)
    {
      g_state.info = *info;
    }

  /* Then start the new daemon */

  pid = task_create("Discover daemon", CONFIG_DISCOVER_PRIORITY,
                    CONFIG_DISCOVER_STACK_SIZE, discover_daemon, NULL);
  if (pid < 0)
    {
      int errval = errno;
      nerr("ERROR: Failed to start the discover daemon: %d\n", errval);
      return -errval;
    }

  /* Return success */

  return pid;
}
