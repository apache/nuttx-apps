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

#include <nuttx/net/icmp.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Positive number represent information */

#define ICMP_I_BEGIN       0   /* extra: not used      */
#define ICMP_I_ROUNDTRIP   1   /* extra: packet delay  */
#define ICMP_I_FINISH      2   /* extra: elapsed time  */

/* Negative odd number represent error(unrecoverable) */

#define ICMP_E_HOSTIP      -1  /* extra: not used      */
#define ICMP_E_MEMORY      -3  /* extra: not used      */
#define ICMP_E_SOCKET      -5  /* extra: error code    */
#define ICMP_E_SENDTO      -7  /* extra: error code    */
#define ICMP_E_SENDSMALL   -9  /* extra: sent bytes    */
#define ICMP_E_POLL        -11 /* extra: error code    */
#define ICMP_E_RECVFROM    -13 /* extra: error code    */
#define ICMP_E_RECVSMALL   -15 /* extra: recv bytes    */

/* Negative even number represent warning(recoverable) */

#define ICMP_W_TIMEOUT     -2  /* extra: timeout value */
#define ICMP_W_IDDIFF      -4  /* extra: recv id       */
#define ICMP_W_SEQNOBIG    -6  /* extra: recv seqno    */
#define ICMP_W_SEQNOSMALL  -8  /* extra: recv seqno    */
#define ICMP_W_RECVBIG     -10 /* extra: recv bytes    */
#define ICMP_W_DATADIFF    -12 /* extra: not used      */
#define ICMP_W_TYPE        -14 /* extra: recv type     */

#define ICMP_PING_DATALEN  56
#define ICMP_IOBUFFER_SIZE(x) (sizeof(struct icmp_hdr_s) + (x))

#define ICMP_NPINGS        10    /* Default number of pings */
#define ICMP_POLL_DELAY    1000  /* 1 second in milliseconds */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ping_result_s;

struct ping_info_s
{
  FAR const char *hostname; /* Host name to ping */
  uint16_t count;           /* Number of pings requested */
  uint16_t datalen;         /* Number of bytes to be sent */
  uint16_t nrequests;       /* Number of ICMP ECHO requests sent */
  uint16_t nreplies;        /* Number of matching ICMP ECHO replies received */
  uint16_t delay;           /* Deciseconds to delay between pings */
  uint16_t timeout;         /* Deciseconds to wait response before timeout */
  FAR void *priv;           /* Private context for callback */
  void (*callback)(FAR const struct ping_result_s *result);
};

struct ping_result_s
{
  int code;                 /* Notice code ICMP_I/E/W_XXX */
  int extra;                /* Extra information for code */
  struct in_addr dest;      /* Target address to ping */
  uint16_t nrequests;       /* Number of ICMP ECHO requests sent */
  uint16_t nreplies;        /* Number of matching ICMP ECHO replies received */
  uint16_t outsize;         /* Bytes(include ICMP header) to be sent */
  uint16_t id;              /* ICMP_ECHO id */
  uint16_t seqno;           /* ICMP_ECHO seqno */
  FAR const struct ping_info_s *info;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTE: This will not work in the kernel build where there will be a
 * separate instance of g_pingid in every process space.
 */

static uint16_t g_pingid = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping_newid
 ****************************************************************************/

static inline uint16_t ping_newid(void)
{
  /* Revisit:  No thread safe */

  return ++g_pingid;
}

/****************************************************************************
 * Name: ping_gethostip
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

static int ping_gethostip(FAR const char *hostname, FAR struct in_addr *dest)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  /* Netdb DNS client support is enabled */

  FAR struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL)
    {
      nerr("ERROR: gethostbyname failed: %d\n", h_errno);
      return -ENOENT;
    }
  else if (he->h_addrtype == AF_INET)
    {
       memcpy(dest, he->h_addr, sizeof(in_addr_t));
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

  int ret = inet_pton(AF_INET, hostname, dest);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or -1
   * with errno set to EAFNOSUPPORT if the address family argument is
   * unsupported.
   */

  return (ret > 0) ? OK : ERROR;

#endif /* CONFIG_LIBC_NETDB */
}

/****************************************************************************
 * Name: icmp_callback
 ****************************************************************************/

static void icmp_callback(FAR struct ping_result_s *result, int code,
                          int extra)
{
  result->code = code;
  result->extra = extra;
  result->info->callback(result);
}

