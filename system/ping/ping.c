/****************************************************************************
 * apps/system/ping/ping.c
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "netutils/icmp_ping.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ICMP_PING_DATALEN  56

#define ICMP_NPINGS        10    /* Default number of pings */
#define ICMP_POLL_DELAY    1000  /* 1 second in milliseconds */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ping_priv_s
{
  int code;                      /* Notice code ICMP_I/E/W_XXX */
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
  printf("\nUsage: %s [-c <count>] [-i <interval>] [-W <timeout>] "
         "[-s <size>] <hostname>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <hostname> is either an IPv4 address or the name of "
         "the remote host\n");
  printf("   that is requested the ICMPv4 ECHO reply.\n");
#else
  printf("\nUsage: %s [-c <count>] [-i <interval>] [-W <timeout>] "
         "[-s <size>] <ip-address>\n", progname);
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  <ip-address> is the IPv4 address request the ICMP "
         "ECHO reply.\n");
#endif
  printf("  -c <count> determines the number of pings.  Default %u.\n",
         ICMP_NPINGS);
  printf("  -i <interval> is the default delay between pings "
         "(milliseconds).\n");
  printf("    Default %d.\n", ICMP_POLL_DELAY);
  printf("  -W <timeout> is the timeout for wait response "
         "(milliseconds).\n");
  printf("    Default %d.\n", ICMP_POLL_DELAY);
  printf("  -s <size> specifies the number of data bytes to be sent.  "
         "Default %u.\n",
         ICMP_PING_DATALEN);
  printf("  -h shows this text and exits.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: ping_result
 ****************************************************************************/

static void ping_result(FAR const struct ping_result_s *result)
{
  FAR struct ping_priv_s *priv = result->info->priv;

  if (result->code < 0)
    {
      priv->code = result->code;
    }

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
               (unsigned int)(result->dest.s_addr) & 0xff,
               (unsigned int)(result->dest.s_addr >> 8) & 0xff,
               (unsigned int)(result->dest.s_addr >> 16) & 0xff,
               (unsigned int)(result->dest.s_addr >> 24) & 0xff,
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
               (unsigned int)(result->dest.s_addr) & 0xff,
               (unsigned int)(result->dest.s_addr >> 8) & 0xff,
               (unsigned int)(result->dest.s_addr >> 16) & 0xff,
               (unsigned int)(result->dest.s_addr >> 24) & 0xff,
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
               (unsigned int)(result->dest.s_addr) & 0xff,
               (unsigned int)(result->dest.s_addr >> 8) & 0xff,
               (unsigned int)(result->dest.s_addr >> 16) & 0xff,
               (unsigned int)(result->dest.s_addr >> 24) & 0xff,
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

            printf("%u packets transmitted, %u received, %u%% packet loss, "
                   "time %d ms\n",
                   result->nrequests, result->nreplies, tmp, result->extra);
          }
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ping_info_s info;
  struct ping_priv_s priv;
  FAR char *endptr;
  int exitcode;
  int option;

  info.count     = ICMP_NPINGS;
  info.datalen   = ICMP_PING_DATALEN;
  info.delay     = ICMP_POLL_DELAY;
  info.timeout   = ICMP_POLL_DELAY;
  info.callback  = ping_result;
  info.priv      = &priv;
  priv.code      = ICMP_I_OK;

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
  icmp_ping(&info);
  return priv.code < 0 ? EXIT_FAILURE: EXIT_SUCCESS;

errout_with_usage:
  optind = 0;
  show_usage(argv[0], exitcode);
  return exitcode;  /* Not reachable */
}
