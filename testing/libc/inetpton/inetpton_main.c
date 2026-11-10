/****************************************************************************
 * apps/testing/libc/inetpton/inetpton_main.c
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
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pton_case_s
{
  FAR const char *src;
  uint8_t addr[16];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* A valid address: inet_pton() must return 1 and the expected bytes */

static int check_valid(FAR const struct pton_case_s *tc)
{
  struct in6_addr addr;
  size_t i;
  int ret;

  memset(&addr, 0xaa, sizeof(addr));
  ret = inet_pton(AF_INET6, tc->src, &addr);
  if (ret == 1 && memcmp(&addr, tc->addr, sizeof(addr)) == 0)
    {
      printf("PASS: \"%s\"\n", tc->src);
      return 0;
    }

  printf("FAIL: \"%s\": returned %d, address", tc->src, ret);
  for (i = 0; i < sizeof(addr); i++)
    {
      printf(" %02x", addr.s6_addr[i]);
    }

  printf("\n");
  return 1;
}

/* Not an address: inet_pton() must return 0 */

static int check_invalid(FAR const char *src)
{
  struct in6_addr addr;
  int ret;

  ret = inet_pton(AF_INET6, src, &addr);
  if (ret == 0)
    {
      printf("PASS: \"%s\" is invalid\n", src);
      return 0;
    }

  printf("FAIL: \"%s\": returned %d, expected 0\n", src, ret);
  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  static const struct pton_case_s valid[] =
  {
    /* RFC 4291 section 2.2, forms 1 and 2 */

    {
      "1:2:3:4:5:6:7:8",
      {0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8}
    },
    {
      "::",
      {0}
    },
    {
      "::1",
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
    },
    {
      "1::",
      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    },
    {
      "FE80::aB:1",
      {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xab, 0, 1}
    },
    {
      "1:2:3:4:5:6:7::",
      {0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 0}
    },
    {
      "::2:3:4:5:6:7:8",
      {0, 0, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8}
    },

    /* Form 3: the last 32 bits in the IPv4 dotted-decimal form */

    {
      "::ffff:127.0.0.1",
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1}
    },
    {
      "::FFFF:255.255.255.255",
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 255, 255, 255, 255}
    },
    {
      "::1.2.3.4",
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4}
    },
    {
      "1:2:3:4:5:6:1.2.3.4",
      {0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 1, 2, 3, 4}
    },
    {
      "1:2:3:4:5::1.2.3.4",
      {0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 0, 1, 2, 3, 4}
    },
    {
      "64:ff9b::192.0.2.33",
      {0, 0x64, 0xff, 0x9b, 0, 0, 0, 0, 0, 0, 0, 0, 192, 0, 2, 33}
    },
  };

  static const FAR char * const invalid[] =
  {
    /* Wrong field count, "::" and ':' */

    "", ":", ":::", "1", "1:2:3", "1:2:3:4:5:6:7", "1:2:3:4:5:6:7:8:9",
    "1:2:3:4::5:6:7:8", "1:2:3:4:5:6:7::8", "::1:2:3:4:5:6:7:8",
    "1::2::3", "1:::2", ":1::2", "1::2:", ":1", "1:", "12345::1", "g::1",

    /* Wrong IPv4 dotted-decimal part */

    "1.2.3.4", "::1.2.3", "::1.2.3.4.5", "1.2.3.4::", "::ffff:256.1.1.1",
    "::1.2.3.4:1", "::1..2.3", "::1.2.3.", "::.1.2.3", ":1.2.3.4",
    "::ffff:1.2.3.1234", "::ffff1.2.3.4", "::0x1.2.3.4",
    "1:2:3:4:5:1.2.3.4", "1:2:3:4:5:6:7:1.2.3.4",
    "1:2:3:4:5:6:7::1.2.3.4", "1::2::1.2.3.4",
  };

  int nerrors = 0;
  size_t i;

  printf("inetpton: inet_pton(AF_INET6) test\n");

  for (i = 0; i < nitems(valid); i++)
    {
      nerrors += check_valid(&valid[i]);
    }

  for (i = 0; i < nitems(invalid); i++)
    {
      nerrors += check_invalid(invalid[i]);
    }

  printf("inetpton: %zu cases, %d errors\n",
         nitems(valid) + nitems(invalid), nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
