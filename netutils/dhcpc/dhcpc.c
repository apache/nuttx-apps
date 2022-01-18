/****************************************************************************
 * apps/netutils/dhcpc/dhcpc.c
 *
 *   Copyright (C) 2007, 2009, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Based heavily on portions of uIP:
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2005, Swedish Institute of Computer Science
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/udp.h>

#include "netutils/dhcpc.h"
#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration */

/* DHCP Definitions */

#define STATE_INITIAL           0
#define STATE_HAVE_OFFER        1
#define STATE_HAVE_LEASE        2

#define DHCP_REQUEST            1
#define DHCP_REPLY              2
#define DHCP_HTYPE_ETHERNET     1
#define DHCP_HLEN_ETHERNET      6
#define DHCP_MSG_LEN            236

#define DHCPC_SERVER_PORT       67
#define DHCPC_CLIENT_PORT       68

#define DHCPDISCOVER            1
#define DHCPOFFER               2
#define DHCPREQUEST             3
#define DHCPDECLINE             4
#define DHCPACK                 5
#define DHCPNAK                 6
#define DHCPRELEASE             7

#define DHCP_OPTION_SUBNET_MASK 1
#define DHCP_OPTION_ROUTER      3
#define DHCP_OPTION_DNS_SERVER  6
#define DHCP_OPTION_HOST_NAME   12
#define DHCP_OPTION_REQ_IPADDR  50
#define DHCP_OPTION_LEASE_TIME  51
#define DHCP_OPTION_MSG_TYPE    53
#define DHCP_OPTION_SERVER_ID   54
#define DHCP_OPTION_REQ_LIST    55
#define DHCP_OPTION_END         255

#define BUFFER_SIZE             256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct dhcp_msg
{
  uint8_t  op;
  uint8_t  htype;
  uint8_t  hlen;
  uint8_t  hops;
  uint8_t  xid[4];
  uint16_t secs;
  uint16_t flags;
  uint8_t  ciaddr[4];
  uint8_t  yiaddr[4];
  uint8_t  siaddr[4];
  uint8_t  giaddr[4];
  uint8_t  chaddr[16];
#ifndef CONFIG_NET_DHCP_LIGHT
  uint8_t  sname[64];
  uint8_t  file[128];
#endif
  uint8_t  options[312];
};

