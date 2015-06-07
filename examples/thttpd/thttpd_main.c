/****************************************************************************
 * examples/thttpd/thttpd_main.c
 *
 *   Copyright (C) 2009-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
  * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include <nuttx/net/arp.h>
#include <apps/netutils/netlib.h>
#include <apps/netutils/thttpd.h>

#include <nuttx/fs/ramdisk.h>
#include <nuttx/binfmt/binfmt.h>

#ifdef CONFIG_THTTPD_NXFLAT
#  include <nuttx/binfmt/nxflat.h>
#endif

#ifdef CONFIG_THTTPD_BINFS
#  include <nuttx/fs/unionfs.h>
#  include <nuttx/binfmt/builtin.h>
#endif

#ifdef CONFIG_NET_SLIP
#  include <nuttx/net/net.h>
#endif

#include "content/romfs.h"

#ifdef CONFIG_THTTPD_NXFLAT
#  include "content/symtab.h"
#endif

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Check configuration.  This is not all of the configuration settings that
 * are required -- only the more obvious.
 */

#if CONFIG_NFILE_DESCRIPTORS < 1
#  error "You must provide file descriptors via CONFIG_NFILE_DESCRIPTORS in your configuration file"
#endif

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
#  define ROMFS_PREFIX       NULL
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
int                         g_thttpdnsymbols;
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * thttp_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int thttp_main(int argc, char *argv[])
#endif
{
  struct in_addr addr;
#ifdef CONFIG_EXAMPLES_THTTPD_NOMAC
  uint8_t mac[IFHWADDRLEN];
#endif
  char *thttpd_argv = "thttpd";
  int ret;

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

#ifdef CONFIG_THTTPD_NXFLAT
  /* Initialize the NXFLAT binary loader */

  printf("Initializing the NXFLAT binary loader\n");
  ret = nxflat_initialize();
  if (ret < 0)
    {
      printf("ERROR: Initialization of the NXFLAT loader failed: %d\n", ret);
      exit(2);
    }
#endif

  /* Create a ROM disk for the ROMFS filesystem */

  printf("Registering romdisk\n");

  ret = romdisk_register(0, (uint8_t*)romfs_img, NSECTORS(romfs_img_len), SECTORSIZE);
  if (ret < 0)
    {
      printf("ERROR: romdisk_register failed: %d\n", ret);
#ifdef CONFIG_THTTPD_NXFLAT
      nxflat_uninitialize();
#endif
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
#ifdef CONFIG_THTTPD_NXFLAT
      nxflat_uninitialize();
#endif
    }

#ifdef CONFIG_THTTPD_BINFS
  /* Initialize the BINFS binary loader */

  printf("Initializing the Built-In binary loader\n");

  ret = builtin_initialize();
  if (ret < 0)
    {
      printf("ERROR: Initialization of the Built-In loader failed: %d\n", ret);
      exit(2);
    }

  /* Mount the BINFS file system */

  printf("Mounting BINFS filesystem at %s\n", BINFS_MOUNTPT);

  ret = mount(NULL, BINFS_MOUNTPT, "binfs", MS_RDONLY, NULL);
  if (ret < 0)
    {
      printf("ERROR: mount(NULL,%s,binfs) failed: %d\n", BINFS_MOUNTPT, errno);
    }

  /* Now create and mount the union file system */

  printf("Creating UNIONFS filesystem at %s\n", UNIONFS_MOUNTPT);

  ret = unionfs_mount(ROMFS_MOUNTPT, ROMFS_PREFIX, BINFS_MOUNTPT, BINFS_PREFIX,
                      UNIONFS_MOUNTPT);
  if (ret < 0)
    {
      printf("ERROR: Failed to create the union file system at %s: %d\n", UNIONFS_MOUNTPT, ret);
    }
#endif

  /* Start THTTPD.  At present, symbol table info is passed via global variables */

#ifdef CONFIG_THTTPD_NXFLAT
  g_thttpdsymtab   = exports;
  g_thttpdnsymbols = NEXPORTS;
#endif

  printf("Starting THTTPD\n");
  fflush(stdout);
  thttpd_main(1, &thttpd_argv);
  printf("THTTPD terminated\n");
  fflush(stdout);
  return 0;
}
