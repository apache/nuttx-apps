/****************************************************************************
 * apps/system/ntpc/ntpc_status.c
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

#include <sys/types.h>
#include <sys/socket.h>

#include <inttypes.h>
#ifdef CONFIG_LIBC_NETDB
#include <netdb.h>
#endif
#include <stdlib.h>
#include <stdio.h>

#include "netutils/ntpclient.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* "-", uint64_t, ".", 9 digits fraction part, NUL */

#define	NTP_TIME_STR_MAX_LEN (1 + 21 + 1 + 9 + 1)

static void
format_ntptimestamp(int64_t ts, FAR char *buf)
{
  FAR const char *sign;
  uint64_t absts;

  if (ts < 0)
    {
      sign = "-";
      absts = -ts;
    }
  else
    {
      sign = "";
      absts = ts;
    }

  sprintf(buf, "%s%" PRIu64 ".%09" PRIu64,
          sign, absts >> 32,
          ((absts & 0xffffffff) * NSEC_PER_SEC) >> 32);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ntpcstatus_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct ntpc_status_s status;
  unsigned int i;
  int ret;

  ret = ntpc_status(&status);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ntpc_status() failed\n");
      return EXIT_FAILURE;
    }

  printf("The number of last samples: %u\n", status.nsamples);
  for (i = 0; i < status.nsamples; i++)
    {
      FAR const struct sockaddr *sa;
      socklen_t salen;
      FAR const char *name = "<noname>";
      char offset_buf[NTP_TIME_STR_MAX_LEN];
      char delay_buf[NTP_TIME_STR_MAX_LEN];

      sa = status.samples[i].srv_addr;
      salen = (sa->sa_family == AF_INET) ? sizeof(struct sockaddr_in)
                                         : sizeof(struct sockaddr_in6);
#ifdef CONFIG_LIBC_NETDB
      char hbuf[NI_MAXHOST];

      ret = getnameinfo(sa, salen,
                        hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
      if (ret == 0)
        {
          name = hbuf;
        }
      else
        {
          fprintf(stderr, "WARNING: getnameinfo failed: %s\n",
                  gai_strerror(ret));
        }
#endif

      format_ntptimestamp(status.samples[i].offset, offset_buf);
      format_ntptimestamp(status.samples[i].delay, delay_buf);
      printf("[%u] srv %s offset %s delay %s\n",
             i, name, offset_buf, delay_buf);
    }

  return EXIT_SUCCESS;
}