struct dhcpc_state_s
{
  FAR const char    *interface;
  int                sockfd;
  struct in_addr     ipaddr;
  struct in_addr     serverid;
  struct dhcp_msg    packet;
  bool               cancel;
  pthread_t          thread;              /* Thread ID of the DHCPC thread */
  dhcpc_callback_t   callback;            /* Thread callback of the DHCPC thread */
  int                maclen;
  uint8_t            macaddr[1];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t xid[4] =
{
  0xad, 0xde, 0x12, 0x23
};

static const uint8_t magic_cookie[4] =
{
  99, 130, 83, 99
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpc_add<option>
 ****************************************************************************/

static FAR uint8_t *dhcpc_addhostname(FAR const char *hostname,
                                      FAR uint8_t *optptr)
{
  int len = strlen(hostname);
  *optptr++ = DHCP_OPTION_HOST_NAME;
  *optptr++ = len;
  memcpy(optptr, hostname, len);
  return optptr + len;
}

static FAR uint8_t *dhcpc_addmsgtype(FAR uint8_t *optptr, uint8_t type)
{
  *optptr++ = DHCP_OPTION_MSG_TYPE;
  *optptr++ = 1;
  *optptr++ = type;
  return optptr;
}

static FAR uint8_t *dhcpc_addserverid(FAR struct in_addr *serverid,
                                      FAR uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_SERVER_ID;
  *optptr++ = 4;
  memcpy(optptr, &serverid->s_addr, 4);
  return optptr + 4;
}

static FAR uint8_t *dhcpc_addreqipaddr(FAR struct in_addr *ipaddr,
                                       FAR uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_IPADDR;
  *optptr++ = 4;
  memcpy(optptr, &ipaddr->s_addr, 4);
  return optptr + 4;
}

static FAR uint8_t *dhcpc_addreqoptions(FAR uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_REQ_LIST;
  *optptr++ = 3;
  *optptr++ = DHCP_OPTION_SUBNET_MASK;
  *optptr++ = DHCP_OPTION_ROUTER;
  *optptr++ = DHCP_OPTION_DNS_SERVER;
  return optptr;
}

static FAR uint8_t *dhcpc_addend(FAR uint8_t *optptr)
{
  *optptr++ = DHCP_OPTION_END;
  return optptr;
}

/****************************************************************************
 * Name: dhcpc_sendmsg
 ****************************************************************************/

static int dhcpc_sendmsg(FAR struct dhcpc_state_s *pdhcpc,
                         FAR struct dhcpc_state *presult, int msgtype)
{
  char hostname[HOST_NAME_MAX];
  struct sockaddr_in addr;
  FAR uint8_t *pend;
  in_addr_t serverid = INADDR_BROADCAST;
  int len;

  /* Create the common message header settings */

  memset(&pdhcpc->packet, 0, sizeof(struct dhcp_msg));
  pdhcpc->packet.op    = DHCP_REQUEST;
  pdhcpc->packet.htype = DHCP_HTYPE_ETHERNET;
  pdhcpc->packet.hlen  = pdhcpc->maclen;
  memcpy(pdhcpc->packet.xid, xid, 4);
  memcpy(pdhcpc->packet.chaddr, pdhcpc->macaddr, pdhcpc->maclen);
  memset(&pdhcpc->packet.chaddr[pdhcpc->maclen], 0, 16 - pdhcpc->maclen);
  memcpy(pdhcpc->packet.options, magic_cookie, sizeof(magic_cookie));

  /* Add the common header options */

  pend = &pdhcpc->packet.options[4];
  pend = dhcpc_addmsgtype(pend, msgtype);

  /* Get the current host name */

  if (gethostname(hostname, sizeof(hostname)) || (0 == strlen(hostname)))
    {
      strncpy(hostname, CONFIG_NETUTILS_DHCPC_HOST_NAME, HOST_NAME_MAX);
    }

  /* Handle the message specific settings */

  switch (msgtype)
    {
      /* Broadcast DISCOVER message to all servers */

      case DHCPDISCOVER:
        /* REVISIT: We don't need the broadcast flag since we can receive
         * unicast traffic before being fully configured.
         */

        /* Broadcast bit. */

        pdhcpc->packet.flags = HTONS(CONFIG_NETUTILS_DHCPC_BOOTP_FLAGS);

        pend     = dhcpc_addhostname(hostname, pend);
        pend     = dhcpc_addreqoptions(pend);
        break;

      /* Send REQUEST message to the server that sent the *first* OFFER */

      case DHCPREQUEST:
        /* REVISIT: We don't need the broadcast flag since we can receive
         * unicast traffic before being fully configured.
         */

        /* Broadcast bit. */

        pdhcpc->packet.flags = HTONS(CONFIG_NETUTILS_DHCPC_BOOTP_FLAGS);

        pend     = dhcpc_addhostname(hostname, pend);
        pend     = dhcpc_addserverid(&pdhcpc->serverid, pend);
        pend     = dhcpc_addreqipaddr(&pdhcpc->ipaddr, pend);
        break;

      /* Send DECLINE message to the server that sent the *last* OFFER */

      case DHCPDECLINE:
        memcpy(pdhcpc->packet.ciaddr, &presult->ipaddr.s_addr, 4);
        pend     = dhcpc_addserverid(&presult->serverid, pend);
        serverid = presult->serverid.s_addr;
        break;

      default:
        return ERROR;
    }

  pend = dhcpc_addend(pend);
  len  = pend - (uint8_t *)&pdhcpc->packet;

  /* Send the request */

  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(DHCPC_SERVER_PORT);
  addr.sin_addr.s_addr = serverid;

  return sendto(pdhcpc->sockfd, &pdhcpc->packet, len, 0,
                (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
}

/****************************************************************************
 * Name: dhcpc_parseoptions
 ****************************************************************************/

static uint8_t dhcpc_parseoptions(FAR struct dhcpc_state *presult,
                                  FAR uint8_t *optptr, int len)
{
  FAR uint8_t *end = optptr + len;
  uint8_t type = 0;

  while (optptr < end)
    {
      switch (*optptr)
        {
          case DHCP_OPTION_SUBNET_MASK:

            /* Get subnet mask in network order */

            if (optptr + 6 <= end)
              {
                memcpy(&presult->netmask.s_addr, optptr + 2, 4);
              }
            else
              {
                nerr("Packet too short (netmask missing)\n");
              }
            break;

          case DHCP_OPTION_ROUTER:

            /* Get the default router address in network order */

            if (optptr + 6 <= end)
              {
                memcpy(&presult->default_router.s_addr, optptr + 2, 4);
              }
            else
              {
                nerr("Packet too short (router address missing)\n");
              }
            break;

          case DHCP_OPTION_DNS_SERVER:

            /* Get the DNS server address in network order */

            if (optptr + 6 <= end)
              {
                memcpy(&presult->dnsaddr.s_addr, optptr + 2, 4);
              }
            else
              {
                nerr("Packet too short (DNS address missing)\n");
              }
            break;

          case DHCP_OPTION_MSG_TYPE:

            /* Get message type */

            if (optptr + 3 <= end)
              {
                type = *(optptr + 2);
              }
            else
              {
                nerr("Packet too short (type missing)\n");
              }
            break;

          case DHCP_OPTION_SERVER_ID:

            /* Get server address in network order */

            if (optptr + 6 <= end)
              {
                memcpy(&presult->serverid.s_addr, optptr + 2, 4);
              }
            else
              {
                nerr("Packet too short (server address missing)\n");
              }
            break;

          case DHCP_OPTION_LEASE_TIME:

              /* Get lease time (in seconds) in host order */

            if (optptr + 6 <= end)
              {
                uint16_t tmp[2];
                memcpy(tmp, optptr + 2, 4);
                presult->lease_time = ((uint32_t)ntohs(tmp[0])) << 16 |
                                       (uint32_t)ntohs(tmp[1]);
              }
            else
              {
                nerr("Packet too short (lease time missing)\n");
              }
            break;

          case DHCP_OPTION_END:
            return type;
        }

      if (optptr + 1 >= end)
        {
          break;
        }

      optptr += optptr[1] + 2;
    }

  return type;
}

/****************************************************************************
 * Name: dhcpc_parsemsg
 ****************************************************************************/

static uint8_t dhcpc_parsemsg(FAR struct dhcpc_state_s *pdhcpc, int buflen,
                              FAR struct dhcpc_state *presult)
{
  if (buflen >= 44 && pdhcpc->packet.op == DHCP_REPLY &&
      memcmp(pdhcpc->packet.xid, xid, sizeof(xid)) == 0 &&
      memcmp(pdhcpc->packet.chaddr,
             pdhcpc->macaddr, pdhcpc->maclen) == 0)
    {
      memcpy(&presult->ipaddr.s_addr, pdhcpc->packet.yiaddr, 4);
      return dhcpc_parseoptions(presult, &pdhcpc->packet.options[4],
                                buflen -
                                (offsetof(struct dhcp_msg, options) + 4));
    }

  return 0;
}

/****************************************************************************
 * Name: dhcpc_run
 ****************************************************************************/

static void *dhcpc_run(void *args)
{
  FAR struct dhcpc_state_s *pdhcpc = (FAR struct dhcpc_state_s *)args;
  struct dhcpc_state result;
  int ret;

  while (1)
    {
      ret = dhcpc_request(pdhcpc, &result);
      if (ret == OK)
        {
          pdhcpc->callback(&result);
        }
      else
        {
          pdhcpc->callback(NULL);
          memset(&result, 0, sizeof(result));
          nerr("dhcpc_request error\n");
        }

      if (pdhcpc->cancel)
        {
          return NULL;
        }

      result.lease_time /= 2;
      while (result.lease_time)
        {
          result.lease_time = sleep(result.lease_time);
          if (pdhcpc->cancel)
            {
              return NULL;
            }
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpc_open
 ****************************************************************************/

FAR void *dhcpc_open(FAR const char *interface, FAR const void *macaddr,
                     int maclen)
{
  FAR struct dhcpc_state_s *pdhcpc;
  struct sockaddr_in addr;
  struct timeval tv;
  int ret;

  ninfo("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        ((uint8_t *)macaddr)[0], ((uint8_t *)macaddr)[1],
        ((uint8_t *)macaddr)[2], ((uint8_t *)macaddr)[3],
        ((uint8_t *)macaddr)[4], ((uint8_t *)macaddr)[5]);

  /* Allocate an internal DHCP structure */

  pdhcpc = malloc(sizeof(struct dhcpc_state_s) + maclen - 1);
  if (pdhcpc)
    {
      /* Initialize the allocated structure */

      memset(pdhcpc, 0, sizeof(struct dhcpc_state_s));
      pdhcpc->interface = interface;
      pdhcpc->maclen    = maclen;
      memcpy(pdhcpc->macaddr, macaddr, pdhcpc->maclen);

      /* Create a UDP socket */

      pdhcpc->sockfd = socket(PF_INET, SOCK_DGRAM, 0);
      if (pdhcpc->sockfd < 0)
        {
          ninfo("socket handle %d\n", pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }

      /* Bind the socket */

      addr.sin_family      = AF_INET;
      addr.sin_port        = HTONS(DHCPC_CLIENT_PORT);
      addr.sin_addr.s_addr = INADDR_ANY;

      ret = bind(pdhcpc->sockfd, (struct sockaddr *)&addr,
                 sizeof(struct sockaddr_in));
      if (ret < 0)
        {
          ninfo("bind status %d\n", ret);
          close(pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }

      /* Configure for read timeouts */

      tv.tv_sec  = CONFIG_NETUTILS_DHCPC_RECV_TIMEOUT;
      tv.tv_usec = 0;

      ret = setsockopt(pdhcpc->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                       sizeof(struct timeval));
      if (ret < 0)
        {
          ninfo("setsockopt(RCVTIMEO) status %d\n", ret);
          close(pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }

#ifdef CONFIG_NET_UDP_BINDTODEVICE
      /* Bind socket to interface, because UDP packets have to be sent to the
       * broadcast address at a moment when it is not possible to decide the
       * target network device using the local or remote address (which is,
       * by definition and purpose of DHCP, undefined yet).
       */

      ret = setsockopt(pdhcpc->sockfd, IPPROTO_UDP, UDP_BINDTODEVICE,
                       pdhcpc->interface, strlen(pdhcpc->interface));
      if (ret < 0)
        {
          ninfo("setsockopt(BINDTODEVICE) status %d\n", ret);
          close(pdhcpc->sockfd);
          free(pdhcpc);
          return NULL;
        }
#endif
    }

  return (FAR void *)pdhcpc;
}

/****************************************************************************
 * Name: dhcpc_close
 ****************************************************************************/

void dhcpc_close(FAR void *handle)
{
  struct dhcpc_state_s *pdhcpc = (struct dhcpc_state_s *)handle;

  if (pdhcpc)
    {
      if (pdhcpc->thread)
        {
          dhcpc_cancel(pdhcpc);
        }

      if (pdhcpc->sockfd)
        {
          close(pdhcpc->sockfd);
        }

      free(pdhcpc);
    }
}

/****************************************************************************
 * Name: dhcpc_cancel
 ****************************************************************************/

void dhcpc_cancel(FAR void *handle)
{
  struct dhcpc_state_s *pdhcpc = (struct dhcpc_state_s *)handle;
  sighandler_t old;
  int ret;

  if (pdhcpc)
    {
      pdhcpc->cancel = true;

      if (pdhcpc->thread)
        {
          old = signal(SIGQUIT, SIG_IGN);

          /* Signal the dhcpc_run */

          ret = pthread_kill(pdhcpc->thread, SIGQUIT);
          if (ret != 0)
            {
              nerr("ERROR: pthread_kill DHCPC thread\n");
            }

          /* Wait for the end of dhcpc_run */

          ret = pthread_join(pdhcpc->thread, NULL);
          if (ret != 0)
            {
              nerr("ERROR: pthread_join DHCPC thread\n");
            }

          pdhcpc->thread = 0;
          signal(SIGQUIT, old);
        }
    }
}

/****************************************************************************
 * Name: dhcpc_request
 ****************************************************************************/

int dhcpc_request(FAR void *handle, FAR struct dhcpc_state *presult)
{
  FAR struct dhcpc_state_s *pdhcpc = (FAR struct dhcpc_state_s *)handle;
  struct in_addr oldaddr;
  struct in_addr newaddr;
  ssize_t result;
  uint8_t msgtype;
  int     retries;
  int     state;

  /* Save the currently assigned IP address (should be INADDR_ANY) */

  oldaddr.s_addr = 0;
  netlib_get_ipv4addr(pdhcpc->interface, &oldaddr);

  /* Set the IP address to INADDR_ANY. */

  newaddr.s_addr = INADDR_ANY;
  netlib_set_ipv4addr(pdhcpc->interface, &newaddr);

  /* Loop sending the DISCOVER up to CONFIG_NETUTILS_DHCPC_RETRIES
   * times
   */

  retries = 0;

  /* Loop sending DISCOVER until we receive an OFFER from a DHCP
   * server.  We will lock on to the first OFFER and decline any
   * subsequent offers (which will happen if there are more than one
   * DHCP servers on the network.
   */

  state = STATE_INITIAL;
  do
    {
      if (pdhcpc->cancel)
        {
          return ERROR;
        }

      /* Send the DISCOVER command */

      ninfo("Broadcast DISCOVER\n");
      if (dhcpc_sendmsg(pdhcpc, presult, DHCPDISCOVER) < 0)
        {
          return ERROR;
        }

      retries++;

      /* Get the DHCPOFFER response */

      result = recv(pdhcpc->sockfd, &pdhcpc->packet,
                    sizeof(struct dhcp_msg), 0);
      if (result >= 0)
        {
          msgtype = dhcpc_parsemsg(pdhcpc, result, presult);
          if (msgtype == DHCPOFFER)
            {
              /* Save the servid from the presult so that it is not
               * clobbered by a new OFFER.
               */

              ninfo("Received OFFER from %08" PRIx32 "\n",
                    (uint32_t)ntohl(presult->serverid.s_addr));
              pdhcpc->ipaddr.s_addr   = presult->ipaddr.s_addr;
              pdhcpc->serverid.s_addr = presult->serverid.s_addr;

              /* Temporarily use the address offered by the server
               * and break out of the loop.
               */

              netlib_set_ipv4addr(pdhcpc->interface,
                                  &presult->ipaddr);
              state = STATE_HAVE_OFFER;
            }
        }

      /* An error has occurred.  If this was a timeout error (meaning
       * that nothing was received on this socket for a long period
       * of time). Then loop and send the DISCOVER command again.
       */

      else if (errno != EAGAIN)
        {
          /* An error other than a timeout was received -- error out */

          return ERROR;
        }
    }
  while (state == STATE_INITIAL &&
         retries < CONFIG_NETUTILS_DHCPC_RETRIES);

  /* If no DHCPOFFER recveived here, error out */

  if (state == STATE_INITIAL)
    {
      return ERROR;
    }

  /* Loop sending the REQUEST up to CONFIG_NETUTILS_DHCPC_RETRIES times
   * (if there is no response)
   */

  retries = 0;
  do
    {
      if (pdhcpc->cancel)
        {
          return ERROR;
        }

      /* Send the REQUEST message to obtain the lease that was offered to
       * us.
       */

      ninfo("Send REQUEST\n");
      if (dhcpc_sendmsg(pdhcpc, presult, DHCPREQUEST) < 0)
        {
          return ERROR;
        }

      retries++;

      /* Get the ACK/NAK response to the REQUEST (or timeout) */

      result = recv(pdhcpc->sockfd, &pdhcpc->packet,
                    sizeof(struct dhcp_msg), 0);
      if (result >= 0)
        {
          /* Parse the response */

          msgtype = dhcpc_parsemsg(pdhcpc, result, presult);

          /* The ACK response means that the server has accepted
           * our request and we have the lease.
           */

          if (msgtype == DHCPACK)
            {
              ninfo("Received ACK\n");
              state = STATE_HAVE_LEASE;
            }

          /* NAK means the server has refused our request.  Break out of
           * this loop with state == STATE_HAVE_OFFER and send DISCOVER
           * again
           */

          else if (msgtype == DHCPNAK)
            {
              ninfo("Received NAK\n");
              break;
            }

          /* If we get any OFFERs from other servers, then decline
           * them now and continue waiting for the ACK from the server
           * that we requested from.
           */

          else if (msgtype == DHCPOFFER)
            {
              ninfo("Received another OFFER, send DECLINE\n");
              dhcpc_sendmsg(pdhcpc, presult, DHCPDECLINE);
            }

          /* Otherwise, it is something that we do not recognize */

          else
            {
              ninfo("Ignoring msgtype=%d\n", msgtype);
            }
        }

      /* An error has occurred.  If this was a timeout error (meaning
       * that nothing was received on this socket for a long period of
       * time). Then break out and send the DISCOVER command again
       * (at most 3 times).
       */

      else if (errno != EAGAIN)
        {
          /* An error other than a timeout was received */

          netlib_set_ipv4addr(pdhcpc->interface, &oldaddr);
          return ERROR;
        }
    }
  while (state == STATE_HAVE_OFFER &&
         retries < CONFIG_NETUTILS_DHCPC_RETRIES);

  /* If no DHCPLEASE recveived here, error out */

  if (state != STATE_HAVE_LEASE)
    {
      return ERROR;
    }

  ninfo("Got IP address %d.%d.%d.%d\n",
        (int)((presult->ipaddr.s_addr)       & 0xff),
        (int)((presult->ipaddr.s_addr >> 8)  & 0xff),
        (int)((presult->ipaddr.s_addr >> 16) & 0xff),
        (int)((presult->ipaddr.s_addr >> 24) & 0xff));
  ninfo("Got netmask %d.%d.%d.%d\n",
        (int)((presult->netmask.s_addr)       & 0xff),
        (int)((presult->netmask.s_addr >> 8)  & 0xff),
        (int)((presult->netmask.s_addr >> 16) & 0xff),
        (int)((presult->netmask.s_addr >> 24) & 0xff));
  ninfo("Got DNS server %d.%d.%d.%d\n",
        (int)((presult->dnsaddr.s_addr)       & 0xff),
        (int)((presult->dnsaddr.s_addr >> 8)  & 0xff),
        (int)((presult->dnsaddr.s_addr >> 16) & 0xff),
        (int)((presult->dnsaddr.s_addr >> 24) & 0xff));
  ninfo("Got default router %d.%d.%d.%d\n",
        (int)((presult->default_router.s_addr)       & 0xff),
        (int)((presult->default_router.s_addr >> 8)  & 0xff),
        (int)((presult->default_router.s_addr >> 16) & 0xff),
        (int)((presult->default_router.s_addr >> 24) & 0xff));
  ninfo("Lease expires in %" PRId32 " seconds\n", presult->lease_time);
  return OK;
}

/****************************************************************************
 * Name: dhcpc_request_async
 ****************************************************************************/

int dhcpc_request_async(FAR void *handle, dhcpc_callback_t callback)
{
  FAR struct dhcpc_state_s *pdhcpc = (FAR struct dhcpc_state_s *)handle;
  int ret;

  if (!handle || !callback)
    {
      return ERROR;
    }

  if (pdhcpc->thread)
    {
      nerr("ERROR: DHCPC thread already running\n");
      return ERROR;
    }

  pdhcpc->callback = callback;
  ret = pthread_create(&pdhcpc->thread, NULL, dhcpc_run, pdhcpc);
  if (ret != 0)
    {
      nerr("ERROR: Failed to start the DHCPC thread\n");
      return ERROR;
    }

  return OK;
}
