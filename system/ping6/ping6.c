/****************************************************************************
 * apps/system/ping/ping.c
 *
 *   Copyright (C) 2017-2018 Gregory Nutt. All rights reserved.
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
#include <sys/socket.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
#  include <netdb.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <nuttx/net/icmpv6.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMPv6_PING6_DATALEN 56
#define ICMPv6_IOBUFFER_SIZE \
  SIZEOF_ICMPV6_ECHO_REQUEST_S(0) + ICMPv6_PING6_DATALEN

#define ICMPv6_NPINGS        10    /* Default number of pings */
#define ICMPv6_POLL_DELAY    1000  /* 1 second in milliseconds */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ping6_info_s
{
  int sockfd;                /* Open IPPROTO_ICMP6 socket */
  FAR struct in6_addr dest;  /* Target address to ping */
  uint16_t count;            /* Number of pings requested */
  uint16_t nrequests;        /* Number of ICMP ECHO requests sent */
  uint16_t nreplies;         /* Number of matching ICMP ECHO replies received */
  int16_t delay;             /* Deciseconds to delay between pings */

  /* I/O buffer for data transfers */

  uint8_t iobuffer[ICMPv6_IOBUFFER_SIZE];

  /* String buffer from representing IPv6 addresses in a more human
   * readable way.
   */

  char strbuffer[INET6_ADDRSTRLEN];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTE: This will not work in the kernel build where there will be a
 * separate instance of g_ping6_id in every process space.
 */

static uint16_t g_ping6_id = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping6_newid
 ****************************************************************************/

static inline uint16_t ping6_newid(void)
{
  /* Revisit:  No thread safe */

  return ++g_ping6_id;
}

/****************************************************************************
 * Name: ping6_gethostip
 *
 * Description:
 *   Call gethostbyname() to get the IP address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *   ipv4addr - The location to return the IPv4 address.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int ping6_gethostip(FAR char *hostname, FAR struct ping6_info_s *info)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  /* Netdb support is enabled */

  FAR struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL)
    {
      nerr("ERROR: gethostbyname failed: %d\n", h_errno);
      return -ENOENT;
    }
  else if (he->h_addrtype == AF_INET6)
    {
      memcpy(&info->dest, he->h_addr, sizeof(struct in6_addr));
    }
  else
    {
      nerr("ERROR: gethostbyname returned an address of type: %d\n",
           he->h_addrtype);
      return -ENOEXEC;
    }

  return OK;

#else /* CONFIG_LIBC_NETDB */

  /* No host name support */
  /* Convert strings to numeric IPv6 address */

  int ret = inet_pton(AF_INET6, hostname, info->dest.s6_addr16);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv6 address string, or -1 with
   * errno set to EAFNOSUPPORT if the address family argument is unsupported.
   */

  return (ret > 0) ? OK : ERROR;

#endif /* CONFIG_LIBC_NETDB */
}

/****************************************************************************
 * Name: icmpv6_ping
 ****************************************************************************/

