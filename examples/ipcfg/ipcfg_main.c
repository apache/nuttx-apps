/****************************************************************************
 * apps/examples/ipcfg/ipcfg_main.c
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
#include <fcntl.h>
#include <debug.h>

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
#define PATH1         CONFIG_IPCFG_PATH "/ipcfg-" DEVICE1
#define PATH2         CONFIG_IPCFG_PATH "/ipcfg-" DEVICE2
#define IPv4PROTO_MAX IPv4PROTO_FALLBACK
#define IPv6PROTO_MAX IPv6PROTO_FALLBACK
#define IOBUFFERSIZE  1024

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static const char *g_ipv4proto_name[] =
{
  "none",      /* IPv4PROTO_NONE */
  "static",    /* IPv4PROTO_STATIC */
  "dhcp",      /* IPv4PROTO_DHCP */
  "fallback"   /* IPv4PROTO_FALLBACK */
};
#endif

#ifdef CONFIG_NET_IPv6
static const char *g_ipv6proto_name[] =
{
  "none",      /* IPv6PROTO_NONE */
  "static",    /* IPv6PROTO_STATIC */
  "autoconf",  /* IPv6PROTO_AUTOCONF */
  "fallback"   /* IPv6PROTO_FALLBACK */
};

static const uint16_t g_ipv6_ipaddr[8] =
{
  HTONS(0xfc00),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0002),
};

static const uint16_t g_ipv6_netmask[8] =
{
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0x0000),
  HTONS(0x0000),
};

static const uint16_t g_ipv6_router[8] =
{
  HTONS(0xfc00),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0000),
  HTONS(0x0001),
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipcfg_nibble
 *
 * Description:
 *  Convert a binary nibble to a hexadecimal character.
 *
 ****************************************************************************/

#ifdef CONFIG_IPCFG_BINARY
static char ipcfg_nibble(unsigned char nibble)
{
  if (nibble < 10)
    {
      return '0' + nibble;
    }
  else
    {
      return 'a' + nibble - 10;
    }
}
#endif

/****************************************************************************
 * Name: ipcfg_dump_file
 *
 * Description:
 *   Dump the contents of a file to stdout.
 *
 * Input Paratemets:
 *   filepath - The full path to the file to be dumped
 *
 * Returned Value:
 *   Zero (OK) on success; -1 (ERROR) on failure.
 *
 ****************************************************************************/

int ipcfg_dump_file(FAR const char *filepath)
{
  FAR char *buffer;
#ifdef CONFIG_IPCFG_BINARY
  int nbytes = 0;
#endif
  int fd;
  int ret = OK;

  /* Open the file for reading */

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      ret = -errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", filepath, ret);
      return ret;
    }

  buffer = (FAR char *)malloc(IOBUFFERSIZE);
  if (buffer == NULL)
    {
      ret = -ENOMEM;
      fprintf(stderr, "ERROR: Failed to allocate buffer\n");
      goto errout_with_fd;
    }

  /* And just dump it byte for byte into stdout */

  for (; ; )
    {
      ssize_t nbytesread = read(fd, buffer, IOBUFFERSIZE);

      /* Check for read errors */

      if (nbytesread < 0)
        {
          ret = -errno;
          fprintf(stderr, "ERROR: Read failed: %d\n", ret);
          goto errout_with_buffer;
        }

      /* Check for data successfully read */

      else if (nbytesread > 0)
        {
#ifndef CONFIG_IPCFG_BINARY
          int nbyteswritten = 0;

          while (nbyteswritten < nbytesread)
            {
              ssize_t n = write(1, buffer + nbyteswritten,
                                nbytesread - nbyteswritten);
              if (n < 0)
                {
                  ret = -errno;
                  fprintf(stderr, "ERROR: Write failed: %d\n", ret);
                  goto errout_with_buffer;
                }
              else
                {
                  nbyteswritten += n;
                }
            }
#else
          int i;

          for (i = 0; i < nbytesread; i++)
            {
              uint8_t byte = buffer[i];

              if (nbytes == 0)
                {
                  printf("  ");
                }

              putchar(ipcfg_nibble(byte >> 4));
              putchar(ipcfg_nibble(byte & 0x0f));

              if (++nbytes == 16)
                {
                  putchar(' ');
                }

              if (nbytes == 32)
                {
                  putchar('\n');
                  nbytes = 0;
                }
            }
#endif
        }

      /* Otherwise, it is the end of file */

      else
        {
#ifdef CONFIG_IPCFG_BINARY
          putchar('\n');
#endif
          break;
        }
    }

  /* Close the input file and return the result */

