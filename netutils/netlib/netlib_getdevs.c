/****************************************************************************
 * apps/netutils/netlib/netlib_getdevs.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <netpacket/netlink.h>

#include <nuttx/net/arp.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NETLINK_ROUTE

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
  struct ifinfomsg iface;
  struct rtattr    attr;
  uint8_t          data[IFNAMSIZ];  /* IFLA_IFNAME is the only attribute
                                     * supported */
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_get_devices
 *
 * Description:
 *   Return a list of all active network devices (devices in the DOWN state
 *   are not reported).
 *
 * Parameters:
 *   devlist  - The location to store the list of devices.
 *   nentries - The size of the provided 'devlist' in number of entries each
 *              of size sizeof(struct netlib_device_s)
 *   family   - Address family.  See AF_* definitions in sys/socket.h.  Use
 *              AF_PACKET to return devices for all address families.
 *
 * Return:
 *   The number of device entries read is returned on success; a negated
 *   errno value is returned on failure.
 *
 ****************************************************************************/

ssize_t netlib_get_devices(FAR struct netlib_device_s *devlist,
                           unsigned int nentries, sa_family_t family)
{
  struct netlib_sendto_request_s req;
  struct netlib_recvfrom_response_s resp;
  struct sockaddr_nl addr;
  static unsigned int seqno = 0;
  unsigned int thiseq;
  ssize_t nsent;
  ssize_t nrecvd;
  size_t ncopied = 0;
  pid_t pid;
  bool enddump;
  int fd;
  int ret;

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
  req.hdr.nlmsg_type   = RTM_GETLINK;
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

  for (enddump = false; !enddump; )
    {
      /* Receive the next device response */

      nrecvd = recv(fd, &resp, sizeof(struct netlib_recvfrom_response_s), 0);
      if (nrecvd < 0)
        {
          int errcode = errno;
          fprintf(stderr, "ERROR: recv() failed: %d\n", errcode);
          ret = -errcode;
          goto errout_with_socket;
        }

      /* Verify the data and transfer the device list to the caller */

      if (resp.hdr.nlmsg_len < sizeof(struct nlmsghdr) ||
          resp.hdr.nlmsg_len != nrecvd)
        {
          fprintf(stderr, "ERROR: Bad message\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* The sequence number in the response should match the sequence
       * number in the request (since we created the socket, this should
       * always be true).
       */

      if (resp.hdr.nlmsg_seq != thiseq)
        {
          fprintf(stderr, "ERROR: Bad sequence number in response\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* Copy the device list to the caller's buffer */

      switch (resp.hdr.nlmsg_type)
        {
          /* NLMSG_DONE means that the entire list of devices has been
           * returned
           */

          case NLMSG_DONE:
            enddump = true;
            break;

          /* RTM_NEWLINK provides information about one device */

          case RTM_NEWLINK:
            {
              FAR struct rtattr *attr;
              unsigned int attrlen;

              /* Verify the expected message length */

              if (nrecvd != sizeof(struct netlib_recvfrom_response_s))
                {
                  fprintf(stderr, "ERROR: Bad massage size: %ld\n",
                          (long)nrecvd);
                  goto errout_with_socket;
                }

              /* Decode the response */

              attr    = &resp.attr;
              attrlen = resp.hdr.nlmsg_len - sizeof(struct nlmsghdr) -
                        sizeof(struct ifinfomsg);

              for (; RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen))
                {
                  switch (attr->rta_type)
                    {
                      case IFLA_IFNAME:
                        if (ncopied < nentries)
                          {
#ifdef CONFIG_NETDEV_IFINDEX
                            FAR struct ifinfomsg *iface = &resp.iface;

                            devlist[ncopied].ifindex = iface->ifi_index;
#endif
                            strlcpy(devlist[ncopied].ifname,
                                    (FAR char *)RTA_DATA(attr), IFNAMSIZ);
                            ncopied++;
                          }

                        break;

                      default:
                        break;
                    }
               }
            }
            break;

          default:
            fprintf(stderr, "ERROR: Message type %u, length %lu\n",
                    resp.hdr.nlmsg_type, (unsigned long)resp.hdr.nlmsg_len);
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

#endif /* CONFIG_NETLINK_ROUTE */
