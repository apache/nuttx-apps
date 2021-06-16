/****************************************************************************
 * apps/examples/wget/wget_main.c
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

#include <stdint.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "netutils/netlib.h"
#include "netutils/webclient.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Configuration Checks *****************************************************/

/* BEWARE:
 * There are other configuration settings needed in netutitls/wget/wgetc.s,
 * but there are default values for those so we cannot check them here.
 */

#ifndef CONFIG_EXAMPLES_WGET_IPADDR
#  error "You must define CONFIG_EXAMPLES_WGET_IPADDR"
#endif

#ifndef CONFIG_EXAMPLES_WGET_DRIPADDR
#  error "You must define CONFIG_EXAMPLES_WGET_DRIPADDR"
#endif

#ifndef CONFIG_EXAMPLES_WGET_NETMASK
#  error "You must define CONFIG_EXAMPLES_WGET_NETMASK"
#endif

#ifndef CONFIG_NET
#  error "You must define CONFIG_NET"
#endif

#ifndef CONFIG_NET_TCP
#  error "You must define CONFIG_NET_TCP"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_iobuffer[512];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: callback
 ****************************************************************************/

static int callback(FAR char **buffer, int offset, int datend,
                     FAR int *buflen, FAR void *arg)
{
  ssize_t written = write(1, &((*buffer)[offset]), datend - offset);
  if (written == -1)
    {
      return -errno;
    }

  /* Revisit: Do we want to check and report short writes? */

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wget_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifndef CONFIG_NSH_NETINIT
  struct in_addr addr;
#if defined(CONFIG_EXAMPLES_WGET_NOMAC)
  uint8_t mac[IFHWADDRLEN];
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_WGET_NOMAC
  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr("eth0", mac);
#endif

  /* Set up our host address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_WGET_IPADDR);
  netlib_set_ipv4addr("eth0", &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_WGET_DRIPADDR);
  netlib_set_dripv4addr("eth0", &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_WGET_NETMASK);
  netlib_set_ipv4netmask("eth0", &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");
#endif /* CONFIG_NSH_NETINIT */

  /* Then start the server */

  struct webclient_context ctx;
  webclient_set_defaults(&ctx);
  ctx.method = "GET";
  ctx.buffer = g_iobuffer;
  ctx.buflen = 512;
  ctx.sink_callback = callback;
  ctx.sink_callback_arg = NULL;
  if (argc > 1)
    {
      ctx.url = argv[1];
    }
  else
    {
      ctx.url = CONFIG_EXAMPLES_WGET_URL;
    }

  int ret = webclient_perform(&ctx);
  if (ret != 0)
    {
      printf("webclient_perform failed with %d\n", ret);
    }

  return 0;
}
