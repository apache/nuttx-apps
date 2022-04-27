/****************************************************************************
 * apps/system/ping6/ping6.c
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
#include <nuttx/clock.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fixedmath.h>

#include <arpa/inet.h>

#include "netutils/icmpv6_ping.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMPv6_PING6_DATALEN 56

#define ICMPv6_NPINGS        10    /* Default number of pings */
#define ICMPv6_POLL_DELAY    1000  /* 1 second in milliseconds */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ping6_priv_s
{
  int code;                        /* Notice code ICMP_I/E/W_XXX */
  long tmin;                       /* Minimum round trip time */
  long tmax;                       /* Maximum round trip time */
  long long tsum;                  /* Sum of all times, for doing average */
  long long tsum2;                 /* Sum2 is the sum of the squares of sum ,for doing mean deviation */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
                       noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
  printf("\nUsage: %s [-c <count>] [-i <interval>] "
         "[-W <timeout>] [-s <size>] <hostname>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <hostname> is either an IPv6 address "
         "or the name of the remote host\n");
  printf("   that is requested the ICMPv6 ECHO reply.\n");
#else
  printf("\nUsage: %s [-c <count>] [-i <interval>] [-W <timeout>] "
         "[-s <size>] <ip-address>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <ip-address> is the IPv6 address request "
         "the ICMPv6 ECHO reply.\n");
#endif
  printf("  -c <count> determines the number of pings.  Default %u.\n",
         ICMPv6_NPINGS);
  printf("  -i <interval> is the default delay between pings "
         "(milliseconds).\n");
  printf("    Default %d.\n", ICMPv6_POLL_DELAY);
  printf("  -W <timeout> is the timeout for wait response "
         "(milliseconds).\n");
  printf("    Default %d.\n", ICMPv6_POLL_DELAY);
  printf("  -s <size> specifies the number of data bytes to be sent. "
         " Default %u.\n",
         ICMPv6_PING6_DATALEN);
  printf("  -h shows this text and exits.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: ping_result
 ****************************************************************************/

static void ping6_result(FAR const struct ping6_result_s *result)
{
  FAR struct ping6_priv_s *priv = result->info->priv;
  char strbuffer[INET6_ADDRSTRLEN];

  if (result->code < 0)
    {
      priv->code = result->code;
    }

  switch (result->code)
    {
      case ICMPv6_E_HOSTIP:
        fprintf(stderr, "ERROR: ping6_gethostip(%s) failed\n",
                result->info->hostname);
        break;

      case ICMPv6_E_MEMORY:
        fprintf(stderr, "ERROR: Failed to allocate memory\n");
        break;

      case ICMPv6_E_SOCKET:
        fprintf(stderr, "ERROR: socket() failed: %ld\n", result->extra);
        break;

      case ICMPv6_I_BEGIN:
        inet_ntop(AF_INET6, result->dest.s6_addr16,
                  strbuffer, INET6_ADDRSTRLEN);
        printf("PING6 %s: %u bytes of data\n",
               strbuffer, result->info->datalen);
        break;

      case ICMPv6_E_SENDTO:
        fprintf(stderr, "ERROR: sendto failed at seqno %u: %ld\n",
                result->seqno, result->extra);
        break;

      case ICMPv6_E_SENDSMALL:
        fprintf(stderr, "ERROR: sendto returned %ld, expected %u\n",
                result->extra, result->outsize);
        break;

      case ICMPv6_E_POLL:
        fprintf(stderr, "ERROR: poll failed: %ld\n", result->extra);
        break;

      case ICMPv6_W_TIMEOUT:
        inet_ntop(AF_INET6, result->dest.s6_addr16, strbuffer,
                  INET6_ADDRSTRLEN);
        printf("No response from %s: icmp_seq=%u time=%ld ms\n",
               strbuffer, result->seqno, result->extra);
        break;

      case ICMPv6_E_RECVFROM:
        fprintf(stderr, "ERROR: recvfrom failed: %ld\n", result->extra);
        break;

      case ICMPv6_E_RECVSMALL:
        fprintf(stderr, "ERROR: short ICMP packet: %ld\n", result->extra);
        break;

      case ICMPv6_W_IDDIFF:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply with ID %ld.  "
                "Expected %u\n",
                result->extra, result->id);
        break;

      case ICMPv6_W_SEQNOBIG:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply to sequence %ld.  "
                "Expected <= %u\n",
                result->extra, result->seqno);
        break;

      case ICMPv6_W_SEQNOSMALL:
        fprintf(stderr, "WARNING: Received after timeout\n");
        break;

      case ICMPv6_I_ROUNDTRIP:
        priv->tsum += result->extra;
        priv->tsum2 += (long long)result->extra * result->extra;
        if (result->extra < priv->tmin)
          {
            priv->tmin = result->extra;
          }

        if (result->extra > priv->tmax)
          {
            priv->tmax = result->extra;
          }

        inet_ntop(AF_INET6, result->dest.s6_addr16, strbuffer,
                  INET6_ADDRSTRLEN);
        printf("%u bytes from %s icmp_seq=%u time=%ld.%ld ms\n",
               result->info->datalen, strbuffer, result->seqno,
               result->extra / USEC_PER_MSEC,
               result->extra % USEC_PER_MSEC / MSEC_PER_DSEC);
        break;

      case ICMPv6_W_RECVBIG:
        fprintf(stderr,
                "WARNING: Ignoring ICMP reply with different payload "
                "size: %ld vs %u\n",
                result->extra, result->outsize);
        break;

      case ICMPv6_W_DATADIFF:
        fprintf(stderr, "WARNING: Echoed data corrupted\n");
        break;

      case ICMPv6_W_TYPE:
        fprintf(stderr, "WARNING: ICMP packet with unknown type: %ld\n",
                result->extra);
        break;

      case ICMPv6_I_FINISH:
        if (result->nrequests > 0)
          {
            unsigned int tmp;

            /* Calculate the percentage of lost packets */

            tmp = (100 * (result->nrequests - result->nreplies) +
                   (result->nrequests >> 1)) /
                   result->nrequests;

            printf("%u packets transmitted, %u received, %u%% packet loss,"
                   "time %ld ms\n",
                   result->nrequests, result->nreplies, tmp,
                   result->extra / USEC_PER_MSEC);
            if (result->nreplies > 0)
              {
                long avg = 0;
                long long tempnum = 0;
                long tmdev = 0;

                if (priv->tsum > 0)
                  {
                    avg = priv->tsum / result->nreplies;
                    tempnum = priv->tsum2 / result->nreplies -
                              (long long)avg * avg;
                    tmdev = ub16toi(ub32sqrtub16(uitoub32(tempnum)));
                  }

                printf("rtt min/avg/max/mdev = %ld.%03ld/%ld.%03ld/"
                       "%ld.%03ld/%ld.%03ld ms\n",
                       priv->tmin / USEC_PER_MSEC,
                       priv->tmin % USEC_PER_MSEC,
                       avg / USEC_PER_MSEC, avg % USEC_PER_MSEC,
                       priv->tmax / USEC_PER_MSEC,
                       priv->tmax % USEC_PER_MSEC,
                       tmdev / USEC_PER_MSEC, tmdev % USEC_PER_MSEC);
              }
          }
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ping6_info_s info;
  struct ping6_priv_s priv;
  FAR char *endptr;
  int exitcode;
  int option;

  info.count     = ICMPv6_NPINGS;
  info.datalen   = ICMPv6_PING6_DATALEN;
  info.delay     = ICMPv6_POLL_DELAY;
  info.timeout   = ICMPv6_POLL_DELAY;
  info.callback  = ping6_result;
  info.priv      = &priv;
  priv.code      = ICMPv6_I_OK;
  priv.tmin      = LONG_MAX;
  priv.tmax      = 0;
  priv.tsum      = 0;
  priv.tsum2     = 0;

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
                  fprintf(stderr, "ERROR: <count> out of range: %ld\n",
                          count);
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
                  fprintf(stderr, "ERROR: <interval> out of range: %ld\n",
                          delay);
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
                  fprintf(stderr, "ERROR: <timeout> out of range: %ld\n",
                          timeout);
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
                  fprintf(stderr, "ERROR: <size> out of range: %ld\n",
                          datalen);
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
  icmp6_ping(&info);
  return priv.code < 0 ? EXIT_FAILURE: EXIT_SUCCESS;

errout_with_usage:
  optind = 0;
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */
}
