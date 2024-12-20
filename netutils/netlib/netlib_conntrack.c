/****************************************************************************
 * apps/netutils/netlib/netlib_conntrack.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <netpacket/netlink.h>

#include <nuttx/net/ip.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RXBUFFER_SIZE 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct netlib_conntrack_sendto_request_s
{
  struct nlmsghdr hdr;
  struct nfgenmsg msg;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ct_parse_ip
 ****************************************************************************/

static void netlib_ct_parse_ip(FAR const struct nfattr *attr,
                               FAR struct netlib_conntrack_tuple_s *tuple)
{
  FAR const struct nfattr *subattr = NFA_DATA(attr);
  ssize_t paylen = NFA_PAYLOAD(attr);

  for (; NFA_OK(subattr, paylen); subattr = NFA_NEXT(subattr, paylen))
    {
      switch (NFA_TYPE(subattr))
        {
          case CTA_IP_V4_SRC:
            net_ipv4addr_hdrcopy(&tuple->src.ipv4, NFA_DATA(subattr));
            break;
          case CTA_IP_V4_DST:
            net_ipv4addr_hdrcopy(&tuple->dst.ipv4, NFA_DATA(subattr));
            break;
          case CTA_IP_V6_SRC:
            net_ipv6addr_hdrcopy(&tuple->src.ipv6, NFA_DATA(subattr));
            break;
          case CTA_IP_V6_DST:
            net_ipv6addr_hdrcopy(&tuple->dst.ipv6, NFA_DATA(subattr));
            break;
        }
    }
}

/****************************************************************************
 * Name: netlib_ct_parse_proto
 ****************************************************************************/

static void netlib_ct_parse_proto(FAR const struct nfattr *attr,
                                  FAR struct netlib_conntrack_tuple_s *tuple)
{
  FAR const struct nfattr *subattr = NFA_DATA(attr);
  ssize_t paylen = NFA_PAYLOAD(attr);

  for (; NFA_OK(subattr, paylen); subattr = NFA_NEXT(subattr, paylen))
    {
      switch (NFA_TYPE(subattr))
        {
          case CTA_PROTO_NUM:
            tuple->l4proto = *(FAR uint8_t *)NFA_DATA(subattr);
            break;

          case CTA_PROTO_SRC_PORT:
            tuple->l4.tcp.sport = ntohs(*(FAR uint16_t *)NFA_DATA(subattr));
            break;

          case CTA_PROTO_DST_PORT:
            tuple->l4.tcp.dport = ntohs(*(FAR uint16_t *)NFA_DATA(subattr));
            break;

          case CTA_PROTO_ICMP_ID:
          case CTA_PROTO_ICMPV6_ID:
            tuple->l4.icmp.id = ntohs(*(FAR uint16_t *)NFA_DATA(subattr));
            break;

          case CTA_PROTO_ICMP_TYPE:
          case CTA_PROTO_ICMPV6_TYPE:
            tuple->l4.icmp.type = *(FAR uint8_t *)NFA_DATA(subattr);
            break;

          case CTA_PROTO_ICMP_CODE:
          case CTA_PROTO_ICMPV6_CODE:
            tuple->l4.icmp.code = *(FAR uint8_t *)NFA_DATA(subattr);
            break;
        }
    }
}

/****************************************************************************
 * Name: netlib_ct_parse_tuple
 ****************************************************************************/

