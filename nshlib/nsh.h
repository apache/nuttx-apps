/****************************************************************************
 * apps/nshlib/nsh.h
 *
 *   Copyright (C) 2007-2018 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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

#ifndef __APPS_NSHLIB_NSH_H
#define __APPS_NSHLIB_NSH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#ifdef CONFIG_NSH_STRERROR
#  include <string.h>
#endif

#include <nuttx/usb/usbdev_trace.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* The background commands require pthread support */

#ifdef CONFIG_DISABLE_PTHREAD
#  ifndef CONFIG_NSH_DISABLEBG
#    define CONFIG_NSH_DISABLEBG 1
#  endif
#endif

#if CONFIG_NFILE_STREAMS == 0
#  undef CONFIG_NSH_TELNET
#  undef CONFIG_NSH_FILE_APPS
#  undef CONFIG_NSH_TELNET
#  undef CONFIG_NSH_CMDPARMS
#endif

/* rmdir, mkdir, rm, and mv are only available if mountpoints are enabled
 * AND there is a writeable file system OR if these operations on the
 * pseudo-filesystem are not disabled.
 */

#undef NSH_HAVE_WRITABLE_MOUNTPOINT
#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_WRITABLE) && \
    CONFIG_NFILE_STREAMS > 0
#  define NSH_HAVE_WRITABLE_MOUNTPOINT 1
#endif

#undef NSH_HAVE_PSEUDOFS_OPERATIONS
#if !defined(CONFIG_DISABLE_PSEUDOFS_OPERATIONS) && CONFIG_NFILE_STREAMS > 0
#  define NSH_HAVE_PSEUDOFS_OPERATIONS 1
#endif

#undef NSH_HAVE_DIROPTS
#if defined(NSH_HAVE_WRITABLE_MOUNTPOINT) || defined(NSH_HAVE_PSEUDOFS_OPERATIONS)
#  define NSH_HAVE_DIROPTS 1
#endif

/* If CONFIG_NSH_CMDPARMS is selected, then the path to a directory to
 * hold temporary files must be provided.
 */

#if defined(CONFIG_NSH_CMDPARMS) && !defined(CONFIG_LIBC_TMPDIR)
#  define CONFIG_LIBC_TMPDIR "/tmp"
#endif

/* Networking support.  Make sure that all non-boolean configuration
 * settings have some value.
 */

#ifndef CONFIG_NSH_IPADDR
#  define CONFIG_NSH_IPADDR    0x0a000002
#endif

#ifndef CONFIG_NSH_DRIPADDR
#  define CONFIG_NSH_DRIPADDR  0x0a000001
#endif

#ifndef CONFIG_NSH_NETMASK
#  define CONFIG_NSH_NETMASK   0xffffff00
#endif

#ifndef CONFIG_NSH_DNSIPADDR
#  define CONFIG_NSH_DNSIPADDR CONFIG_NSH_DRIPADDR
#endif

#ifndef CONFIG_NSH_MACADDR
#  define CONFIG_NSH_MACADDR   0x00e0deadbeef
#endif

#if !defined(CONFIG_NSH_NETINIT_THREAD) || !defined(CONFIG_ARCH_PHY_INTERRUPT) || \
    !defined(CONFIG_NETDEV_PHY_IOCTL) || !defined(CONFIG_NET_UDP) || \
     defined(CONFIG_DISABLE_SIGNALS)
#  undef CONFIG_NSH_NETINIT_MONITOR
#endif

#ifndef CONFIG_NSH_NETINIT_RETRYMSEC
#  define CONFIG_NSH_NETINIT_RETRYMSEC 2000
#endif

#ifndef CONFIG_NSH_NETINIT_SIGNO
#  define CONFIG_NSH_NETINIT_SIGNO 18
#endif

#ifndef CONFIG_NSH_NETINIT_THREAD_STACKSIZE
#  define CONFIG_NSH_NETINIT_THREAD_STACKSIZE 1568
#endif

#ifndef CONFIG_NSH_NETINIT_THREAD_PRIORITY
#  define CONFIG_NSH_NETINIT_THREAD_PRIORITY 100
#endif

/* Some networking commands do not make sense unless there is a network
 * device.  There might not be a network device if, for example, only Unix
 * domain sockets were enable.
 */

#if !defined(CONFIG_NET_ETHERNET)   && !defined(CONFIG_NET_6LOWPAN) && \
    !defined(CONFIG_NET_IEEE802154) && !defined(CONFIG_NET_LOOPBACK) && \
    !defined(CONFIG_NET_SLIP)       &&  !defined(CONFIG_NET_TUN)
  /* No link layer protocol is a good indication that there is no network
   * device.
   */

#  undef CONFIG_NSH_DISABLE_IFUPDOWN
#  undef CONFIG_NSH_DISABLE_IFCONFIG
#  define CONFIG_NSH_DISABLE_IFUPDOWN 1
#  define CONFIG_NSH_DISABLE_IFCONFIG 1
#endif

/* Telnetd requires networking support */

#ifndef CONFIG_NET
#  undef CONFIG_NSH_TELNET
#endif

/* get and put require TFTP client support */

#ifndef CONFIG_NETUTILS_TFTPC
#  undef  CONFIG_NSH_DISABLE_PUT
#  undef  CONFIG_NSH_DISABLE_GET
#  define CONFIG_NSH_DISABLE_PUT 1
#  define CONFIG_NSH_DISABLE_GET 1
#endif

/* Route depends on routing table support and procfs support */

#if !defined(CONFIG_FS_PROCFS) || defined(CONFIG_FS_PROCFS_EXCLUDE_NET) || \
    defined(CONFIG_FS_PROCFS_EXCLUDE_ROUTE) || !defined(CONFIG_NET_ROUTE) || \
    (!defined(CONFIG_NET_IPv4) && !defined(CONFIG_NET_IPv6))
#  ifndef CONFIG_NSH_DISABLE_ROUTE
#    define CONFIG_NSH_DISABLE_ROUTE 1
#  endif
#endif

/* wget depends on web client support */

#ifndef CONFIG_NETUTILS_WEBCLIENT
#  undef  CONFIG_NSH_DISABLE_WGET
#  define CONFIG_NSH_DISABLE_WGET 1
#endif

