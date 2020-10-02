/****************************************************************************
 * examples/ipcfg/ipcfg_main.c
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

#include <sys/mount.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "fsutils/ipcfg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_IPCFG_CHARDEV
#  error This example will not work with a character device
#endif

#ifndef CONFIG_IPCFG_WRITABLE
#  warning This example will not work with a without write support
#endif

#define DEVICE1       "eth0"
#define DEVICE2       "eth1"
#define IPv4PROTO_MAX IPv4PROTO_FALLBACK

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *g_ipv4proto_name[] =
{
  "none",      /* IPv4PROTO_NONE */
  "static",    /* IPv4PROTO_STATIC */
  "dhcp",      /* IPv4PROTO_DHCP */
  "fallback"   /* IPv4PROTO_FALLBACK */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_dump_addr
 ****************************************************************************/

static void ipcfg_dump_addr(FAR const char *variable, in_addr_t address)
{
  if (address == 0)
    {
      printf("%s[UNSPECIFIED]\n", variable);
    }
  else
    {
      struct in_addr saddr =
      {
        address
      };

      printf("%s%s\n", variable, inet_ntoa(saddr));
    }
}

/****************************************************************************
 * Name: ipcfg_dump_config
 ****************************************************************************/

static int ipcfg_dump_config(FAR const char *netdev)
{
  struct ipv4cfg_s ipv4cfg;
  int ret;

  ret = ipcfg_read(netdev, (FAR struct ipcfg_s *)&ipv4cfg, AF_INET);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipcfg_read() failed: %d\n", ret);
      return ret;
    }

  /* Dump the content in human readable form */

  printf("%s:\n", netdev);

  if (ipv4cfg.proto > IPv4PROTO_MAX)
    {
      printf("BOOTPROTO: %d [INVALID]\n", ipv4cfg.proto);
    }
  else
    {
      printf("BOOTPROTO: %s\n", g_ipv4proto_name[ipv4cfg.proto]);
    }

  ipcfg_dump_addr("IPADDR:     ",  ipv4cfg.ipaddr);
  ipcfg_dump_addr("NETMASK:    ",  ipv4cfg.netmask);
  ipcfg_dump_addr("ROUTER:     ",  ipv4cfg.router);
  ipcfg_dump_addr("DNS:        ",  ipv4cfg.dnsaddr);

  return OK;
}

/****************************************************************************
 * Name: ipcfg_write_config
 ****************************************************************************/

static int ipcfg_write_config(FAR const char *netdev)
{
  struct ipv4cfg_s ipv4cfg;
  int ret;

  ipv4cfg.proto   = IPv4PROTO_DHCP;
  ipv4cfg.ipaddr  = HTONL(0x0a000002);
  ipv4cfg.netmask = HTONL(0xffffff00);
  ipv4cfg.router  = HTONL(0x0a000001);
  ipv4cfg.dnsaddr = HTONL(0x0a000003);

  ret = ipcfg_write(netdev, (FAR struct ipcfg_s *)&ipv4cfg, AF_INET);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipcfg_write() ipcfg_write: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  /* Mount the TMPFS file system at the expected location for the IPv4
   * Configuration file directory.
   */

  ret = mount(NULL, CONFIG_IPCFG_PATH, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open " CONFIG_IPCFG_PATH "\n: %d",
              errcode);
      return EXIT_FAILURE;
    }

  /* Dump files.  These should all fail. */

  printf("\n1. Dump before creating configuration files\n\n");
  ipcfg_dump_config(DEVICE1);
  ipcfg_dump_config(DEVICE2);

#ifdef CONFIG_IPCFG_WRITABLE
  /* Create files */

  printf("\n2. Create configuration files\n\n");
  ipcfg_write_config(DEVICE1);
  ipcfg_write_config(DEVICE2);

  /* Dump the files again */

  printf("\n3. Dump after creating configuration files\n\n");
  ipcfg_dump_config(DEVICE1);
  ipcfg_dump_config(DEVICE2);
#endif

  umount(CONFIG_IPCFG_PATH);
  return EXIT_SUCCESS;
}