/****************************************************************************
 * Name: icmp_ping
 ****************************************************************************/

static void icmp_ping(FAR const struct ping_info_s *info)
{
  struct ping_result_s result;
  struct sockaddr_in destaddr;
  struct sockaddr_in fromaddr;
  struct icmp_hdr_s outhdr;
  FAR struct icmp_hdr_s *inhdr;
  struct pollfd recvfd;
  FAR uint8_t *iobuffer;
  FAR uint8_t *ptr;
  int32_t elapsed;
  clock_t kickoff;
  clock_t start;
  socklen_t addrlen;
  ssize_t nsent;
  ssize_t nrecvd;
  bool retry;
  int sockfd;
  int ret;
  int ch;
  int i;

  /* Initialize result structure */

  memset(&result, 0, sizeof(result));
  result.info    = info;
  result.id      = ping_newid();
  result.outsize = ICMP_IOBUFFER_SIZE(info->datalen);
  if (ping_gethostip(info->hostname, &result.dest) < 0)
    {
      icmp_callback(&result, ICMP_E_HOSTIP, 0);
      return;
    }

  /* Allocate memory to hold ping buffer */

  iobuffer = (FAR uint8_t *)malloc(result.outsize);
  if (iobuffer == NULL)
    {
      icmp_callback(&result, ICMP_E_MEMORY, 0);
      return;
    }

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (sockfd < 0)
    {
      icmp_callback(&result, ICMP_E_SOCKET, errno);
      free(iobuffer);
      return;
    }

  kickoff = clock();

  memset(&destaddr, 0, sizeof(struct sockaddr_in));
  destaddr.sin_family      = AF_INET;
  destaddr.sin_port        = 0;
  destaddr.sin_addr.s_addr = result.dest.s_addr;

  memset(&outhdr, 0, sizeof(struct icmp_hdr_s));
  outhdr.type              = ICMP_ECHO_REQUEST;
  outhdr.id                = htons(result.id);
  outhdr.seqno             = htons(result.seqno);

  icmp_callback(&result, ICMP_I_BEGIN, 0);

  while (result.nrequests < info->count)
    {
      /* Copy the ICMP header into the I/O buffer */

      memcpy(iobuffer, &outhdr, sizeof(struct icmp_hdr_s));

     /* Add some easily verifiable payload data */

      ptr = &iobuffer[sizeof(struct icmp_hdr_s)];
      ch  = 0x20;

      for (i = 0; i < info->datalen; i++)
        {
          *ptr++ = ch;
          if (++ch > 0x7e)
            {
              ch = 0x20;
            }
        }

      start = clock();
      nsent = sendto(sockfd, iobuffer, result.outsize, 0,
                     (FAR struct sockaddr*)&destaddr,
                     sizeof(struct sockaddr_in));
      if (nsent < 0)
        {
          icmp_callback(&result, ICMP_E_SENDTO, errno);
          goto done;
        }
      else if (nsent != result.outsize)
        {
          icmp_callback(&result, ICMP_E_SENDSMALL, nsent);
          goto done;
        }

      result.nrequests++;

      elapsed = 0;
      do
        {
          retry           = false;

          recvfd.fd       = sockfd;
          recvfd.events   = POLLIN;
          recvfd.revents  = 0;

          ret = poll(&recvfd, 1, info->timeout - elapsed);
          if (ret < 0)
            {
              icmp_callback(&result, ICMP_E_POLL, errno);
              goto done;
            }
          else if (ret == 0)
            {
              icmp_callback(&result, ICMP_W_TIMEOUT, info->timeout);
              continue;
            }

          /* Get the ICMP response (ignoring the sender) */

          addrlen = sizeof(struct sockaddr_in);
          nrecvd  = recvfrom(sockfd, iobuffer, result.outsize, 0,
                             (FAR struct sockaddr *)&fromaddr, &addrlen);
          if (nrecvd < 0)
            {
              icmp_callback(&result, ICMP_E_RECVFROM, errno);
              goto done;
            }
          else if (nrecvd < sizeof(struct icmp_hdr_s))
            {
              icmp_callback(&result, ICMP_E_RECVSMALL, nrecvd);
             goto done;
            }

          elapsed = (unsigned int)TICK2MSEC(clock() - start);
          inhdr   = (FAR struct icmp_hdr_s *)iobuffer;

          if (inhdr->type == ICMP_ECHO_REPLY)
            {
              if (ntohs(inhdr->id) != result.id)
                {
                  icmp_callback(&result, ICMP_W_IDDIFF, ntohs(inhdr->id));
                  retry = true;
                }
              else if (ntohs(inhdr->seqno) > result.seqno)
                {
                  icmp_callback(&result, ICMP_W_SEQNOBIG, ntohs(inhdr->seqno));
                  retry = true;
                }
              else
                {
                  bool verified = true;
                  int32_t pktdelay = elapsed;

                  if (ntohs(inhdr->seqno) < result.seqno)
                    {
                      icmp_callback(&result, ICMP_W_SEQNOSMALL, ntohs(inhdr->seqno));
                      pktdelay += info->delay;
                      retry     = true;
                    }

                  icmp_callback(&result, ICMP_I_ROUNDTRIP, pktdelay);

                  /* Verify the payload data */

                  if (nrecvd != result.outsize)
                    {
                      icmp_callback(&result, ICMP_W_RECVBIG, nrecvd);
                      verified = false;
                    }
                  else
                    {
                      ptr = &iobuffer[sizeof(struct icmp_hdr_s)];
                      ch  = 0x20;

                      for (i = 0; i < info->datalen; i++, ptr++)
                        {
                          if (*ptr != ch)
                            {
                              icmp_callback(&result, ICMP_W_DATADIFF, 0);
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
                      result.nreplies++;
                    }
                }
            }
          else
            {
              icmp_callback(&result, ICMP_W_TYPE, inhdr->type);
            }
        }
      while (retry && info->delay > elapsed && info->timeout > elapsed);

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

      outhdr.seqno = htons(++result.seqno);
    }

done:
  icmp_callback(&result, ICMP_I_FINISH, TICK2MSEC(clock() - kickoff));
  close(sockfd);
  free(iobuffer);
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  printf("\nUsage: %s [-c <count>] [-i <interval>] [-W <timeout>] [-s <size>] <hostname>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <hostname> is either an IPv4 address or the name of the remote host\n");
  printf("   that is requested the ICMPv4 ECHO reply.\n");
#else
  printf("\nUsage: %s [-c <count>] [-i <interval>] [-W <timeout>] [-s <size>] <ip-address>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <ip-address> is the IPv4 address request the ICMP ECHO reply.\n");
#endif
  printf("  -c <count> determines the number of pings.  Default %u.\n",
         ICMP_NPINGS);
  printf("  -i <interval> is the default delay between pings (milliseconds).\n");
  printf("    Default %d.\n", ICMP_POLL_DELAY);
  printf("  -W <timeout> is the timeout for wait response (milliseconds).\n");
  printf("    Default %d.\n", ICMP_POLL_DELAY);
  printf("  -s <size> specifies the number of data bytes to be sent.  Default %u.\n",
         ICMP_PING_DATALEN);
  printf("  -h shows this text and exits.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: ping_result
 ****************************************************************************/

static void ping_result(FAR const struct ping_result_s *result)
{
  switch (result->code)
    {
      case ICMP_E_HOSTIP:
        fprintf(stderr, "ERROR: ping_gethostip(%s) failed\n",
                result->info->hostname);
        break;

      case ICMP_E_MEMORY:
        fprintf(stderr, "ERROR: Failed to allocate memory\n");
        break;

      case ICMP_E_SOCKET:
        fprintf(stderr, "ERROR: socket() failed: %d\n", result->extra);
        break;

      case ICMP_I_BEGIN:
        printf("PING %u.%u.%u.%u %u bytes of data\n",
               (result->dest.s_addr      ) & 0xff,
               (result->dest.s_addr >> 8 ) & 0xff,
               (result->dest.s_addr >> 16) & 0xff,
               (result->dest.s_addr >> 24) & 0xff,
               result->info->datalen);
        break;

      case ICMP_E_SENDTO:
        fprintf(stderr, "ERROR: sendto failed at seqno %u: %d\n",
                result->seqno, result->extra);
        break;

      case ICMP_E_SENDSMALL:
        fprintf(stderr, "ERROR: sendto returned %d, expected %u\n",
                result->extra, result->outsize);
        break;

      case ICMP_E_POLL:
        fprintf(stderr, "ERROR: poll failed: %d\n", result->extra);
        break;

      case ICMP_W_TIMEOUT:
        printf("No response from %u.%u.%u.%u: icmp_seq=%u time=%d ms\n",
               (result->dest.s_addr      ) & 0xff,
               (result->dest.s_addr >> 8 ) & 0xff,
               (result->dest.s_addr >> 16) & 0xff,
               (result->dest.s_addr >> 24) & 0xff,
               result->seqno, result->extra);
        break;

      case ICMP_E_RECVFROM:
        fprintf(stderr, "ERROR: recvfrom failed: %d\n", result->extra);
        break;

      case ICMP_E_RECVSMALL:
        fprintf(stderr, "ERROR: short ICMP packet: %d\n", result->extra);
        break;

      case ICMP_W_IDDIFF:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply with ID %d.  "
                "Expected %u\n",
                result->extra, result->id);
        break;

      case ICMP_W_SEQNOBIG:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply to sequence %d.  "
                "Expected <= %u\n",
                result->extra, result->seqno);
        break;

      case ICMP_W_SEQNOSMALL:
        fprintf(stderr, "WARNING: Received after timeout\n");
        break;

      case ICMP_I_ROUNDTRIP:
        printf("%u bytes from %u.%u.%u.%u: icmp_seq=%u time=%d ms\n",
               result->info->datalen,
               (result->dest.s_addr      ) & 0xff,
               (result->dest.s_addr >> 8 ) & 0xff,
               (result->dest.s_addr >> 16) & 0xff,
               (result->dest.s_addr >> 24) & 0xff,
               result->seqno, result->extra);
        break;

      case ICMP_W_RECVBIG:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply with different payload "
                "size: %d vs %u\n",
                result->extra, result->outsize);
        break;

      case ICMP_W_DATADIFF:
        fprintf(stderr, "WARNING: Echoed data corrupted\n");
        break;

      case ICMP_W_TYPE:
        fprintf(stderr, "WARNING: ICMP packet with unknown type: %d\n",
                result->extra);
        break;

      case ICMP_I_FINISH:
        if (result->nrequests > 0)
          {
            unsigned int tmp;

            /* Calculate the percentage of lost packets */

            tmp = (100 * (result->nrequests - result->nreplies) +
                  (result->nrequests >> 1)) /
                   result->nrequests;

            printf("%u packets transmitted, %u received, %u%% packet loss, time %d ms\n",
                   result->nrequests, result->nreplies, tmp, result->extra);
          }
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int ping_main(int argc, char **argv)
#endif
{
  struct ping_info_s info;
  FAR char *endptr;
  int exitcode;
  int option;

  info.count     = ICMP_NPINGS;
  info.datalen   = ICMP_PING_DATALEN;
  info.delay     = ICMP_POLL_DELAY;
  info.timeout   = ICMP_POLL_DELAY;
  info.callback  = ping_result;

  /* Parse command line options */

  exitcode = EXIT_FAILURE;

  while ((option = getopt(argc, argv, ":c:i:W:s:h")) != ERROR)
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

              info.count = (uint16_t)count;
            }
            break;

          case 'i':
            {
              long delay = strtol(optarg, &endptr, 10);
              if (delay < 1 || delay > UINT16_MAX)
                {
                  fprintf(stderr, "ERROR: <interval> out of range: %ld\n", delay);
                  goto errout_with_usage;
                }

              info.delay = (int16_t)delay;
            }
            break;

          case 'W':
            {
              long timeout = strtol(optarg, &endptr, 10);
              if (timeout < 1 || timeout > UINT16_MAX)
                {
                  fprintf(stderr, "ERROR: <timeout> out of range: %ld\n", timeout);
                  goto errout_with_usage;
                }

              info.timeout = (int16_t)timeout;
            }
            break;

          case 's':
            {
              long datalen = strtol(optarg, &endptr, 10);
              if (datalen < 1 || datalen > UINT16_MAX)
                {
                  fprintf(stderr, "ERROR: <size> out of range: %ld\n", datalen);
                  goto errout_with_usage;
                }

              info.datalen = (uint16_t)datalen;
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
      goto errout_with_usage;
    }

  info.hostname = argv[optind];
  icmp_ping(&info);
  return EXIT_SUCCESS;

errout_with_usage:
  optind = 0;
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */
}