/* mksmartfs depends on smartfs and mksmartfs support */

#if !defined(CONFIG_FS_SMARTFS) || !defined(CONFIG_FSUTILS_MKSMARTFS)
#  undef  CONFIG_NSH_DISABLE_MKSMARTFS
#  define CONFIG_NSH_DISABLE_MKSMARTFS 1
#endif

/* One front end must be defined */

#if !defined(CONFIG_NSH_CONSOLE) && !defined(CONFIG_NSH_TELNET)
#  error "No NSH front end defined"
#endif

/* If a USB device is selected for the NSH console then we need to handle some
 * special start-up conditions.
 */

#undef HAVE_USB_CONSOLE
#if defined(CONFIG_USBDEV)

/* Check for a PL2303 serial console.  Use console device "/dev/console". */

#  if defined(CONFIG_PL2303) && defined(CONFIG_PL2303_CONSOLE)
#    define HAVE_USB_CONSOLE 1

/* Check for a CDC/ACM serial console.  Use console device "/dev/console". */

#  elif defined(CONFIG_CDCACM) && defined(CONFIG_CDCACM_CONSOLE)
#    define HAVE_USB_CONSOLE 1

/* Check for a generic USB console.  In this case, the USB console device
 * must be provided in CONFIG_NSH_USBCONDEV.
 */

#  elif defined(CONFIG_NSH_USBCONSOLE)
#    define HAVE_USB_CONSOLE 1
#  endif
#endif

/* Defaults for the USB console */

#ifdef HAVE_USB_CONSOLE

/* The default USB console device minor number is 0 */

#  ifndef CONFIG_NSH_USBDEV_MINOR
#    define CONFIG_NSH_USBDEV_MINOR 0
#  endif

/* The default USB serial console device */

#  ifndef CONFIG_NSH_USBCONDEV
#    if defined(CONFIG_CDCACM)
#      define CONFIG_NSH_USBCONDEV "/dev/ttyACM0"
#    elif defined(CONFIG_PL2303)
#      define CONFIG_NSH_USBCONDEV "/dev/ttyUSB0"
#    else
#      define CONFIG_NSH_USBCONDEV "/dev/console"
#    endif
#  endif

#endif /* HAVE_USB_CONSOLE */

/* If a USB keyboard device is selected for NSH input then we need to handle
 * some special start-up conditions.
 */

#undef HAVE_USB_KEYBOARD

/* Check pre-requisites */

#if !defined(CONFIG_USBHOST) || !defined(CONFIG_USBHOST_HIDKBD) || \
    defined(HAVE_USB_CONSOLE)
#  undef CONFIG_NSH_USBKBD
#endif

/* Check default settings */

#if defined(CONFIG_NSH_USBKBD)

/* Check for a USB HID keyboard in the configuration */

#  define HAVE_USB_KEYBOARD 1

/* The default keyboard device is /dev/kbda */

#  ifndef NSH_USBKBD_DEVNAME
#    define NSH_USBKBD_DEVNAME "/dev/kbda"
#  endif
#endif /* HAVE_USB_KEYBOARD */

/* USB trace settings */

#ifndef CONFIG_USBDEV_TRACE
#  undef CONFIG_NSH_USBDEV_TRACE
#endif

#ifdef CONFIG_NSH_USBDEV_TRACE
#  ifdef CONFIG_NSH_USBDEV_TRACEINIT
#    define TRACE_INIT_BITS         (TRACE_INIT_BIT)
#  else
#    define TRACE_INIT_BITS         (0)
#  endif

#  define TRACE_ERROR_BITS          (TRACE_DEVERROR_BIT|TRACE_CLSERROR_BIT)

#  ifdef CONFIG_NSH_USBDEV_TRACECLASS
#    define TRACE_CLASS_BITS        (TRACE_CLASS_BIT|TRACE_CLASSAPI_BIT|\
                                     TRACE_CLASSSTATE_BIT)
#  else
#    define TRACE_CLASS_BITS        (0)
#  endif

#  ifdef CONFIG_NSH_USBDEV_TRACETRANSFERS
#    define TRACE_TRANSFER_BITS     (TRACE_OUTREQQUEUED_BIT|TRACE_INREQQUEUED_BIT|\
                                     TRACE_READ_BIT|TRACE_WRITE_BIT|\
                                     TRACE_COMPLETE_BIT)
#  else
#    define TRACE_TRANSFER_BITS     (0)
#  endif

#  ifdef CONFIG_NSH_USBDEV_TRACECONTROLLER
#    define TRACE_CONTROLLER_BITS   (TRACE_EP_BIT|TRACE_DEV_BIT)
#  else
#    define TRACE_CONTROLLER_BITS   (0)
#  endif

#  ifdef CONFIG_NSH_USBDEV_TRACEINTERRUPTS
#    define TRACE_INTERRUPT_BITS    (TRACE_INTENTRY_BIT|TRACE_INTDECODE_BIT|\
                                     TRACE_INTEXIT_BIT)
#  else
#    define TRACE_INTERRUPT_BITS    (0)
#  endif

#  define TRACE_BITSET              (TRACE_INIT_BITS|TRACE_ERROR_BITS|\
                                     TRACE_CLASS_BITS|TRACE_TRANSFER_BITS|\
                                     TRACE_CONTROLLER_BITS|TRACE_INTERRUPT_BITS)

#endif /* CONFIG_NSH_USBDEV_TRACE */

/* If Telnet is selected for the NSH console, then we must configure
 * the resources used by the Telnet daemon and by the Telnet clients.
 *
 * CONFIG_NSH_TELNETD_PORT - The telnet daemon will listen on this.
 *   port.  Default: 23
 * CONFIG_NSH_TELNETD_DAEMONPRIO - Priority of the Telnet daemon.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_NSH_TELNETD_DAEMONSTACKSIZE - Stack size allocated for the
 *   Telnet daemon. Default: 2048
 * CONFIG_NSH_TELNETD_CLIENTPRIO - Priority of the Telnet client.
 *   Default: SCHED_PRIORITY_DEFAULT
 * CONFIG_NSH_TELNETD_CLIENTSTACKSIZE - Stack size allocated for the
 *   Telnet client. Default: 2048
 * CONFIG_NSH_TELNET_LOGIN - Support a simple Telnet login.
 *
 * If CONFIG_NSH_TELNET_LOGIN is defined, then these additional
 * options may be specified:
 *
 * CONFIG_NSH_LOGIN_USERNAME - Login user name.  Default: "admin"
 * CONFIG_NSH_LOGIN_PASSWORD - Login password:  Default: "Administrator"
 * CONFIG_NSH_LOGIN_FAILCOUNT - Number of login retry attempts.
 *   Default 3.
 */