static void icmpv6_ping(FAR struct ping6_info_s *info)
{
  struct sockaddr_in6 destaddr;
  struct sockaddr_in6 fromaddr;
  struct icmpv6_echo_request_s outhdr;
  FAR struct icmpv6_echo_reply_s *inhdr;
  struct pollfd recvfd;
  FAR uint8_t *ptr;
  int32_t elapsed;
  clock_t start;
  socklen_t addrlen;
  ssize_t nsent;
  ssize_t nrecvd;
  size_t outsize;
  bool retry;
  int delay;
  int ret;
  int ch;
  int i;

  memset(&destaddr, 0, sizeof(struct sockaddr_in6));
  destaddr.sin6_family     = AF_INET6;
  destaddr.sin6_port       = 0;
  memcpy(&destaddr.sin6_addr, &info->dest, sizeof(struct in6_addr));

  memset(&outhdr, 0, SIZEOF_ICMPV6_ECHO_REQUEST_S(0));
  outhdr.type              = ICMPv6_ECHO_REQUEST;
  outhdr.id                = htons(ping6_newid());
  outhdr.seqno             = 0;

  (void)inet_ntop(AF_INET6, info->dest.s6_addr16, info->strbuffer,
                  INET6_ADDRSTRLEN);
  printf("PING6 %s: %d bytes of data\n",
         info->strbuffer, ICMPv6_PING6_DATALEN);

  while (info->nrequests < info->count)
    {
      /* Copy the ICMP header into the I/O buffer */

      memcpy(info->iobuffer, &outhdr, SIZEOF_ICMPV6_ECHO_REQUEST_S(0));

     /* Add some easily verifiable payload data */

      ptr = &info->iobuffer[SIZEOF_ICMPV6_ECHO_REQUEST_S(0)];
      ch  = 0x20;

      for (i = 0; i < ICMPv6_PING6_DATALEN; i++)
        {
          *ptr++ = ch;
          if (++ch > 0x7e)
            {
              ch = 0x20;
            }
        }

      start   = clock();
      outsize = SIZEOF_ICMPV6_ECHO_REPLY_S(0) + ICMPv6_PING6_DATALEN;
      nsent   = sendto(info->sockfd, info->iobuffer, outsize, 0,
                       (FAR struct sockaddr*)&destaddr,
                       sizeof(struct sockaddr_in6));
      if (nsent < 0)
        {
          fprintf(stderr, "ERROR: sendto failed at seqno %u: %d\n",
                  ntohs(outhdr.seqno), errno);
          return;
        }
      else if (nsent != outsize)
        {
          fprintf(stderr, "ERROR: sendto returned %ld, expected %lu\n",
                  (long)nsent, (unsigned long)outsize);
          return;
        }

      info->nrequests++;

      delay = info->delay;
      do
        {
          /* Wait for a reply with a timeout */

          retry           = false;

          recvfd.fd       = info->sockfd;
          recvfd.events   = POLLIN;
          recvfd.revents  = 0;

          ret = poll(&recvfd, 1, delay);
          if (ret < 0)
            {
              fprintf(stderr, "ERROR: poll failed: %d\n", errno);
              return;
            }
          else if (ret == 0)
            {
              (void)inet_ntop(AF_INET6, info->dest.s6_addr16,
                              info->strbuffer, INET6_ADDRSTRLEN);
              printf("No response from %s: icmp_seq=%u time=%u ms\n",
                     info->strbuffer, ntohs(outhdr.seqno), info->delay);

              continue;
            }

          /* Get the ICMP response (ignoring the sender) */

          addrlen = sizeof(struct sockaddr_in6);
          nrecvd  = recvfrom(info->sockfd, info->iobuffer,
                             ICMPv6_IOBUFFER_SIZE, 0,
                             (FAR struct sockaddr *)&fromaddr, &addrlen);
          if (nrecvd < 0)
            {
              fprintf(stderr, "ERROR: recvfrom failed: %d\n", errno);
              return;
            }
          else if (nrecvd < SIZEOF_ICMPV6_ECHO_REPLY_S(0))
            {
              fprintf(stderr, "ERROR: short ICMP packet: %ld\n", (long)nrecvd);
              return;
            }

          elapsed = (unsigned int)TICK2MSEC(clock() - start);
          inhdr   = (FAR struct icmpv6_echo_reply_s *)info->iobuffer;

          if (inhdr->type == ICMPv6_ECHO_REPLY)
            {
              if (inhdr->id != outhdr.id)
                {
                  fprintf(stderr,
                          "WARNING: Ignoring ICMP reply with ID %u.  "
                          "Expected %u\n",
                          ntohs(inhdr->id), ntohs(outhdr.id));
                  retry = true;
                }
              else if (ntohs(inhdr->seqno) > ntohs(outhdr.seqno))
                {
                  fprintf(stderr,
                          "WARNING: Ignoring ICMP reply to sequence %u.  "
                          "Expected <= &u\n",
                          ntohs(inhdr->seqno), ntohs(outhdr.seqno));
                  retry = true;
                }
              else
                {
                  bool verified = true;
                  int32_t pktdelay = elapsed;

                  if (ntohs(inhdr->seqno) < ntohs(outhdr.seqno))
                    {
                      fprintf(stderr, "WARNING: Received after timeout\n");
                      pktdelay += info->delay;
                      retry     = true;
                    }

                  (void)inet_ntop(AF_INET6, fromaddr.sin6_addr.s6_addr16,
                                  info->strbuffer, INET6_ADDRSTRLEN);
                  printf("%ld bytes from %s icmp_seq=%u time=%u ms\n",
                         nrecvd - SIZEOF_ICMPV6_ECHO_REPLY_S(0),
                         info->strbuffer, ntohs(inhdr->seqno), pktdelay);

                  /* Verify the payload data */

                  if (nrecvd != outsize)
                    {
                      fprintf(stderr,
                              "WARNING: Ignoring ICMP reply with different payload "
                              "size: %ld vs %lu\n",
                              (long)nrecvd, (unsigned long)outsize);
                      verified = false;
                    }
                  else
                    {
                      ptr = &info->iobuffer[SIZEOF_ICMPV6_ECHO_REPLY_S(0)];
                      ch  = 0x20;

                      for (i = 0; i < ICMPv6_PING6_DATALEN; i++, ptr++)
                        {
                          if (*ptr != ch)
                            {
                              fprintf(stderr, "WARNING: Echoed data corrupted\n");
                              verified = false;
                              break;
                            }

                          if (++ch > 0x7e)
                            {
                              ch = 0x20;
                            }
                        }
                    }

                  /* Only count the number of good replies */

                  if (verified)
                    {
                      info->nreplies++;
                    }
                }
            }
          else
            {
              fprintf(stderr, "WARNING: ICMP packet with unknown type: %u\n",
                      inhdr->type);
            }

          delay -= elapsed;
        }
      while (retry && delay > 0);

      /* Wait if necessary to preserved the requested ping rate */

      elapsed = (unsigned int)TICK2MSEC(clock() - start);
      if (elapsed < info->delay)
        {
          struct timespec rqt;
          unsigned int remaining;
          unsigned int sec;
          unsigned int frac;  /* In deciseconds */

          remaining   = info->delay - elapsed;
          sec         = remaining / MSEC_PER_SEC;
          frac        = remaining - MSEC_PER_SEC * sec;

          rqt.tv_sec  = sec;
          rqt.tv_nsec = frac * NSEC_PER_MSEC;

          (void)nanosleep(&rqt, NULL);
        }

      outhdr.seqno = htons(ntohs(outhdr.seqno) + 1);
    }
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  printf("\nUsage: %s [-c <count>] [-i <interval>] <hostname>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <hostname> is either an IPv6 address or the name of the remote host\n");
  printf("   that is requested the ICMPv6 ECHO reply.\n");
#else
  printf("\nUsage: %s [-c <count>] [-i <interval>] <ip-address>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <ip-address> is the IPv6 address request the ICMPv6 ECHO reply.\n");
#endif
  printf("  -c <count> determines the number of pings.  Default %u.\n",
         ICMPv6_NPINGS);
  printf("  -i <interval> is the default delay between pings (milliseconds).\n");
  printf("    Default %d.\n", ICMPv6_POLL_DELAY);
  printf("  -h shows this text and exits.\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int ping6_main(int argc, char **argv)
#endif
{
  FAR struct ping6_info_s *info;
  FAR char *endptr;
  clock_t start;
  int32_t elapsed;
  int exitcode;
  int option;

  /* Allocate memory to hold ping information */

  info = (FAR struct ping6_info_s *)zalloc(sizeof(struct ping6_info_s));
  if (info == NULL)
    {
      fprintf(stderr, "ERROR: Failed to allocate memory\n", argv[1]);
      return EXIT_FAILURE;
    }

  info->count = ICMPv6_NPINGS;
  info->delay = ICMPv6_POLL_DELAY;

  /* Parse command line options */

  exitcode = EXIT_FAILURE;

  while ((option = getopt(argc, argv, ":c:i:h")) != ERROR)
    {
      switch (option)
        {
          case 'c':
            {
              long count = strtol(optarg, &endptr, 10);
              if (count < 1 || count > UINT16_MAX)
                {
                  fprintf(stderr, "ERROR: <count> out of range: %ld\n", count);
                  goto errout_with_usage;
                }

              info->count = (uint16_t)count;
            }
            break;

          case 'i':
            {
              long delay = strtol(optarg, &endptr, 10);
              if (delay < 1 || delay > INT16_MAX)
                {
                  fprintf(stderr, "ERROR: <interval> out of range: %ld\n", delay);
                  goto errout_with_usage;
                }

              info->delay = (int16_t)delay;
            }
            break;

          case 'h':
            exitcode = EXIT_SUCCESS;
            goto errout_with_usage;

          case ':':
            fprintf(stderr, "ERROR: Missing required argument\n");
            goto errout_with_usage;

          case '?':
          default:
            fprintf(stderr, "ERROR: Unrecognized option\n");
            goto errout_with_usage;
        }
    }

  /* There should be one final parameters remaining on the command line */

  if (optind >= argc)
    {
      printf("ERROR: Missing required <ip-address> argument\n");
      free(info);
      show_usage(argv[0], EXIT_FAILURE);
    }

  if (ping6_gethostip(argv[optind], info) < 0)
    {
      fprintf(stderr, "ERROR: ping6_gethostip(%s) failed\n", argv[optind]);
      goto errout_with_info;
    }

  info->sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMP6);
  if (info->sockfd < 0)
    {
      fprintf(stderr, "ERROR: socket() failed: %d\n", errno);
      goto errout_with_info;
    }

  start = clock();
  icmpv6_ping(info);

  /* Get the total elapsed time */

  elapsed = (int32_t)TICK2MSEC(clock() - start);

  if (info->nrequests > 0)
    {
      unsigned int tmp;

      /* Calculate the percentage of lost packets */

      tmp = (100 * (info->nrequests - info->nreplies) + (info->nrequests >> 1)) /
             info->nrequests;

      printf("%u packets transmitted, %u received, %u%% packet loss, time %ld ms\n",
             info->nrequests, info->nreplies, tmp, (long)elapsed);
    }

  close(info->sockfd);
  free(info);
  return EXIT_SUCCESS;

errout_with_usage:
  free(info);
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */

errout_with_info:
  free(info);
  return EXIT_FAILURE;
}
