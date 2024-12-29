/****************************************************************************
 * apps/system/dhcp6c/renew6_main.c
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
#include <debug.h>

#include <string.h>
#include <errno.h>
#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char * const argv[])
{
  int ret = -EINVAL;

  if (argc < 2)
    {
      nerr("Input parameters are invalid!\n");
      return ret;
    }

  ret = netlib_obtain_ipv6addr(argv[1]);
  if (ret)
    {
      nerr("obtain ipv6addr fail\n");
    }

  return ret;
}