#ifndef CONFIG_NSH_TELNETD_PORT
#  define CONFIG_NSH_TELNETD_PORT 23
#endif

#ifndef CONFIG_NSH_TELNETD_DAEMONPRIO
#  define CONFIG_NSH_TELNETD_DAEMONPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_NSH_TELNETD_DAEMONSTACKSIZE
#  define CONFIG_NSH_TELNETD_DAEMONSTACKSIZE 2048
#endif

#ifndef CONFIG_NSH_TELNETD_CLIENTPRIO
#  define CONFIG_NSH_TELNETD_CLIENTPRIO SCHED_PRIORITY_DEFAULT
#endif

#ifndef CONFIG_NSH_TELNETD_CLIENTSTACKSIZE
#  define CONFIG_NSH_TELNETD_CLIENTSTACKSIZE 2048
#endif

#ifdef CONFIG_NSH_TELNET_LOGIN

#  ifndef CONFIG_NSH_LOGIN_USERNAME
#    define CONFIG_NSH_LOGIN_USERNAME  "admin"
#  endif

#  ifndef CONFIG_NSH_LOGIN_PASSWORD
#    define CONFIG_NSH_LOGIN_PASSWORD  "nuttx"
#  endif

#  ifndef CONFIG_NSH_LOGIN_FAILCOUNT
#    define CONFIG_NSH_LOGIN_FAILCOUNT 3
#  endif

#endif /* CONFIG_NSH_TELNET_LOGIN */

/* Verify support for ROMFS /etc directory support options */

#ifdef CONFIG_NSH_ROMFSETC
#  ifdef CONFIG_DISABLE_MOUNTPOINT
#    error "Mountpoint support is disabled"
#    undef CONFIG_NSH_ROMFSETC
#  endif

#  if CONFIG_NFILE_DESCRIPTORS < 4
#    error "Not enough file descriptors"
#    undef CONFIG_NSH_ROMFSETC
#  endif

#  ifndef CONFIG_FS_ROMFS
#    error "ROMFS support not enabled"
#    undef CONFIG_NSH_ROMFSETC
#  endif

#  ifndef CONFIG_NSH_ROMFSMOUNTPT
#    define CONFIG_NSH_ROMFSMOUNTPT "/etc"
#  endif

#  ifndef CONFIG_NSH_INITSCRIPT
#    define CONFIG_NSH_INITSCRIPT "init.d/rcS"
#  endif

#  undef NSH_INITPATH
#  define NSH_INITPATH CONFIG_NSH_ROMFSMOUNTPT "/" CONFIG_NSH_INITSCRIPT

#  ifdef CONFIG_NSH_ROMFSRC
#    ifndef CONFIG_NSH_RCSCRIPT
#      define CONFIG_NSH_RCSCRIPT ".nshrc"
#    endif

#    undef NSH_RCPATH
#    define NSH_RCPATH CONFIG_NSH_ROMFSMOUNTPT "/" CONFIG_NSH_RCSCRIPT
#  endif

#  ifndef CONFIG_NSH_ROMFSDEVNO
#    define CONFIG_NSH_ROMFSDEVNO 0
#  endif

#  ifndef CONFIG_NSH_ROMFSSECTSIZE
#    define CONFIG_NSH_ROMFSSECTSIZE 64
#  endif

#  define NSECTORS(b)        (((b)+CONFIG_NSH_ROMFSSECTSIZE-1)/CONFIG_NSH_ROMFSSECTSIZE)
#  define STR_RAMDEVNO(m)    #m
#  define MKMOUNT_DEVNAME(m) "/dev/ram" STR_RAMDEVNO(m)
#  define MOUNT_DEVNAME      MKMOUNT_DEVNAME(CONFIG_NSH_ROMFSDEVNO)

#else

#  undef CONFIG_NSH_ROMFSRC
#  undef CONFIG_NSH_ROMFSMOUNTPT
#  undef CONFIG_NSH_INITSCRIPT
#  undef CONFIG_NSH_RCSCRIPT
#  undef CONFIG_NSH_ROMFSDEVNO
#  undef CONFIG_NSH_ROMFSSECTSIZE

#endif

/* This is the maximum number of arguments that will be accepted for a
 * command.  Here we attempt to select the smallest number possible depending
 * upon the of commands that are available.  Most commands use seven or fewer
 * arguments, but there are a few that require more.
 *
 * This value is also configurable with CONFIG_NSH_MAXARGUMENTS.  This
 * configurability is necessary since there may also be external, "built-in"
 * commands that require more commands than NSH is aware of.
 */

#if CONFIG_NSH_MAXARGUMENTS < 11
#  if defined(CONFIG_NET) && !defined(CONFIG_NSH_DISABLE_IFCONFIG)
#    undef CONFIG_NSH_MAXARGUMENTS
#    define CONFIG_NSH_MAXARGUMENTS 11
#  endif
#endif

/* Argument list size
 *
 *   argv[0]:      The command name.
 *   argv[1]:      The beginning of argument (up to CONFIG_NSH_MAXARGUMENTS)
 *   argv[argc-3]: Possibly '>' or '>>'
 *   argv[argc-2]: Possibly <file>
 *   argv[argc-1]: Possibly '&' (if pthreads are enabled)
 *   argv[argc]:   NULL terminating pointer
 *
 * Maximum size is CONFIG_NSH_MAXARGUMENTS+5
 */

#ifndef CONFIG_NSH_DISABLEBG
#  define MAX_ARGV_ENTRIES (CONFIG_NSH_MAXARGUMENTS+5)
#else
#  define MAX_ARGV_ENTRIES (CONFIG_NSH_MAXARGUMENTS+4)
#endif