static void netlib_ct_parse_tuple(FAR const struct nfattr *attr,
                                  FAR struct netlib_conntrack_tuple_s *tuple)
{
  FAR const struct nfattr *subattr = NFA_DATA(attr);
  ssize_t paylen = NFA_PAYLOAD(attr);

  for (; NFA_OK(subattr, paylen); subattr = NFA_NEXT(subattr, paylen))
    {
      switch (NFA_TYPE(subattr))
        {
          case CTA_TUPLE_IP:
            netlib_ct_parse_ip(subattr, tuple);
            break;

          case CTA_TUPLE_PROTO:
            netlib_ct_parse_proto(subattr, tuple);
            break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_parse_conntrack
 *
 * Description:
 *   Parse a conntrack message.
 *
 * Input Parameters:
 *   nlh - The netlink message to parse.
 *   ct  - The conntrack to fill.
 *
 * Returned Value:
 *   OK on success, -errno on failure.
 *
 ****************************************************************************/

int netlib_parse_conntrack(FAR const struct nlmsghdr *nlh, size_t len,
                           FAR struct netlib_conntrack_s *ct)
{
  FAR const struct nfgenmsg *nfmsg;
  FAR const struct nfattr   *attr;
  ssize_t                    paylen;

  if (len < sizeof(struct nlmsghdr) || len < nlh->nlmsg_len)
    {
      fprintf(stderr, "Error message length got %zd vs %" PRIu32 "\n",
              len, nlh->nlmsg_len);
      return -EINVAL;
    }

  if (NFNL_SUBSYS_ID(nlh->nlmsg_type) != NFNL_SUBSYS_CTNETLINK)
    {
      fprintf(stderr, "Unknown message type %" PRIx32 "\n", nlh->nlmsg_type);
      return -ENOTSUP;
    }

  nfmsg  = NLMSG_DATA(nlh);
  attr   = NFM_NFA(nfmsg);
  paylen = NFM_PAYLOAD(nlh);

  ct->family = nfmsg->nfgen_family;
  ct->type   = NFNL_MSG_TYPE(nlh->nlmsg_type);

  for (; NFA_OK(attr, paylen); attr = NFA_NEXT(attr, paylen))
    {
      switch (NFA_TYPE(attr))
        {
          case CTA_TUPLE_ORIG:
            netlib_ct_parse_tuple(attr, &ct->orig);
            break;
          case CTA_TUPLE_REPLY:
            netlib_ct_parse_tuple(attr, &ct->reply);
            break;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: netlib_get_conntrack
 *
 * Description:
 *   Get the conntrack table.
 *
 * Input Parameters:
 *   family - The address family to get the conntrack table for.
 *   cb     - The callback to call for each conntrack entry.
 *
 * Returned Value:
 *   The number of conntrack entries processed on success, -errno on failure.
 *
 ****************************************************************************/

int netlib_get_conntrack(sa_family_t family, netlib_conntrack_cb_t cb)
{
  FAR const struct nlmsghdr *nlh;
  struct netlib_conntrack_sendto_request_s req;
  struct netlib_conntrack_s ct;
  struct sockaddr_nl addr;
  static unsigned int seqno = 0;
  unsigned int thiseq;
  uint8_t buf[RXBUFFER_SIZE];
  ssize_t nrecvd;
  size_t cnt;
  int fd;
  int ret;

  /* Create a NetLink socket with NETLINK_NETFILTER protocol */

  fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_NETFILTER);
  if (fd < 0)
    {
      perror("ERROR: failed to create netlink socket");
      return -errno;
    }

  addr.nl_family = AF_NETLINK;
  addr.nl_pad    = 0;
  addr.nl_pid    = getpid();
  addr.nl_groups = 0;

  if (bind(fd, (FAR const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("ERROR: failed to bind netlink socket");
      ret = -errno;
      goto errout_with_socket;
    }

  /* Initialize the request */

  thiseq = ++seqno;

  memset(&req, 0, sizeof(req));
  req.hdr.nlmsg_len    = NLMSG_LENGTH(sizeof(struct nfgenmsg));
  req.hdr.nlmsg_flags  = NLM_F_REQUEST | NLM_F_DUMP;
  req.hdr.nlmsg_seq    = thiseq;
  req.hdr.nlmsg_type   = NFNL_SUBSYS_CTNETLINK << 8 | IPCTNL_MSG_CT_GET;
  req.hdr.nlmsg_pid    = addr.nl_pid;
  req.msg.nfgen_family = family;
  req.msg.version      = NFNETLINK_V0;
  req.msg.res_id       = 0;

  ret = send(fd, &req, req.hdr.nlmsg_len, 0);
  if (ret < 0)
    {
      perror("ERROR: send() failed");
      ret = -errno;
      goto errout_with_socket;
    }

  /* Read the response */

  for (cnt = 0; ; cnt++)
    {
      nrecvd = recv(fd, buf, sizeof(buf), 0);
      if (nrecvd < 0)
        {
          perror("ERROR: recv() failed");
          ret = -errno;
          goto errout_with_socket;
        }

      nlh = (FAR struct nlmsghdr *)buf;

      /* Verify the data and transfer the connection info to the caller */

      if (nrecvd < sizeof(struct nlmsghdr) ||
          nlh->nlmsg_len < sizeof(struct nlmsghdr))
        {
          fprintf(stderr, "ERROR: Bad message\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* The sequence number in the response should match the sequence
       * number in the request (since we created the socket, this should
       * always be true).
       */

      if (nlh->nlmsg_seq != thiseq)
        {
          fprintf(stderr, "ERROR: Bad sequence number in response\n");
          ret = -EIO;
          goto errout_with_socket;
        }

      /* Callback the connection info to the caller */

      if (nlh->nlmsg_type == NLMSG_DONE)
        {
          ret = cnt;
          break;
        }

      ret = netlib_parse_conntrack(nlh, nrecvd, &ct);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: failed to parse conntrack message\n");
          goto errout_with_socket;
        }

      ret = cb(&ct);
      if (ret < 0)
        {
          fprintf(stderr, "ERROR: callback failed\n");
          goto errout_with_socket;
        }
    }

errout_with_socket:
  close(fd);
  return ret;
}