errout_with_buffer:
  free(buffer);

errout_with_fd:
  close(fd);
  return ret;
}

/****************************************************************************
 * Name: ipcfg_dump_ipv4addr
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static void ipcfg_dump_ipv4addr(FAR const char *variable, in_addr_t address)
{
  if (address != 0)
    {
      struct in_addr saddr =
      {
        address
      };

      char inetaddr[INET_ADDRSTRLEN];

      printf("%s%s\n", variable,
             inet_ntoa_r(saddr, inetaddr, sizeof(inetaddr)));
    }
}
#endif

/****************************************************************************
 * Name: ipcfg_check_ipv6addr
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static int ipcfg_check_ipv6addr(FAR const struct in6_addr *address)
{
  int i;

  for (i = 0; i < 4; i++)
    {
      if (address->s6_addr32[i] != 0)
        {
          return OK;
        }
    }

  return -ENXIO;
}
#endif

/****************************************************************************
 * Name: ipcfg_dump_ipv6addr
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static void ipcfg_dump_ipv6addr(FAR const char *variable,
                                FAR const struct in6_addr *address)
{
  int ret;

  ret = ipcfg_check_ipv6addr(address);
  if (ret == OK)
    {
      char converted[INET6_ADDRSTRLEN];

      /* Convert the address to ASCII text */

      if (inet_ntop(AF_INET6, address, converted, INET6_ADDRSTRLEN) == NULL)
        {
          ret = -errno;
          fprintf(stderr, "ERROR: inet_ntop() failed: %d\n", ret);
        }
      else
        {
          printf("%s%s\n", variable, converted);
        }
    }
}
#endif