/* strerror() produces much nicer output but is, however, quite large and
 * will only be used if CONFIG_NSH_STRERROR is defined.  Note that the strerror
 * interface must also have been enabled with CONFIG_LIBC_STRERROR.
 */

#ifndef CONFIG_LIBC_STRERROR
#  undef CONFIG_NSH_STRERROR
#endif

#ifdef CONFIG_NSH_STRERROR
#  define NSH_ERRNO         strerror(errno)
#  define NSH_ERRNO_OF(err) strerror(err)
#else
#  define NSH_ERRNO         (errno)
#  define NSH_ERRNO_OF(err) (err)
#endif

/* Maximum size of one command line (telnet or serial) */

#ifndef CONFIG_NSH_LINELEN
#  define CONFIG_NSH_LINELEN 80
#endif

/* The following two settings are used only in the telnetd interface */

#ifndef CONFIG_NSH_IOBUFFER_SIZE
# define CONFIG_NSH_IOBUFFER_SIZE 512
#endif

/* The maximum number of nested if-then[-else]-fi sequences that
 * are permissable.
 */

#ifndef CONFIG_NSH_NESTDEPTH
# define CONFIG_NSH_NESTDEPTH 3
#endif

/* Define to enable dumping of all input/output buffers */

#undef CONFIG_NSH_TELNETD_DUMPBUFFER

/* Make sure that the home directory is defined */

#ifndef CONFIG_LIB_HOMEDIR
# define CONFIG_LIB_HOMEDIR "/"
#endif

/* Stubs used when working directory is not supported */

#if CONFIG_NFILE_DESCRIPTORS <= 0 || defined(CONFIG_DISABLE_ENVIRON)
#  define nsh_getfullpath(v,p) ((FAR char*)(p))
#  define nsh_freefullpath(p)
#endif

/* The size of the I/O buffer may be specified in the
 * configs/<board-name>defconfig file -- provided that it is at least as
 * large as PATH_MAX.
 */

#define NSH_HAVE_IOBUFFER 1

#if CONFIG_NFILE_DESCRIPTORS <= 0
#  undef NSH_HAVE_IOBUFFER
#endif

/* The I/O buffer is needed for the ls, cp, and ps commands.  It is also
 * needed if the platform supplied MOTD is configured.
 */

#if defined(CONFIG_NSH_DISABLE_LS) && defined(CONFIG_NSH_DISABLE_CP) && \
    defined(CONFIG_NSH_DISABLE_PS) && !defined(CONFIG_NSH_PLATFORM_MOTD)
#  undef NSH_HAVE_IOBUFFER
#endif

#ifdef NSH_HAVE_IOBUFFER
#  ifdef CONFIG_NSH_FILEIOSIZE
#    if CONFIG_NSH_FILEIOSIZE > (PATH_MAX + 1)
#      define IOBUFFERSIZE CONFIG_NSH_FILEIOSIZE
#    else
#      define IOBUFFERSIZE (PATH_MAX + 1)
#    endif
#  else
#    define IOBUFFERSIZE 1024
#  endif
#else
#  define IOBUFFERSIZE (PATH_MAX + 1)
#endif

/* Certain commands are not available in a kernel builds because they depend
 * on interfaces that are not exported by the kernel.  These are actually
 * bugs that need to be fixed but for now the commands are simply disabled.
 * There are three classes of fixes required:
 *
 * - Some of these interfaces are inherently internal to the OS (such as
 *   register_ramdisk()) and should never be made available to user
 *   applications as OS interfaces.
 * - Other interfaces are more standard and for these there probably should
 *   be new system calls to support the OS interface.  Such interfaces
 *   include things like mkrd.
 * - Other interfaces simply need to be moved out of the OS and into the C
 *   library where they will become accessible to application code.
 */

#if defined(CONFIG_BUILD_PROTECTED) || defined(CONFIG_BUILD_LOADABLE)
#  undef  CONFIG_NSH_DISABLE_MKRD        /* 'mkrd' depends on ramdisk_register */
#  define CONFIG_NSH_DISABLE_MKRD 1
#endif

/* Certain commands/features are only available if the procfs file system is
 * enabled.
 */

#if !defined(CONFIG_FS_PROCFS) || defined(CONFIG_FS_PROCFS_EXCLUDE_NET)
#  undef  CONFIG_NSH_DISABLE_IFCONFIG    /* 'ifconfig' depends on network procfs */
#  define CONFIG_NSH_DISABLE_IFCONFIG 1

#  undef  CONFIG_NSH_DISABLE_IFUPDOWN    /* 'ifup/down' depend on network procfs */
#  define CONFIG_NSH_DISABLE_IFUPDOWN 1
#endif

#if !defined(CONFIG_FS_PROCFS) || defined(CONFIG_FS_PROCFS_EXCLUDE_PROCESS)
#  undef  CONFIG_NSH_DISABLE_PS          /* 'ps' depends on process procfs */
#  define CONFIG_NSH_DISABLE_PS 1
#endif

#define NSH_HAVE_CPULOAD  1
#if !defined(CONFIG_FS_PROCFS) || defined(CONFIG_FS_PROCFS_EXCLUDE_CPULOAD) || \
    !defined(CONFIG_SCHED_CPULOAD) || defined(CONFIG_NSH_DISABLE_PS)
#  undef NSH_HAVE_CPULOAD
#endif

#if !defined(CONFIG_FS_PROCFS) || (defined(CONFIG_FS_PROCFS_EXCLUDE_BLOCKS) && \
                                   defined(CONFIG_FS_PROCFS_EXCLUDE_USAGE))
#  undef  CONFIG_NSH_DISABLE_DF          /* 'df' depends on fs procfs */
#  define CONFIG_NSH_DISABLE_DF 1
#endif

#if defined(CONFIG_FS_PROCFS) || !defined(CONFIG_NSH_DISABLE_DF)
#  define HAVE_DF_HUMANREADBLE 1
#  define HAVE_DF_BLOCKOUTPUT  1
#  if defined(CONFIG_FS_PROCFS_EXCLUDE_USAGE)
#    undef HAVE_DF_HUMANREADBLE
#  endif
#  if defined(CONFIG_FS_PROCFS_EXCLUDE_BLOCKS)
#    undef HAVE_DF_BLOCKOUTPUT
#  endif
#endif

