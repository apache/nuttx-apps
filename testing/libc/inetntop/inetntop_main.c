/****************************************************************************
 * apps/testing/libc/inetntop/inetntop_main.c
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

#include <sys/param.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ntop_case_s
{
  uint16_t words[8];          /* The address, as eight 16-bit fields */
  FAR const char *text;       /* The expected text */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Convert src with a buffer of size bytes.  The text must be expect, or
 * the call must fail with ENOSPC if expect is NULL.  Return the number of
 * errors: 0 or 1.
 */

static int check_size(int af, FAR const void *src, socklen_t size,
                      FAR const char *expect, FAR const char *name)
{
  FAR const char *ret;
  char buf[INET6_ADDRSTRLEN + 8];

  memset(buf, 'X', sizeof(buf));
  errno = 0;
  ret = inet_ntop(af, src, buf, size);
  if (expect != NULL && ret == buf && strcmp(buf, expect) == 0)
    {
      return 0;
    }

  if (expect == NULL && ret == NULL && errno == ENOSPC)
    {
      return 0;
    }

  printf("FAIL: %s, size %u: returned \"%s\", errno %d, expected \"%s\"\n",
         name, size, ret != NULL ? buf : "NULL", errno,
         expect != NULL ? expect : "ENOSPC");
  return 1;
}

/* The text must fit in strlen() + 1 bytes and fail with ENOSPC in
 * strlen() bytes.
 */

static int check(int af, FAR const void *src, FAR const char *expect)
{
  socklen_t len = strlen(expect);
  int nerrors;

  nerrors  = check_size(af, src, len + 1, expect, expect);
  nerrors += check_size(af, src, len, NULL, expect);
  nerrors += check_size(af, src, INET6_ADDRSTRLEN, expect, expect);
  if (nerrors == 0)
    {
      printf("PASS: \"%s\"\n", expect);
    }

  return nerrors;
}

static int check_ipv6(FAR const struct ntop_case_s *tc)
{
  struct in6_addr addr;
  int i;

  for (i = 0; i < 8; i++)
    {
      addr.s6_addr[2 * i]     = tc->words[i] >> 8;
      addr.s6_addr[2 * i + 1] = tc->words[i] & 0xff;
    }

  return check(AF_INET6, &addr, tc->text);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  static const struct ntop_case_s cases[] =
  {
    /* No 0 field, and the longest text */

    {
      {1, 2, 3, 4, 5, 6, 7, 8}, "1:2:3:4:5:6:7:8"
    },
    {
      {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
      "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"
    },

    /* "::" at the start, in the middle and at the end */

    {
      {0, 0, 0, 0, 0, 0, 0, 0}, "::"
    },
    {
      {0, 0, 0, 0, 0, 0, 0, 1}, "::1"
    },
    {
      {1, 0, 0, 0, 0, 0, 0, 0}, "1::"
    },
    {
      {0xfe80, 0, 0, 0, 0, 0, 0xab, 1}, "fe80::ab:1"
    },
    {
      {0x2001, 0xdb8, 0, 0, 0, 0xff00, 0x42, 0x8329},
      "2001:db8::ff00:42:8329"
    },

    /* RFC 5952 4.2.2: one 0 field is not shortened */

    {
      {1, 0, 2, 3, 4, 5, 6, 7}, "1:0:2:3:4:5:6:7"
    },
    {
      {0, 1, 2, 3, 4, 5, 6, 7}, "0:1:2:3:4:5:6:7"
    },
    {
      {1, 2, 3, 4, 5, 6, 7, 0}, "1:2:3:4:5:6:7:0"
    },
    {
      {0x2001, 0xdb8, 0, 1, 1, 1, 1, 1}, "2001:db8:0:1:1:1:1:1"
    },

    /* RFC 5952 4.2.3: the longest run, the first one on a tie */

    {
      {1, 0, 0, 2, 0, 0, 0, 3}, "1:0:0:2::3"
    },
    {
      {1, 0, 0, 2, 0, 0, 3, 4}, "1::2:0:0:3:4"
    },
    {
      {0x2001, 0xdb8, 0, 0, 1, 0, 0, 1}, "2001:db8::1:0:0:1"
    },

    /* RFC 5952 4.3: lowercase */

    {
      {0x2001, 0xdb8, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd, 0xeeee, 0xaaaa},
      "2001:db8:aaaa:bbbb:cccc:dddd:eeee:aaaa"
    },

    /* RFC 5952 5: IPv4-mapped and IPv4-compatible, but not :: and ::1 */

    {
      {0, 0, 0, 0, 0, 0xffff, 0x7f00, 1}, "::ffff:127.0.0.1"
    },
    {
      {0, 0, 0, 0, 0, 0xffff, 0, 0}, "::ffff:0.0.0.0"
    },
    {
      {0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff}, "::ffff:255.255.255.255"
    },
    {
      {0, 0, 0, 0, 0, 0, 0x102, 0x304}, "::1.2.3.4"
    },
    {
      {0, 0, 0, 0, 0, 0, 1, 0}, "::0.1.0.0"
    },
    {
      {0, 0, 0, 0, 0, 0, 0, 0x102}, "::102"
    },

    /* Near forms that are not IPv4-mapped */

    {
      {0, 0, 0, 0, 0, 0xfffe, 0x102, 0x304}, "::fffe:102:304"
    },
    {
      {1, 0, 0, 0, 0, 0xffff, 0x102, 0x304}, "1::ffff:102:304"
    },
    {
      {0, 0, 0, 0, 0xffff, 0, 0x102, 0x304}, "::ffff:0:102:304"
    },
    {
      {0x64, 0xff9b, 0, 0, 0, 0, 0xc000, 0x221}, "64:ff9b::c000:221"
    },
  };

  static const uint8_t ipv4[][4] =
  {
    {0, 0, 0, 0},
    {1, 2, 3, 4},
    {192, 168, 10, 200},
    {255, 255, 255, 255},
  };

  static const FAR char * const ipv4text[] =
  {
    "0.0.0.0", "1.2.3.4", "192.168.10.200", "255.255.255.255",
  };

  int nerrors = 0;
  size_t i;

  printf("inetntop: inet_ntop() test\n");

  for (i = 0; i < nitems(cases); i++)
    {
      nerrors += check_ipv6(&cases[i]);
    }

  for (i = 0; i < nitems(ipv4); i++)
    {
      nerrors += check(AF_INET, ipv4[i], ipv4text[i]);
    }

  printf("inetntop: %zu cases, %d errors\n",
         nitems(cases) + nitems(ipv4), nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
