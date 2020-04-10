/****************************************************************************
 * netutils/netlib/netlib_getroute.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <nuttx/config.h>

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <net/route.h>
#include <netpacket/netlink.h>

#include "netutils/netlib.h"

#if defined(CONFIG_NETLINK_ROUTE) && defined(CONFIG_NET_ROUTE)

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

#define RXBUFFER_SIZE 64

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct netlib_sendto_request_s
{
  struct nlmsghdr  hdr;
  struct rtgenmsg  gen;
};

struct netlib_recvfrom_response_s
{
  struct nlmsghdr  hdr;
  struct rtmsg     rte;
  struct rtattr    attr;

  /* Attribute data and additional attributes follow */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int copy_address(FAR struct sockaddr_storage *dest, FAR void *src,
                        unsigned int addrlen, unsigned int maxaddr,
                        sa_family_t family)
{
  DEBUGASSERT(addrlen <= maxaddr);
  if (addrlen > maxaddr)
    {
      fprintf(stderr, "ERROR: Bad address length: %u\n", addrlen);
      return -EIO;
    }

  dest->ss_family = family;
  memcpy(dest->ss_data, src, addrlen);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_get_route
 *
 * Description:
 *   Return a snapshot of the routing table for the selected address family.
 *
 * Parameters:
 *   rtelist  - The location to store the list of devices.
 *   nentries - The size of the provided 'rtelist' in number of entries.
 *              The size of one entry is given by sizeof(struct rtentry);
 *   family   - Address family.  See AF_* definitions in sys/socket.h.
 *
 * Return:
 *   The number of routing table entries read is returned on success; a
 *   negated errno value is returned on failure.
 *
 ****************************************************************************/

ssize_t netlib_get_route(FAR struct rtentry *rtelist,
                         unsigned int nentries, sa_family_t family)
{
  struct netlib_sendto_request_s req;
  struct sockaddr_nl addr;
  static unsigned int seqno = 0;
  unsigned int thiseq;
  unsigned int maxaddr;
  ssize_t nsent;
  ssize_t nrecvd;
  size_t ncopied = 0;
  pid_t pid;
  bool enddump;
  int fd;
  int ret;

  union
  {
    char rxbuffer[RXBUFFER_SIZE];
    struct netlib_recvfrom_response_s resp;
  } u;

  /* Create a NetLink socket with NETLINK_ROUTE protocol */

  fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: socket() failed: %d\n", errcode);
      return -errcode;
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
  req.hdr.nlmsg_type   = RTM_GETROUTE;
  req.hdr.nlmsg_pid    = pid;
  req.gen.rtgen_family = family;

  nsent = send(fd, &req, req.hdr.nlmsg_len, 0);
  if (nsent < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: send() failed: %d\n", errcode);
      ret = -errcode;
      goto errout_with_socket;
    }

  /* Read the response(s) */

  maxaddr = sizeof(struct sockaddr_storage) - sizeof(sa_family_t);

  for (enddump = false; !enddump; )
    {
      /* Receive the next device response */

      nrecvd = recv(fd, &u.rxbuffer, RXBUFFER_SIZE, 0);
      if (nrecvd < 0)
        {
          int errcode = errno;
          fprintf(stderr, "ERROR: recv() failed: %d\n", errcode);
          ret = -errcode;
          goto errout_with_socket;
        }

      /* Verify the data and transfer the device list to the caller */

      if (u.resp.hdr.nlmsg_len < sizeof(struct nlmsghdr) ||
          u.resp.hdr.nlmsg_len != nrecvd)
        {
          fprintf(stderr, "ERROR: Bad message\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* The sequence number in the response should match the sequence
       * number in the request (since we created the socket, this should
       * always be true).
       */

      if (u.resp.hdr.nlmsg_seq != thiseq)
        {
          fprintf(stderr, "ERROR: Bad sequence number in response\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* This should be a routing table response */

      if (u.resp.rte.rtm_table != RT_TABLE_MAIN)
        {
          fprintf(stderr, "ERROR: Not a routing table response\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* Copy the routing table entry to the caller's buffer */

      switch (u.resp.hdr.nlmsg_type)
        {
          case NLMSG_DONE:
            enddump = true;
            break;

          case RTM_NEWROUTE:
            {
              FAR struct rtentry *dest;
              FAR struct rtattr *attr;
              unsigned int paylen;

              /* Verify the expected message length */

              if (nrecvd <= sizeof(struct netlib_recvfrom_response_s) ||
                  u.resp.rte.rtm_family != family)
                {
                  fprintf(stderr, "ERROR: Bad massage size: %ld\n",
                          (long)nrecvd);
                  goto errout_with_socket;
                }

              /* Decode the response */

              dest    = &rtelist[ncopied];
              memset(dest, 0, sizeof(struct rtentry));

              attr   = &u.resp.attr;
              paylen = u.resp.hdr.nlmsg_len - sizeof(struct nlmsghdr) -
                        sizeof(struct rtmsg);

              for (; RTA_OK(attr, paylen); attr = RTA_NEXT(attr, paylen))
                {
                  unsigned int attrlen = RTA_PAYLOAD(attr);

                  switch (attr->rta_type)
                    {
                      case RTA_DST:      /* The destination network */
                        ret = copy_address(&dest->rt_dst, RTA_DATA(attr),
                                           attrlen, maxaddr, family);
                        if (ret < 0)
                          {
                            goto errout_with_socket;
                          }

                        break;

                      case RTA_SRC:      /* The source address mask */
                        break;

                      case RTA_GENMASK:  /* The network address mask */
                        ret = copy_address(&dest->rt_genmask, RTA_DATA(attr),
                                           attrlen, maxaddr, family);
                        if (ret < 0)
                          {
                            goto errout_with_socket;
                          }

                        break;

                      case RTA_GATEWAY:  /* Route packets via this router */
                        ret = copy_address(&dest->rt_gateway, RTA_DATA(attr),
                                           attrlen, maxaddr, family);
                        if (ret < 0)
                          {
                            goto errout_with_socket;
                          }

                        break;

                      default:
                        break;
                    }
                }

              ncopied++;
            }
            break;

          default:
            fprintf(stderr, "ERROR: Message type %u, length %u\n",
                    u.resp.hdr.nlmsg_type, (unsigned)u.resp.hdr.nlmsg_len);
            ret = -EIO;
            goto errout_with_socket;
        }
    }

  close(fd);
  return ncopied;

errout_with_socket:
  close(fd);
  return ret;
}

#endif /* CONFIG_NETLINK_ROUTE && CONFIG_NET_ROUTE */