#undef HAVE_IRQINFO
#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_PROCFS) && \
     defined(CONFIG_SCHED_IRQMONITOR)
#  define HAVE_IRQINFO            1
#endif

#undef HAVE_MOUNT_LIST
#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_MOUNT) && \
   !defined(CONFIG_FS_PROCFS_EXCLUDE_MOUNT)
#  define HAVE_MOUNT_LIST         1
#endif

#if !defined(CONFIG_FS_PROCFS) || defined(CONFIG_FS_PROCFS_EXCLUDE_MEMINFO)
#  undef  CONFIG_NSH_DISABLE_FREE
#  define CONFIG_NSH_DISABLE_FREE 1
#endif

/* Suppress unused file utilities */

#define NSH_HAVE_CATFILE          1
#define NSH_HAVE_READFILE         1
#define NSH_HAVE_FOREACH_DIRENTRY 1
#define NSH_HAVE_TRIMDIR          1
#define NSH_HAVE_TRIMSPACES       1

#if CONFIG_NFILE_DESCRIPTORS <= 0
#  undef NSH_HAVE_CATFILE
#  undef NSH_HAVE_READFILE
#  undef NSH_HAVE_FOREACH_DIRENTRY
#  undef NSH_HAVE_TRIMDIR
#endif

/* nsh_catfile used by cat, ifconfig, ifup/down, df, free, irqinfo, and mount (with
 * no arguments).
 */

#if !defined(CONFIG_NSH_DISABLE_CAT) && !defined(CONFIG_NSH_DISABLE_IFCONFIG) && \
    !defined(CONFIG_NSH_DISABLE_IFUPDOWN) && !defined(CONFIG_NSH_DISABLE_DF) && \
    !defined(CONFIG_NSH_DISABLE_FREE) && !defined(HAVE_IRQINFO) && \
    !defined(HAVE_MOUNT_LIST)
#  undef NSH_HAVE_CATFILE
#endif

/* nsh_readfile used by ps command */

#if defined(CONFIG_NSH_DISABLE_PS)
#  undef NSH_HAVE_READFILE
#endif

/* nsh_foreach_direntry used by the ls and ps commands */

#if defined(CONFIG_NSH_DISABLE_LS) && defined(CONFIG_NSH_DISABLE_PS)
#  undef NSH_HAVE_FOREACH_DIRENTRY
#endif

#if defined(CONFIG_NSH_DISABLE_CP)
#  undef NSH_HAVE_TRIMDIR
#endif

/* nsh_trimspaces used by the set and ps commands */

#if defined(CONFIG_NSH_DISABLE_SET) && defined(CONFIG_NSH_DISABLE_PS)
#  undef NSH_HAVE_TRIMSPACES
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
#  define NSH_NP_SET_OPTIONS "ex"    /* Maintain order see nsh_npflags_e */
#  define NSH_NP_SET_OPTIONS_INIT    (NSH_PFLAG_SILENT)
#endif

#if defined(CONFIG_DISABLE_ENVIRON) && defined(CONFIG_NSH_DISABLESCRIPT)
#  undef  CONFIG_NSH_DISABLE_SET
#  define CONFIG_NSH_DISABLE_SET 1
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ITEF
/* State when parsing and if-then-else sequence */

enum nsh_itef_e
{
  NSH_ITEF_NORMAL = 0,         /* Not in an if-then-else sequence */
  NSH_ITEF_IF,                 /* Just parsed 'if', expect condition */
  NSH_ITEF_THEN,               /* Just parsed 'then', looking for 'else' or 'fi' */
  NSH_ITEF_ELSE                /* Just parsed 'else', look for 'fi' */
};

/* All state data for parsing one if-then-else sequence */

struct nsh_itef_s
{
  uint8_t   ie_ifcond   : 1;   /* Value of command in 'if' statement */
  uint8_t   ie_disabled : 1;   /* TRUE: Unconditionally disabled */
  uint8_t   ie_inverted : 1;   /* TRUE: inverted logic ('if ! <cmd>') */
  uint8_t   ie_unused   : 3;
  uint8_t   ie_state    : 2;   /* If-then-else state (see enum nsh_itef_e) */
};
#endif

#ifndef CONFIG_NSH_DISABLE_LOOPS
/* State when parsing and while-do-done or until-do-done sequence */

enum nsh_lp_e
{
  NSH_LOOP_NORMAL = 0,         /* Not in a while-do-done or until-do-done sequence */
  NSH_LOOP_WHILE,              /* Just parsed 'while', expect condition */
  NSH_LOOP_UNTIL,              /* Just parsed 'until', expect condition */
  NSH_LOOP_DO                  /* Just parsed 'do', looking for 'done' */
};

/* All state data for parsing one while-do-done or until-do-done sequence */

struct nsh_loop_s
{
  uint8_t   lp_enable   : 1;   /* Loop command processing is enabled */
  uint8_t   lp_unused   : 5;
  uint8_t   lp_state    : 2;   /* Loop state (see enume nsh_lp_e) */
#ifndef CONFIG_NSH_DISABLE_ITEF
  uint8_t   lp_iendx;          /* Saved if-then-else-fi index */
#endif
  long      lp_topoffs;        /* Top of loop file offset */
};
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
/* Define the bits that correspond to the option defined in
 * NSH_NP_SET_OPTIONS. The bit value is 1 shifted left the offset
 * of the char in NSH_NP_SET_OPTIONS string.
 */

enum nsh_npflags_e
{
  NSH_PFLAG_IGNORE = 1,      /*  set for +e no exit on errors,
                              *  cleared -e exit on error */
  NSH_PFLAG_SILENT = 2,      /*  cleared -x  print a trace of commands
                              *  when parsing.
                              *  set +x no print a trace of commands */
};
#endif

/* These structure provides the overall state of the parser */

