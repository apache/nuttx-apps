/****************************************************************************
 * apps/netutils/netlib/netlib_getarptab.c
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

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netpacket/netlink.h>

#include <nuttx/net/arp.h>

#include "netutils/netlib.h"

#if defined(CONFIG_NET_ARP) && defined(CONFIG_NETLINK_ROUTE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct netlib_sendto_request_s
{
  struct nlmsghdr hdr;
  struct rtgenmsg gen;
};

struct netlib_recvfrom_response_s
{
  struct nlmsghdr hdr;
  struct ndmsg msg;
  struct rtattr attr;
  uint8_t data[1];
};

#define SIZEOF_NETLIB_RECVFROM_RESPONSE_S(n) \
  (sizeof(struct netlib_recvfrom_response_s) + (n) - 1)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_get_arptable
 *
 * Description:
 *   Attempt to read the entire ARP table into a buffer.
 *
 * Parameters:
 *   arptab   - The location to store the copy of the ARP table
 *   nentries - The size of the provided 'arptab' in number of entries each
 *              of size sizeof(struct arp_entry_s)
 *
 * Return:
 *   The number of ARP table entries read is returned on success; a negated
 *   errno value is returned on failure.
 *
 ****************************************************************************/

ssize_t netlib_get_arptable(FAR struct arp_entry_s *arptab,
                            unsigned int nentries)
{
  FAR struct netlib_recvfrom_response_s *resp;
  struct netlib_sendto_request_s req;
  struct sockaddr_nl addr;
  static unsigned int seqno = 0;
  unsigned int thiseq;
  unsigned int allocsize;
  ssize_t nsent;
  ssize_t nrecvd;
  ssize_t paysize;
  ssize_t maxsize;
  pid_t pid;
  int fd;
  int ret;

  /* Pre-allocate a buffer to hold the response */

  maxsize   = CONFIG_NET_ARPTAB_SIZE * sizeof(struct arp_entry_s);
  allocsize = SIZEOF_NETLIB_RECVFROM_RESPONSE_S(maxsize);
  resp = (FAR struct netlib_recvfrom_response_s *)malloc(allocsize);
  if (resp == NULL)
    {
      fprintf(stderr, "ERROR: Failed to allocate response buffer\n");
      ret = -ENOMEM;
      return EXIT_FAILURE;
    }

  /* Create a NetLink socket with NETLINK_ROUTE protocol */

  fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: socket() failed: %d\n", errcode);
      ret = -errcode;
      goto errout_with_resp;
    }

  /* Bind the socket so that we can use send() and receive() */

  pid            = getpid();
  addr.nl_family = AF_NETLINK;
  addr.nl_pad    = 0;
  addr.nl_pid    = pid;
  addr.nl_groups = 0;

  ret = bind(fd, (FAR const struct sockaddr *)&addr,
             sizeof(struct sockaddr_nl));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: bind() failed: %d\n", errcode);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Initialize the request */

  thiseq = ++seqno;

  memset(&req, 0, sizeof(req));
  req.hdr.nlmsg_len    = NLMSG_LENGTH(sizeof(struct rtgenmsg));
  req.hdr.nlmsg_flags  = NLM_F_REQUEST | NLM_F_DUMP;
  req.hdr.nlmsg_seq    = thiseq;
  req.hdr.nlmsg_type   = RTM_GETNEIGH;
  req.hdr.nlmsg_pid    = pid;
  req.gen.rtgen_family = AF_INET;

  nsent = send(fd, &req, req.hdr.nlmsg_len, 0);
  if (nsent < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: send() failed: %d\n", errcode);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Read the response */

  nrecvd = recv(fd, resp, allocsize, 0);
  if (nrecvd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: recv() failed: %d\n", errcode);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Verify the data and transfer the ARP table data to the caller */

  if (resp->hdr.nlmsg_len < sizeof(struct nlmsghdr) ||
      resp->hdr.nlmsg_len > nrecvd)
    {
      fprintf(stderr, "ERROR: Bad message\n");
      ret = -EIO;
      goto errout_with_socket;
    }

  /* The sequence number in the response should match the sequence
   * number in the request (since we created the socket, this should
   * always be true).
   */

  if (resp->hdr.nlmsg_seq != thiseq)
    {
      fprintf(stderr, "ERROR: Bad sequence number in response\n");
      ret = -EIO;
      goto errout_with_socket;
    }

  /* Copy the ARP table data to the caller's buffer */

  paysize = RTA_PAYLOAD(&resp->attr);
  if (paysize > maxsize)
    {
      paysize = maxsize;
    }

  memcpy(arptab, resp->data, paysize);
  ret = paysize / sizeof(struct arp_entry_s);

errout_with_socket:
  close(fd);

errout_with_resp:
  free(resp);
  return ret;
}

#endif /* CONFIG_NET_ARP && CONFIG_NETLINK_ROUTE */
