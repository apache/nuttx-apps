/****************************************************************************
 * apps/examples/webpanel/cgi_renew.c
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

#include <stdio.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcprenew_main
 *
 * Description:
 *   CGI entry point that renews the IPv4 address on the configured
 *   interface and returns the result as JSON.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Argument vector.
 *
 * Returned Value:
 *   Zero on success; one on failure.
 *
 ****************************************************************************/

int dhcprenew_main(int argc, FAR char *argv[])
{
  int ret;

  ret = netlib_obtain_ipv4addr(CONFIG_EXAMPLES_WEBPANEL_NETIF);

  puts("Content-type: application/json\r\n"
       "\r\n");

  if (ret >= 0)
    {
      puts("{\"status\":\"ok\"}\n");
    }
  else
    {
      printf("{\"status\":\"error\",\"code\":%d}\n", ret);
    }

  return ret < 0 ? 1 : 0;
}