struct nsh_parser_s
{
#ifndef CONFIG_NSH_DISABLEBG
  bool     np_bg;       /* true: The last command executed in background */
#endif
#if CONFIG_NFILE_STREAMS > 0
  bool     np_redirect; /* true: Output from the last command was re-directed */
#endif
  bool     np_fail;     /* true: The last command failed */
#ifndef CONFIG_NSH_DISABLESCRIPT
  uint8_t  np_flags;    /* See nsh_npflags_e above */
#endif
#ifndef CONFIG_NSH_DISABLEBG
  int      np_nice;     /* "nice" value applied to last background cmd */
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  FILE    *np_stream;   /* Stream of current script */
#ifndef CONFIG_NSH_DISABLE_LOOPS
  long     np_foffs;    /* File offset to the beginning of a line */
#ifndef NSH_DISABLE_SEMICOLON
  uint16_t np_loffs;    /* Byte offset to the beginning of a command */
  bool     np_jump;     /* "Jump" to the top of the loop */
#endif
  uint8_t  np_lpndx;    /* Current index into np_lpstate[] */
#endif
#ifndef CONFIG_NSH_DISABLE_ITEF
  uint8_t  np_iendx;    /* Current index into np_iestate[] */
#endif

  /* This is a stack of parser state information. */

#ifndef CONFIG_NSH_DISABLE_ITEF
  struct nsh_itef_s np_iestate[CONFIG_NSH_NESTDEPTH];
#endif
#ifndef CONFIG_NSH_DISABLE_LOOPS
  struct nsh_loop_s np_lpstate[CONFIG_NSH_NESTDEPTH];
#endif
#endif
};

/* This is the general form of a command handler */

struct nsh_vtbl_s; /* Defined in nsh_console.h */
typedef CODE int (*nsh_cmd_t)(FAR struct nsh_vtbl_s *vtbl, int argc,
                              FAR char **argv);

/* This is the form of a callback from nsh_foreach_direntry() */

struct dirent;     /* Defined in dirent.h */
typedef CODE int (*nsh_direntry_handler_t)(FAR struct nsh_vtbl_s *vtbl,
                                           FAR const char *dirpath,
                                           FAR struct dirent *entryp,
                                           FAR void *pvarg);

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char g_nshgreeting[];
#if defined(CONFIG_NSH_MOTD) && !defined(CONFIG_NSH_PLATFORM_MOTD)
extern const char g_nshmotd[];
#endif
#ifdef CONFIG_NSH_LOGIN
#if defined(CONFIG_NSH_TELNET_LOGIN) && defined(CONFIG_NSH_TELNET)
extern const char g_telnetgreeting[];
#endif
extern const char g_userprompt[];
extern const char g_passwordprompt[];
extern const char g_loginsuccess[];
extern const char g_badcredentials[];
extern const char g_loginfailure[];
#endif
extern const char g_nshprompt[];
extern const char g_fmtsyntax[];
extern const char g_fmtargrequired[];
extern const char g_fmtnomatching[];
extern const char g_fmtarginvalid[];
extern const char g_fmtargrange[];
extern const char g_fmtcmdnotfound[];
extern const char g_fmtnosuch[];
extern const char g_fmttoomanyargs[];
extern const char g_fmtdeepnesting[];
extern const char g_fmtcontext[];
extern const char g_fmtcmdfailed[];
extern const char g_fmtcmdoutofmemory[];
extern const char g_fmtinternalerror[];
#ifndef CONFIG_DISABLE_SIGNALS
extern const char g_fmtsignalrecvd[];
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initialization */

#ifdef CONFIG_NSH_ROMFSETC
int nsh_romfsetc(void);
#else
#  define nsh_romfsetc() (-ENOSYS)
#endif

#ifdef CONFIG_NSH_NETINIT
int nsh_netinit(void);
#else
#  define nsh_netinit() (-ENOSYS)
#endif

#ifdef HAVE_USB_CONSOLE
int nsh_usbconsole(void);
#else
#  define nsh_usbconsole() (-ENOSYS)
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && CONFIG_NFILE_STREAMS > 0 && !defined(CONFIG_NSH_DISABLESCRIPT)
int nsh_script(FAR struct nsh_vtbl_s *vtbl, const char *cmd, const char *path);
#ifdef CONFIG_NSH_ROMFSETC
int nsh_initscript(FAR struct nsh_vtbl_s *vtbl);
#ifdef CONFIG_NSH_ROMFSRC
int nsh_loginscript(FAR struct nsh_vtbl_s *vtbl);
#endif
#endif
#endif

/* Architecture-specific initialization */

#if defined(CONFIG_NSH_ARCHINIT) && !defined(CONFIG_LIB_BOARDCTL)
#  warning CONFIG_NSH_ARCHINIT is set, but CONFIG_LIB_BOARDCTL is not
#  undef CONFIG_NSH_ARCHINIT
#endif

/* Basic session and message handling */

struct console_stdio_s;
int nsh_session(FAR struct console_stdio_s *pstate);
int nsh_parse(FAR struct nsh_vtbl_s *vtbl, char *cmdline);

/****************************************************************************
 * Name: nsh_login
 *
 * Description:
 *   Prompt the user for a username and password.  Return a failure if valid
 *   credentials are not returned (after some retries.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_CONSOLE_LOGIN
#  if CONFIG_NFILE_DESCRIPTORS > 0
int nsh_login(FAR struct console_stdio_s *pstate);
#  else
int nsh_stdlogin(FAR struct console_stdio_s *pstate);
#  endif
#endif

#ifdef CONFIG_NSH_TELNET_LOGIN
int nsh_telnetlogin(FAR struct console_stdio_s *pstate);
#endif

/* Application interface */

int nsh_command(FAR struct nsh_vtbl_s *vtbl, int argc, char *argv[]);

#ifdef CONFIG_NSH_BUILTIN_APPS
int nsh_builtin(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR char **argv, FAR const char *redirfile, int oflags);
#endif

#ifdef CONFIG_NSH_FILE_APPS
int nsh_fileapp(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR char **argv, FAR const char *redirfile, int oflags);
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_DISABLE_ENVIRON)
/* Working directory support */

FAR const char *nsh_getcwd(void);
FAR char *nsh_getfullpath(FAR struct nsh_vtbl_s *vtbl,
                          FAR const char *relpath);
void nsh_freefullpath(FAR char *fullpath);
#endif

/* Debug */

void nsh_dumpbuffer(FAR struct nsh_vtbl_s *vtbl, const char *msg,
                    const uint8_t *buffer, ssize_t nbytes);

#ifdef CONFIG_WIRELESS_WAPI
/* Wireless */

