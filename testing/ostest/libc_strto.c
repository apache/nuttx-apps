/****************************************************************************
 * apps/testing/ostest/libc_strto.c
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

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#include "ostest.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int strto_test(void)
{
  unsigned long value;
  char input[3];
  char *endptr;
  int base;

  /* Verify the largest lower- and upper-case digit and a positional value
   * in every supported explicit base.
   */

  input[1] = '!';
  input[2] = '\0';
  for (base = 2; base <= 36; base++)
    {
      input[0] = base <= 10 ? '0' + base - 1 : 'a' + base - 11;
      errno = 0;
      value = strtoul(input, &endptr, base);
      ASSERT(value == (unsigned long)base - 1);
      ASSERT(endptr == input + 1);
      ASSERT(errno == 0);

      if (base > 10)
        {
          input[0] = 'A' + base - 11;
          errno = 0;
          value = strtoul(input, &endptr, base);
          ASSERT(value == (unsigned long)base - 1);
          ASSERT(endptr == input + 1);
          ASSERT(errno == 0);
        }

      errno = 0;
      value = strtoul("10!", &endptr, base);
      ASSERT(value == (unsigned long)base);
      ASSERT(*endptr == '!');
      ASSERT(errno == 0);
    }

  /* All public integer conversion interfaces share this base validation. */

  errno = 0;
  ASSERT(strtol("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  errno = 0;
  ASSERT(strtoul("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  errno = 0;
  ASSERT(strtoll("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  errno = 0;
  ASSERT(strtoull("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  errno = 0;
  ASSERT(strtoimax("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  errno = 0;
  ASSERT(strtoumax("z!", &endptr, 36) == 35);
  ASSERT(*endptr == '!');
  ASSERT(errno == 0);

  /* Bases outside the supported range remain invalid. */

  errno = 0;
  ASSERT(strtoul("10", &endptr, 1) == 0);
  ASSERT(errno == EINVAL);

  errno = 0;
  ASSERT(strtoul("10", &endptr, 37) == 0);
  ASSERT(errno == EINVAL);

  return OK;
}
