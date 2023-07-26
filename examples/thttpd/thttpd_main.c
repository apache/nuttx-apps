/****************************************************************************
 * apps/examples/thttpd/thttpd_main.c
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

#include <sys/ioctl.h>
#include <sys/mount.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include "netutils/netlib.h"
#include "netutils/thttpd.h"

#include <nuttx/drivers/ramdisk.h>

#ifdef CONFIG_THTTPD_NXFLAT
#  include <sys/boardctl.h>
#endif

#ifdef CONFIG_NET_SLIP
#  include <nuttx/net/net.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#ifdef CONFIG_BINFMT_DISABLE
#  error "You must not disable loadable modules via CONFIG_BINFMT_DISABLE in your configuration file"
#endif

#ifdef CONFIG_THTTPD_NXFLAT
#  ifndef CONFIG_NXFLAT
#    error "You must select CONFIG_NXFLAT in your configuration file"
#  endif

#  ifndef CONFIG_FS_ROMFS
#    error "You must select CONFIG_FS_ROMFS in your configuration file"
#  endif

#  ifdef CONFIG_DISABLE_MOUNTPOINT
#    error "You must not disable mountpoints via CONFIG_DISABLE_MOUNTPOINT in your configuration file"
#  endif

#ifndef CONFIG_BOARDCTL_ROMDISK
#  error "CONFIG_BOARDCTL_ROMDISK should be enabled in the configuration file"
#endif
#endif

#ifdef CONFIG_THTTPD_BINFS
#  ifndef CONFIG_BUILTIN
#    error "You must select CONFIG_BUILTIN=y in your configuration file"
#  endif

#  ifndef CONFIG_FS_BINFS
#    error "You must select CONFIG_FS_BINFS=y in your configuration file"
#  endif

#  ifndef CONFIG_FS_UNIONFS
#    error "CONFIG_FS_UNIONFS=y is required in this configuration"
#  endif
#endif

/* Ethernet specific configuration */

#ifdef CONFIG_NET_ETHERNET
/* Use the standard Ethernet device name */

#  define NET_DEVNAME "eth0"

#else

/* No Ethernet -> No MAC address operations */

#  undef CONFIG_EXAMPLES_THTTPD_NOMAC
#endif

/* SLIP-specific configuration */

#ifdef CONFIG_NET_SLIP

/* TTY device to use */

#  ifndef CONFIG_NET_SLIPTTY
#    define CONFIG_NET_SLIPTTY "/dev/ttyS1"
#  endif

#  define SLIP_DEVNO 0

#  ifndef NET_DEVNAME
#    define NET_DEVNAME "sl0"
#  endif
#endif

/* Describe the ROMFS file system */

#define SECTORSIZE   64
#define NSECTORS(b)  (((b)+SECTORSIZE-1)/SECTORSIZE)
#define ROMFSDEV     "/dev/ram0"

#ifdef CONFIG_THTTPD_BINFS
#  define ROMFS_MOUNTPT      "/mnt/tmp1"
#  define ROMFS_PREFIX       ""
#  define BINFS_MOUNTPT      "/mnt/tmp2"
#  define BINFS_PREFIX       "cgi-bin"
#  define UNIONFS_MOUNTPT    CONFIG_THTTPD_PATH
#else
#  define ROMFS_MOUNTPT      CONFIG_THTTPD_PATH
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef CONFIG_THTTPD_NXFLAT
/* These values must be provided by the user before the THTTPD task daemon
 * is started:
 *
 * g_thttpdsymtab:  A symbol table describing all of the symbols exported
 *   from the base system.  These symbols are used to bind address references
 *   in CGI programs to NuttX.
 * g_nsymbols:  The number of symbols in g_thttpdsymtab[].
 */

FAR const struct symtab_s *g_thttpdsymtab;
int                        g_thttpdnsymbols;
#endif

/****************************************************************************
 * Symbols from Auto-Generated Code
 ****************************************************************************/

extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;