int nsh_associate(FAR const char *ifname);
#endif

#ifdef CONFIG_NSH_USBDEV_TRACE
/* USB debug support */

void nsh_usbtrace(void);
#endif

/* Shell command handlers */

#ifndef CONFIG_NSH_DISABLE_BASENAME
  int cmd_basename(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
  int cmd_break(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_DIRNAME
  int cmd_dirname(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_ECHO
  int cmd_echo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_PRINTF
  int cmd_printf(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_EXEC
  int cmd_exec(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_MB
  int cmd_mb(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_MH
  int cmd_mh(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_MW
  int cmd_mw(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_FREE
  int cmd_free(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_TIME
  int cmd_time(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_PS
  int cmd_ps(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_NSH_DISABLE_XD
  int cmd_xd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if defined(CONFIG_MODULE) && !defined(CONFIG_NSH_DISABLE_MODCMDS)
int cmd_insmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
int cmd_rmmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_MODULE)
int cmd_lsmod(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#endif

#ifdef HAVE_IRQINFO
int cmd_irqinfo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_TEST)
  int cmd_test(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
  int cmd_lbracket(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_DATE
  int cmd_date(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0
#  ifndef CONFIG_NSH_DISABLE_CAT
      int cmd_cat(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_CP
      int cmd_cp(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_CMP
      int cmd_cmp(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_DD
      int cmd_dd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_HEXDUMP
      int cmd_hexdump(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#  if !defined(CONFIG_NSH_DISABLE_LN) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
      int cmd_ln(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_LS
      int cmd_ls(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if defined(CONFIG_RAMLOG_SYSLOG) && !defined(CONFIG_NSH_DISABLE_DMESG)
      int cmd_dmesg(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if !defined(CONFIG_NSH_DISABLE_READLINK) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
      int cmd_readlink(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if CONFIG_NFILE_STREAMS > 0 && !defined(CONFIG_NSH_DISABLESCRIPT)
#     ifndef CONFIG_NSH_DISABLE_SH
       int cmd_sh(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
# endif  /* CONFIG_NFILE_STREAMS && !CONFIG_NSH_DISABLESCRIPT */

# ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_MKDIR
      int cmd_mkdir(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_MV
      int cmd_mv(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_RM
      int cmd_rm(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_RMDIR
      int cmd_rmdir(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
# endif /* CONFIG_NFILE_STREAMS && NSH_HAVE_DIROPTS */

# ifndef CONFIG_DISABLE_MOUNTPOINT
#   if defined(CONFIG_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSETUP)
       int cmd_losetup(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#   if defined(CONFIG_SMART_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSMART)
       int cmd_losmart(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#   if defined(CONFIG_PIPES) && CONFIG_DEV_FIFO_SIZE > 0 && \
      !defined(CONFIG_NSH_DISABLE_MKFIFO)
       int cmd_mkfifo(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#   ifdef CONFIG_FS_READABLE
#     ifdef NSH_HAVE_CATFILE
#       ifndef CONFIG_NSH_DISABLE_DF
           int cmd_df(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#       endif
#       ifndef CONFIG_NSH_DISABLE_MOUNT
           int cmd_mount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#       endif
#     endif
#     ifndef CONFIG_NSH_DISABLE_UMOUNT
         int cmd_umount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#     ifdef CONFIG_FS_WRITABLE
#       ifndef CONFIG_NSH_DISABLE_MKRD
           int cmd_mkrd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#       endif
#     endif /* CONFIG_FS_WRITABLE */
#   endif /* CONFIG_FS_READABLE */
#   ifdef CONFIG_FSUTILS_MKFATFS
#     ifndef CONFIG_NSH_DISABLE_MKFATFS
         int cmd_mkfatfs(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#   endif /* CONFIG_FSUTILS_MKFATFS */
#   if defined(CONFIG_FS_SMARTFS) && defined(CONFIG_FSUTILS_MKSMARTFS)
#     ifndef CONFIG_NSH_DISABLE_MKSMARTFS
         int cmd_mksmartfs(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#   endif /* CONFIG_FS_SMARTFS */
#   ifndef CONFIG_NSH_DISABLE_TRUNCATE
      int cmd_truncate(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#   if defined(CONFIG_NSH_LOGIN_PASSWD) && defined(CONFIG_FS_WRITABLE) && \
      !defined(CONFIG_FSUTILS_PASSWD_READONLY)
#     ifndef CONFIG_NSH_DISABLE_USERADD
         int cmd_useradd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#     ifndef CONFIG_NSH_DISABLE_USERDEL
         int cmd_userdel(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#     ifndef CONFIG_NSH_DISABLE_PASSWD
         int cmd_passwd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#     endif
#   endif
# endif /* !CONFIG_DISABLE_MOUNTPOINT */
# if !defined(CONFIG_DISABLE_ENVIRON)
#   ifndef CONFIG_NSH_DISABLE_CD
       int cmd_cd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
#   ifndef CONFIG_NSH_DISABLE_PWD
       int cmd_pwd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#   endif
# endif /* !CONFIG_DISABLE_MOUNTPOINT */
#endif /* CONFIG_NFILE_DESCRIPTORS */

#if defined(CONFIG_NET)
#  if defined(CONFIG_NET_ARP) && !defined(CONFIG_NSH_DISABLE_ARP)
      int cmd_arp(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_ADDROUTE)
      int cmd_addroute(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_DELROUTE)
      int cmd_delroute(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#    ifndef CONFIG_NSH_DISABLE_GET
      int cmd_get(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#    endif
#  endif
#  ifndef CONFIG_NSH_DISABLE_IFCONFIG
      int cmd_ifconfig(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_IFUPDOWN
      int cmd_ifup(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
      int cmd_ifdown(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && \
      defined(CONFIG_FS_READABLE) && defined(CONFIG_NFS)
#    ifndef CONFIG_NSH_DISABLE_NFSMOUNT
      int cmd_nfsmount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#    endif
#  endif
#  if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#    ifndef CONFIG_NSH_DISABLE_PUT
      int cmd_put(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#    endif
#  endif
#  if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#    ifndef CONFIG_NSH_DISABLE_WGET
        int cmd_wget(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#    endif
#  endif
#  ifndef CONFIG_NSH_DISABLE_ROUTE
      int cmd_route(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  if defined(CONFIG_NSH_TELNET)
#    ifndef CONFIG_NSH_DISABLE_TELNETD
        int cmd_telnetd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#    endif
#  endif
#endif /* CONFIG_NET */

#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT) && \
   !defined(CONFIG_NSH_DISABLE_NSLOOKUP)
   int cmd_nslookup(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if defined(CONFIG_BOARDCTL_POWEROFF) && !defined(CONFIG_NSH_DISABLE_POWEROFF)
   int cmd_poweroff(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if defined(CONFIG_BOARDCTL_RESET) && !defined(CONFIG_NSH_DISABLE_REBOOT)
   int cmd_reboot(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#if (defined(CONFIG_BOARDCTL_POWEROFF) || defined(CONFIG_BOARDCTL_RESET)) && \
    !defined(CONFIG_NSH_DISABLE_SHUTDOWN)
   int cmd_shutdown(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_UNAME
   int cmd_uname(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_SET
      int cmd_set(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif
#ifndef CONFIG_DISABLE_ENVIRON
#  ifndef CONFIG_NSH_DISABLE_UNSET
      int cmd_unset(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#endif /* CONFIG_DISABLE_ENVIRON */

#ifndef CONFIG_DISABLE_SIGNALS
#  ifndef CONFIG_NSH_DISABLE_KILL
      int cmd_kill(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_SLEEP
      int cmd_sleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_USLEEP
      int cmd_usleep(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#endif /* CONFIG_DISABLE_SIGNALS */

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_BASE64)
#  ifndef CONFIG_NSH_DISABLE_BASE64DEC
      int cmd_base64decode(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_BASE64ENC
      int cmd_base64encode(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_HASH_MD5)
#  ifndef CONFIG_NSH_DISABLE_MD5
      int cmd_md5(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_URLCODE)
#  ifndef CONFIG_NSH_DISABLE_URLDECODE
      int cmd_urlencode(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#  ifndef CONFIG_NSH_DISABLE_URLENCODE
      int cmd_urldecode(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#  endif
#endif

/****************************************************************************
 * Name: nsh_extmatch_count
 *
 * Description:
 *   This support function is used to provide support for realine tab-
 *   completion logic  nsh_extmatch_count() counts the number of matching
 *   nsh command names
 *
 * Input Parameters:
 *   name    - A point to the name containing the name to be matched.
 *   matches - A table is size CONFIG_READLINE_MAX_EXTCMDS that can
 *             be used to remember matching name indices.
 *   namelen - The lenght of the name to match
 *
 * Returned Values:
 *   The number commands that match to the first namelen characters.
 *
 ****************************************************************************/

#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
int nsh_extmatch_count(FAR char *name, FAR int *matches, int namelen);
#endif

/****************************************************************************
 * Name: nsh_extmatch_getname
 *
 * Description:
 *   This support function is used to provide support for realine tab-
 *   completion logic  nsh_extmatch_getname() will return the full command
 *   string from an index that was previously saved by nsh_exmatch_count().
 *
 * Input Parameters:
 *   index - The index of the command name to be returned.
 *
 * Returned Values:
 *   The numb
 *
 ****************************************************************************/

#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
FAR const char *nsh_extmatch_getname(int index);
#endif

/****************************************************************************
 * Name: nsh_catfile
 *
 * Description:
 *   Dump the contents of a file to the current NSH terminal.
 *
 * Input Paratemets:
 *   vtbl     - The console vtable
 *   cmd      - NSH command name to use in error reporting
 *   filepath - The full path to the file to be dumped
 *
 * Returned Value:
 *   Zero (OK) on success; -1 (ERROR) on failure.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_CATFILE
int nsh_catfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR const char *filepath);
#endif

/****************************************************************************
 * Name: nsh_readfile
 *
 * Description:
 *   Read a small file into a user-provided buffer.  The data is assumed to
 *   be a string and is guaranteed to be NUL-termined.  An error occurs if
 *   the file content (+terminator)  will not fit into the provided 'buffer'.
 *
 * Input Paramters:
 *   vtbl     - The console vtable
 *   cmd      - NSH command name to use in error reporting
 *   filepath - The full path to the file to be read
 *   buffer   - The user-provided buffer into which the file is read.
 *   buflen   - The size of the user provided buffer
 *
 * Returned Value:
 *   Zero (OK) is returned on success; -1 (ERROR) is returned on any
 *   failure to read the fil into the buffer.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_READFILE
int nsh_readfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                 FAR const char *filepath, FAR char *buffer, size_t buflen);
#endif

/****************************************************************************
 * Name: nsh_foreach_direntry
 *
 * Description:
 *    Call the provided 'handler' for each entry found in the directory at
 *    'dirpath'.
 *
 * Input Parameters
 *   vtbl     - The console vtable
 *   cmd      - NSH command name to use in error reporting
 *   dirpath  - The full path to the directory to be traversed
 *   handler  - The handler to be called for each entry of the directory
 *   pvarg    - User provided argument to be passed to the 'handler'
 *
 * Returned Value:
 *   Zero (OK) returned on success; -1 (ERROR) returned on failure.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_FOREACH_DIRENTRY
int nsh_foreach_direntry(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                         FAR const char *dirpath,
                         nsh_direntry_handler_t handler, void *pvarg);
#endif

/****************************************************************************
 * Name: nsh_trimdir
 *
 * Description:
 *   Skip any trailing '/' characters (unless it is also the leading '/')
 *
 * Input Parmeters:
 *   dirpath - The directory path to be trimmed.  May be modified!
 *
 * Returned value:
 *   None
 *
 ****************************************************************************/

#ifdef NSH_HAVE_TRIMDIR
void nsh_trimdir(FAR char *dirpath);
#endif

/****************************************************************************
 * Name: nsh_trimspaces
 *
 * Description:
 *   Trim any leading or trailing spaces from a string.
 *
 * Input Parmeters:
 *   str - The sring to be trimmed.  May be modified!
 *
 * Returned value:
 *   The new string pointer.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_TRIMSPACES
FAR char *nsh_trimspaces(FAR char *str);
#endif

#endif /* __APPS_NSHLIB_NSH_H */