/****************************************************************************
 * Name: ipcfg_dump_ipv4config
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static int ipcfg_dump_ipv4config(FAR const char *netdev)
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

  if (ipv4cfg.proto > IPv4PROTO_MAX)
    {
      printf("\n  IPv4BOOTPROTO:  %d [INVALID]\n", ipv4cfg.proto);
      return -EINVAL;
    }
  else
    {
      printf("\n  IPv4BOOTPROTO:  %s\n", g_ipv4proto_name[ipv4cfg.proto]);
    }

  if (ipv4cfg.proto == IPv4PROTO_STATIC ||
      ipv4cfg.proto == IPv4PROTO_FALLBACK)
    {
      ipcfg_dump_ipv4addr("  IPv4IPADDR:     ",  ipv4cfg.ipaddr);
      ipcfg_dump_ipv4addr("  IPv4NETMASK:    ",  ipv4cfg.netmask);
      ipcfg_dump_ipv4addr("  IPv4ROUTER:     ",  ipv4cfg.router);
      ipcfg_dump_ipv4addr("  IPv4DNS:        ",  ipv4cfg.dnsaddr);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ipcfg_dump_ipv6config
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static int ipcfg_dump_ipv6config(FAR const char *netdev)
{
  struct ipv6cfg_s ipv6cfg;
  int ret;

  ret = ipcfg_read(netdev, (FAR struct ipcfg_s *)&ipv6cfg, AF_INET6);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipcfg_read() failed: %d\n", ret);
      return ret;
    }

  /* Dump the content in human readable form */

  if (ipv6cfg.proto > IPv6PROTO_MAX)
    {
      printf("\n  IPv6BOOTPROTO:  %d [INVALID]\n", ipv6cfg.proto);
      return -EINVAL;
    }
  else
    {
      printf("\n  IPv6BOOTPROTO:  %s\n", g_ipv6proto_name[ipv6cfg.proto]);
    }

  if (ipv6cfg.proto == IPv6PROTO_STATIC ||
      ipv6cfg.proto == IPv6PROTO_FALLBACK)
    {
      ipcfg_dump_ipv6addr("  IPv6IPADDR:     ", &ipv6cfg.ipaddr);
      ipcfg_dump_ipv6addr("  IPv6NETMASK:    ", &ipv6cfg.netmask);
      ipcfg_dump_ipv6addr("  IPv6ROUTER:     ", &ipv6cfg.router);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ipcfg_dump_config
 ****************************************************************************/

static int ipcfg_dump_config(FAR const char *netdev)
{
  int ret;

  printf("\n%s:\n", netdev);

#ifdef CONFIG_NET_IPv4
  ret = ipcfg_dump_ipv4config(netdev);
  if (ret < 0)
    {
      return ret;
    }
#endif

#ifdef CONFIG_NET_IPv6
  ret = ipcfg_dump_ipv6config(netdev);
  if (ret < 0)
    {
      return ret;
    }
#endif

  return OK;
}

/****************************************************************************
 * Name: ipcfg_write_ipv4
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
static int ipcfg_write_ipv4(FAR const char *netdev)
{
  struct ipv4cfg_s ipv4cfg;
  int ret;

  memset(&ipv4cfg, 0, sizeof(struct ipv4cfg_s));

  ipv4cfg.proto   = IPv4PROTO_FALLBACK;
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
#endif

/****************************************************************************
 * Name: ipcfg_write_ipv6
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
static int ipcfg_write_ipv6(FAR const char *netdev)
{
  struct ipv6cfg_s ipv6cfg;
  int ret;

  memset(&ipv6cfg, 0, sizeof(struct ipv6cfg_s));

  ipv6cfg.proto = IPv6PROTO_FALLBACK;

  memcpy(ipv6cfg.ipaddr.s6_addr16, g_ipv6_ipaddr,
         sizeof(struct in6_addr));
  memcpy(ipv6cfg.netmask.s6_addr16, g_ipv6_netmask,
         sizeof(struct in6_addr));
  memcpy(ipv6cfg.router.s6_addr16, g_ipv6_router,
         sizeof(struct in6_addr));

  ret = ipcfg_write(netdev, (FAR struct ipcfg_s *)&ipv6cfg, AF_INET6);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipcfg_write() ipcfg_write: %d\n", ret);
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: ipcfg_write_config
 ****************************************************************************/

static int ipcfg_write_config(FAR const char *netdev, FAR const char *path)
{
  int ret;

  /* Write IPv6 first which might cause re-organization of file */

#ifdef CONFIG_NET_IPv6
  ret = ipcfg_write_ipv6(netdev);
  if (ret < 0)
    {
      return ret;
    }

  printf("\n  %s after IPv6 Update:\n", path);
  ipcfg_dump_file(path);
#endif

#ifdef CONFIG_NET_IPv4
  ret = ipcfg_write_ipv4(netdev);
  if (ret < 0)
    {
      return ret;
    }

  printf("\n  %s after IPv4 Update:\n", path);
  ipcfg_dump_file(path);
#endif

  return OK;
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
      fprintf(stderr, "ERROR: Failed to mount " CONFIG_IPCFG_PATH "\n: %d",
              errcode);
      return EXIT_FAILURE;
    }

  /* Dump files.  These should all fail. */

  printf("\n1. Dump before creating configuration files\n");
  ipcfg_dump_config(DEVICE1);
  ipcfg_dump_config(DEVICE2);

#ifdef CONFIG_IPCFG_WRITABLE
  /* Create files */

  printf("\n2. Create configuration files\n\n");
  ipcfg_write_config(DEVICE1, PATH1);
  ipcfg_write_config(DEVICE2, PATH2);

  /* Dump the files again */

  printf("\n3. Dump after creating configuration files\n");
  ipcfg_dump_config(DEVICE1);
  ipcfg_dump_config(DEVICE2);
#endif

  umount(CONFIG_IPCFG_PATH);
  return EXIT_SUCCESS;
}