#ifdef CONFIG_THTTPD_NXFLAT
extern const struct symtab_s g_thttpd_exports[];
extern const int g_thttpd_nexports;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * thttp_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct in_addr addr;
#ifdef CONFIG_EXAMPLES_THTTPD_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif
  char *thttpd_argv = "thttpd";
  int ret;
#ifdef CONFIG_THTTPD_NXFLAT
  struct boardioc_romdisk_s desc;
#endif

  /* Configure SLIP */

#ifdef CONFIG_NET_SLIP
  ret = slip_initialize(SLIP_DEVNO, CONFIG_NET_SLIPTTY);
  if (ret < 0)
    {
      printf("ERROR: SLIP initialization failed: %d\n", ret);
      exit(1);
    }
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#ifdef CONFIG_EXAMPLES_THTTPD_NOMAC
  printf("Assigning MAC\n");

  mac[0] = 0x00;
  mac[1] = 0xe0;
  mac[2] = 0xde;
  mac[3] = 0xad;
  mac[4] = 0xbe;
  mac[5] = 0xef;
  netlib_setmacaddr(NET_DEVNAME, mac);
#endif

  /* Set up our host address */

  printf("Setup network addresses\n");
  addr.s_addr = HTONL(CONFIG_THTTPD_IPADDR);
  netlib_set_ipv4addr(NET_DEVNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_THTTPD_DRIPADDR);
  netlib_set_dripv4addr(NET_DEVNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_EXAMPLES_THTTPD_NETMASK);
  netlib_set_ipv4netmask(NET_DEVNAME, &addr);

  /* New versions of netlib_set_ipvXaddr will not bring the network up,
   * So ensure the network is really up at this point.
   */

  netlib_ifup("eth0");

#ifdef CONFIG_THTTPD_NXFLAT
  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk\n");

  desc.minor    = 0;                                    /* Minor device number of the ROM disk. */
  desc.nsectors = NSECTORS(romfs_img_len);              /* The number of sectors in the ROM disk */
  desc.sectsize = SECTORSIZE;                           /* The size of one sector in bytes */
  desc.image    = (FAR uint8_t *)romfs_img;             /* File system image */

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);

  if (ret < 0)
    {
      printf("ERROR: romdisk_register failed: %d\n", ret);
      exit(1);
    }

  /* Mount the ROMFS file system */

  printf("Mounting ROMFS filesystem at target=%s with source=%s\n",
         ROMFS_MOUNTPT, ROMFSDEV);

  ret = mount(ROMFSDEV, ROMFS_MOUNTPT, "romfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: mount(%s,%s,romfs) failed: %d\n",
             ROMFSDEV, ROMFS_MOUNTPT, errno);
    }
#endif

#ifdef CONFIG_THTTPD_BINFS
  /* Mount the BINFS file system */

  printf("Mounting BINFS filesystem at %s\n", BINFS_MOUNTPT);

  ret = mount(NULL, BINFS_MOUNTPT, "binfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: mount(NULL,%s,binfs) failed: %d\n",
             BINFS_MOUNTPT, errno);
    }

  /* Now create and mount the union file system */

  printf("Creating UNIONFS filesystem at %s\n", UNIONFS_MOUNTPT);

  ret = mount(NULL, UNIONFS_MOUNTPT, "unionfs", 0,
              "fspath1=" ROMFS_MOUNTPT ",prefix1=" ROMFS_PREFIX
              ",fspath2=" BINFS_MOUNTPT ",prefix2=" BINFS_PREFIX);
  if (ret < 0)
    {
      printf("ERROR: Failed to create the union file system at %s: %d\n",
             UNIONFS_MOUNTPT, ret);
    }
#endif

  /* Start THTTPD.  At present, symbol table info is passed via
   * global variables.
   */

#ifdef CONFIG_THTTPD_NXFLAT
  g_thttpdsymtab   = g_thttpd_exports;
  g_thttpdnsymbols = g_thttpd_nexports;
#endif

  printf("Starting THTTPD\n");
  fflush(stdout);
  thttpd_main(1, &thttpd_argv);
  printf("THTTPD terminated\n");
  fflush(stdout);
  return 0;
}
