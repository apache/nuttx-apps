/****************************************************************************
 * apps/nshlib/nsh_command.c
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

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#ifdef CONFIG_NSH_BUILTIN_APPS
#  include <nuttx/lib/builtin.h>
#endif

#if defined(CONFIG_SYSTEM_READLINE) && defined(CONFIG_READLINE_HAVE_EXTMATCH)
#  include "system/readline.h"
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Help command summary layout */

#define HELP_LINELEN  80
#define HELP_TABSIZE  4
#define NUM_CMDS      ((sizeof(g_cmdmap)/sizeof(struct cmdmap_s)) - 1)

/* Help marco for nsh command */

#ifdef CONFIG_NSH_DISABLE_HELP
#  define CMD_MAP(cmd, handler, min, max, usage) \
          { cmd, handler, min, max }
#else
#  define CMD_MAP(cmd, handler, min, max, usage) \
          { cmd, handler, min, max, usage }
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cmdmap_s
{
  FAR const char *cmd;    /* Name of the command */
  nsh_cmd_t   handler;    /* Function that handles the command */
  uint8_t     minargs;    /* Minimum number of arguments (including command) */
  uint8_t     maxargs;    /* Maximum number of arguments (including command) */
#ifndef CONFIG_NSH_DISABLE_HELP
  FAR const char *usage;  /* Usage instructions for 'help' command */
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static int  cmd_help(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv);
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
static int  cmd_true(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv);
static int  cmd_false(FAR struct nsh_vtbl_s *vtbl, int argc,
                      FAR char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_EXIT
static int  cmd_exit(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_EXPR
static int cmd_expr(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv);
#endif

static int  cmd_unrecognized(FAR struct nsh_vtbl_s *vtbl, int argc,
                             FAR char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct cmdmap_s g_cmdmap[] =
{
#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_SOURCE)
  CMD_MAP(".",        cmd_source,   2, 2, "<script-path>"),
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_TEST)
  CMD_MAP("[",        cmd_lbracket,
          4, CONFIG_NSH_MAXARGUMENTS, "<expression> ]"),
#endif

#ifndef CONFIG_NSH_DISABLE_HELP
  CMD_MAP("?",        cmd_help,     1, 1, NULL),
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_ADDROUTE)
  CMD_MAP("addroute", cmd_addroute, 3, 4, "<target> [<netmask>] <router>"),
#endif

#ifdef CONFIG_NSH_ALIAS
  CMD_MAP("alias",    cmd_alias,    1, CONFIG_NSH_MAXARGUMENTS,
    "[name[=value] ... ]"),
  CMD_MAP("unalias",  cmd_unalias,  1, CONFIG_NSH_MAXARGUMENTS,
    "[-a] name [name ... ]"),
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ARP) && !defined(CONFIG_NSH_DISABLE_ARP)
  CMD_MAP("arp",      cmd_arp,      1, 6,
    "[-i <ifname>] [-a <ipaddr>|-d <ipaddr>|-s <ipaddr> <hwaddr>]"),
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_BASE64)
#  ifndef CONFIG_NSH_DISABLE_BASE64DEC
  CMD_MAP("base64dec", cmd_base64decode,
          2, 4, "[-w] [-f] <string or filepath>"),
#  endif
#  ifndef CONFIG_NSH_DISABLE_BASE64ENC
  CMD_MAP("base64enc", cmd_base64encode,
          2, 4, "[-w] [-f] <string or filepath>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_BASENAME
  CMD_MAP("basename", cmd_basename, 2, 3, "<path> [<suffix>]"),
#endif

#if defined(CONFIG_BOARDCTL_BOOT_IMAGE) && !defined(CONFIG_NSH_DISABLE_BOOT)
  CMD_MAP("boot",     cmd_boot,     1, 3, "[<image path> [<header size>]]"),
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
  CMD_MAP("break",    cmd_break,    1, 1, NULL),
#endif

#ifndef CONFIG_NSH_DISABLE_CAT
  CMD_MAP("cat",      cmd_cat,      1, CONFIG_NSH_MAXARGUMENTS,
    "[<path> [<path> [<path> ...]]]"),
#endif

#ifndef CONFIG_NSH_DISABLE_CD
  CMD_MAP("cd",       cmd_cd,       1, 2, "[<dir-path>|-|~|..]"),
#endif

#ifndef CONFIG_NSH_DISABLE_CP
  CMD_MAP("cp",       cmd_cp,       3, 4, "[-r] <source-path> <dest-path>"),
#endif

#ifndef CONFIG_NSH_DISABLE_CMP
  CMD_MAP("cmp",      cmd_cmp,      3, 3, "<path1> <path2>"),
#endif

#ifndef CONFIG_NSH_DISABLE_DIRNAME
  CMD_MAP("dirname",  cmd_dirname,  2, 2, "<path>"),
#endif

#ifndef CONFIG_NSH_DISABLE_DATE
  CMD_MAP("date",     cmd_date,
          1, 4, "[-s \"MMM DD HH:MM:SS YYYY\"] [-u] [+format]"),
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_DELROUTE)
  CMD_MAP("delroute", cmd_delroute, 2, 3, "<target> [<netmask>]"),
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && !defined(CONFIG_NSH_DISABLE_DF)
#ifdef NSH_HAVE_CATFILE
#if defined(HAVE_DF_HUMANREADBLE) && defined(HAVE_DF_BLOCKOUTPUT)
  CMD_MAP("df",       cmd_df,       1, 2, "[-h]"),
#else
  CMD_MAP("df",       cmd_df,       1, 1, NULL),
#endif
#endif
#endif

#if defined(CONFIG_SYSLOG_DEVPATH) && !defined(CONFIG_NSH_DISABLE_DMESG)
  CMD_MAP("dmesg",    cmd_dmesg,    1, 2, "[-c,--clear |-C,--read-clear]"),
#endif

#ifndef CONFIG_NSH_DISABLE_ECHO
#  ifndef CONFIG_DISABLE_ENVIRON
  CMD_MAP("echo",     cmd_echo,     1, CONFIG_NSH_MAXARGUMENTS,
    "[-n] [<string|$name> [<string|$name>...]]"),
#  else
  CMD_MAP("echo",     cmd_echo,     1, CONFIG_NSH_MAXARGUMENTS,
    "[-n] [<string> [<string>...]]"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_ENV
  CMD_MAP("env",      cmd_env,      1, 1, NULL),
#endif

#ifndef CONFIG_NSH_DISABLE_EXEC
  CMD_MAP("exec",     cmd_exec,     2, 3, "<hex-address>"),
#endif

#ifndef CONFIG_NSH_DISABLE_EXIT
  CMD_MAP("exit",     cmd_exit,     1, 1, NULL),
#endif

#ifndef CONFIG_NSH_DISABLE_EXPR
  CMD_MAP("expr",     cmd_expr,     4, 4,
    "<operand1> <operator> <operand2>"),
#endif

#ifndef CONFIG_NSH_DISABLE_EXPORT
  CMD_MAP("export",   cmd_export,   2, 3, "[<name> [<value>]]"),
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  CMD_MAP("false",    cmd_false,    1, 1, NULL),
#endif

#ifdef CONFIG_FS_PROCFS
#  ifndef CONFIG_NSH_DISABLE_FDINFO
  CMD_MAP("fdinfo",   cmd_fdinfo,   1, 2, "[pid]"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_FREE
  CMD_MAP("free",     cmd_free,     1, 1, NULL),
#endif

#ifdef CONFIG_DEBUG_MM
#  ifndef CONFIG_NSH_DISABLE_MEMDUMP
  CMD_MAP("memdump",  cmd_memdump,
          1, 4, "[pid/used/free/on/off]" " <minseq> <maxseq>"),
#  endif
#endif

#ifdef CONFIG_NET_UDP
#  ifndef CONFIG_NSH_DISABLE_GET
  CMD_MAP("get",      cmd_get,      4, 7,
    "[-b|-n] [-f <local-path>] -h <ip-address> <remote-path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_HELP
#  ifdef CONFIG_NSH_HELP_TERSE
  CMD_MAP("help",     cmd_help,     1, 2, "[<cmd>]"),
#  else
  CMD_MAP("help",     cmd_help,     1, 3, "[-v] [<cmd>]"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_HEXDUMP
#ifndef CONFIG_NSH_CMDOPT_HEXDUMP
  CMD_MAP("hexdump",  cmd_hexdump,  2, 2, "<file or device>"),
#else
  CMD_MAP("hexdump",  cmd_hexdump,  2, 4,
    "<file or device> [skip=<bytes>] [count=<bytes>]"),
#endif
#endif

#ifdef CONFIG_NET
#  ifndef CONFIG_NSH_DISABLE_IFCONFIG
  CMD_MAP("ifconfig", cmd_ifconfig, 1, 12,
    "[interface [mtu <len>]|[address_family] [[add|del] <ip-address>|dhcp]]"
    "[dr|gw|gateway <dr-address>] [netmask <net-mask>|prefixlen <len>] "
    "[dns <dns-address>] [hw <hw-mac>]"),
#  endif
#  ifndef CONFIG_NSH_DISABLE_IFUPDOWN
  CMD_MAP("ifdown",   cmd_ifdown,   2, 2, "<interface>"),
  CMD_MAP("ifup",     cmd_ifup,     2, 2, "<interface>"),
#  endif
#endif

#if defined(CONFIG_MODULE) && !defined(CONFIG_NSH_DISABLE_MODCMDS)
  CMD_MAP("insmod",   cmd_insmod,   3, 3, "<file-path> <module-name>"),
#endif

#ifdef HAVE_IRQINFO
  CMD_MAP("irqinfo",  cmd_irqinfo,  1, 1, NULL),
#endif

#ifndef CONFIG_NSH_DISABLE_KILL
  CMD_MAP("kill",     cmd_kill,     2, 3, "[-<signal>] <pid>"),
#endif

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_PKILL)
  CMD_MAP("pkill",     cmd_pkill,     2, 3, "[-<signal>] <name>"),
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  if defined(CONFIG_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSETUP)
  CMD_MAP("losetup",  cmd_losetup,  3, 6,
    "[-d <dev-path>] | [[-o <offset>] [-r] [-b <sect-size>] "
    "<dev-path> <file-path>]"),
#  endif
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  if defined(CONFIG_SMART_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSMART)
  CMD_MAP("losmart",  cmd_losmart,  2, 11,
    "[-d <dev-path>] | [[-m <minor>] [-o <offset>] [-e <erase-size>] "
    "[-s <sect-size>] [-r] <file-path>]"),
#  endif
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  if defined(CONFIG_MTD_LOOP) && !defined(CONFIG_NSH_DISABLE_LOMTD)
  CMD_MAP("lomtd",    cmd_lomtd,    3, 9,
    "[-d <dev-path>] | [[-o <offset>] [-e <erase-size>] "
    "[-b <sect-size>] <dev-path> <file-path>]]"),
#  endif
#endif

#if !defined(CONFIG_NSH_DISABLE_LN) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
  CMD_MAP("ln",       cmd_ln,       3, 4, "[-s] <target> <link>"),
#endif

#ifndef CONFIG_NSH_DISABLE_LS
  CMD_MAP("ls",       cmd_ls,       1, 5, "[-lRsh] <dir-path>"),
#endif

#if defined(CONFIG_MODULE) && !defined(CONFIG_NSH_DISABLE_MODCMDS)
#  if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_MODULE)
  CMD_MAP("lsmod",    cmd_lsmod,    1, 1,  NULL),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_MB
  CMD_MAP("mb",       cmd_mb,       2, 3,
    "<hex-address>[=<hex-value>] [<hex-byte-count>]"),
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_HASH_MD5)
#  ifndef CONFIG_NSH_DISABLE_MD5
  CMD_MAP("md5",      cmd_md5,      1, 3,
          "[string] or [-f <filepath>] or read stdin"),
#  endif
#endif

#ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_MKDIR
  CMD_MAP("mkdir",    cmd_mkdir,    2, 3, "[-p] <path>"),
#  endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FSUTILS_MKFATFS)
#  ifndef CONFIG_NSH_DISABLE_MKFATFS
  CMD_MAP("mkfatfs",  cmd_mkfatfs,  2, 6,
    "[-F <fatsize>] [-r <rootdirentries>] <block-driver>"),
#  endif
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  if defined(CONFIG_PIPES) && CONFIG_DEV_FIFO_SIZE > 0 && \
    !defined(CONFIG_NSH_DISABLE_MKFIFO)
  CMD_MAP("mkfifo",   cmd_mkfifo,   2, 2, "<path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_MKRD
  CMD_MAP("mkrd",     cmd_mkrd,     2, 6,
    "[-m <minor>] [-s <sector-size>] <nsectors>"),
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_SMARTFS) && \
    defined(CONFIG_FSUTILS_MKSMARTFS)
#  ifndef CONFIG_NSH_DISABLE_MKSMARTFS
#    ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
  CMD_MAP("mksmartfs", cmd_mksmartfs, 2, 6,
    "[-s <sector-size>] [-f] <path> [<num-root-directories>]"),
#    else
  CMD_MAP("mksmartfs", cmd_mksmartfs,
          2, 5, "[-s <sector-size>] [-f] <path>"),
#    endif
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_MH
  CMD_MAP("mh",       cmd_mh,       2, 3,
    "<hex-address>[=<hex-value>] [<hex-byte-count>]"),
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  ifndef CONFIG_NSH_DISABLE_MOUNT
#    if defined(NSH_HAVE_CATFILE) && defined(HAVE_MOUNT_LIST)
  CMD_MAP("mount",    cmd_mount,    1, 7,
    "[-t <fstype> [-o <options>] [<block-device>] <mount-point>]"),
#    else
  CMD_MAP("mount",    cmd_mount,    4, 7,
    "-t <fstype> [-o <options>] [<block-device>] <mount-point>"),
#    endif
#  endif
#endif

#ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_MV
  CMD_MAP("mv",       cmd_mv,       3, 3, "<old-path> <new-path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_MW
  CMD_MAP("mw",       cmd_mw,       2, 3,
    "<hex-address>[=<hex-value>] [<hex-byte-count>]"),
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_NET) && \
    defined(CONFIG_NFS)
#  ifndef CONFIG_NSH_DISABLE_NFSMOUNT
  CMD_MAP("nfsmount", cmd_nfsmount, 4, 5,
    "<server-address> <mount-point> <remote-path> [udp]"),
#  endif
#endif

#if defined(CONFIG_LIBC_NETDB) && !defined(CONFIG_NSH_DISABLE_NSLOOKUP)
  CMD_MAP("nslookup", cmd_nslookup, 2, 2, "<host-name>"),
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && \
     defined(CONFIG_NSH_LOGIN_PASSWD) && \
    !defined(CONFIG_FSUTILS_PASSWD_READONLY)
#  ifndef CONFIG_NSH_DISABLE_PASSWD
  CMD_MAP("passwd",   cmd_passwd,   3, 3, "<username> <password>"),
#  endif
#endif

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_PIDOF)
  CMD_MAP("pidof",   cmd_pidof, 2, 2, "<name>"),
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_NSH_DISABLE_PMCONFIG)
  CMD_MAP("pmconfig", cmd_pmconfig, 1, 4,
    "[stay|relax] [normal|idle|standby|sleep] [domain]"),
#endif

#if defined(CONFIG_BOARDCTL_POWEROFF) && !defined(CONFIG_NSH_DISABLE_POWEROFF)
  CMD_MAP("poweroff", cmd_poweroff, 1, 2, NULL),
  CMD_MAP("quit", cmd_poweroff, 1, 2, NULL),
#endif

#ifndef CONFIG_NSH_DISABLE_PRINTF
#  ifndef CONFIG_DISABLE_ENVIRON
  CMD_MAP("printf",   cmd_printf,   1, CONFIG_NSH_MAXARGUMENTS,
    "[\\xNN] [\\n\\r\\t] [<string|$name> [<string|$name>...]]"),
#  else
  CMD_MAP("printf",   cmd_printf,   1, CONFIG_NSH_MAXARGUMENTS,
    "[\\xNN] [\\n\\r\\t] [<string> [<string>...]]"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_PS
  CMD_MAP("ps",       cmd_ps,       1, CONFIG_NSH_MAXARGUMENTS,
    "<-heap> <pid1 pid2 ...>"),
#endif

#ifdef CONFIG_NET_UDP
#  ifndef CONFIG_NSH_DISABLE_PUT
  CMD_MAP("put",      cmd_put,      4, 7,
    "[-b|-n] [-f <remote-path>] -h <ip-address> <local-path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_PWD
  CMD_MAP("pwd",      cmd_pwd,      1, 1, NULL),
#endif

#if !defined(CONFIG_NSH_DISABLE_READLINK) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
  CMD_MAP("readlink", cmd_readlink, 2, 2, "<link>"),
#endif

#if defined(CONFIG_BOARDCTL_RESET) && !defined(CONFIG_NSH_DISABLE_REBOOT)
  CMD_MAP("reboot",   cmd_reboot,   1, 2, NULL),
#endif

#if defined(CONFIG_BOARDCTL_RESET_CAUSE) && !defined(CONFIG_NSH_DISABLE_RESET_CAUSE)
  CMD_MAP("resetcause", cmd_reset_cause, 1, 1, NULL),
#endif

#if defined(CONFIG_BOARDCTL_IRQ_AFFINITY) && !defined(CONFIG_NSH_DISABLE_IRQ_AFFINITY)
  CMD_MAP("irqaff", cmd_irq_affinity, 3, 3,
    "irqaff [IRQ Number] [Core Mask]"),
#endif

#ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_RM
  CMD_MAP("rm",       cmd_rm,       2, 3, "[-rf] <file-path>"),
#  endif
#endif

#ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_RMDIR
  CMD_MAP("rmdir",    cmd_rmdir,    2, 2, "<dir-path>"),
#  endif
#endif

#if defined(CONFIG_MODULE) && !defined(CONFIG_NSH_DISABLE_MODCMDS)
  CMD_MAP("rmmod",    cmd_rmmod,    2, 2, "<module-name>"),
#endif

#ifndef CONFIG_NSH_DISABLE_ROUTE
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  CMD_MAP("route",    cmd_route,    2, 2, "ipv4|ipv6"),
#elif defined(CONFIG_NET_IPv4)
  CMD_MAP("route",    cmd_route,    1, 2, "[ipv4]"),
#elif defined(CONFIG_NET_IPv6)
  CMD_MAP("route",    cmd_route,    1, 2, "[ipv6]"),
#endif
#endif

#if defined(CONFIG_RPMSG) && !defined(CONFIG_NSH_DISABLE_RPMSG)
  CMD_MAP("rpmsg",    cmd_rpmsg,    2, 7,
    "<panic|dump|ping> <path|all>"
    " [value|times length ack sleep]"),
#endif

#if defined(CONFIG_RPTUN) && !defined(CONFIG_NSH_DISABLE_RPTUN)
  CMD_MAP("rptun",    cmd_rptun,    2, 7,
    "<start|stop|reset|panic|dump|ping> <path|all>"
    " [value|times length ack sleep]"),
#endif

#ifndef CONFIG_NSH_DISABLE_SET
#ifdef CONFIG_NSH_VARS
#  if !defined(CONFIG_DISABLE_ENVIRON) && !defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,
          1, 4, "[{+|-}{e|x|xe|ex}] [<name> <value>]"),
#  elif !defined(CONFIG_DISABLE_ENVIRON) && defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,      1, 3, "[<name> <value>]"),
#  elif defined(CONFIG_DISABLE_ENVIRON) && !defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,      1, 2, "[{+|-}{e|x|xe|ex}]"),
#  endif
#else
#  if !defined(CONFIG_DISABLE_ENVIRON) && !defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,
          2, 4, "[{+|-}{e|x|xe|ex}] [<name> <value>]"),
#  elif !defined(CONFIG_DISABLE_ENVIRON) && defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,      3, 3, "<name> <value>"),
#  elif defined(CONFIG_DISABLE_ENVIRON) && !defined(CONFIG_NSH_DISABLESCRIPT)
  CMD_MAP("set",      cmd_set,      2, 2, "{+|-}{e|x|xe|ex}"),
#  endif
#endif
#endif /* CONFIG_NSH_DISABLE_SET */

#ifndef CONFIG_NSH_DISABLE_SHUTDOWN
#if defined(CONFIG_BOARDCTL_POWEROFF) && defined(CONFIG_BOARDCTL_RESET)
  CMD_MAP("shutdown", cmd_shutdown, 1, 2, "[--reboot]"),
#elif defined(CONFIG_BOARDCTL_POWEROFF)
  CMD_MAP("shutdown", cmd_shutdown, 1, 1, NULL),
#elif defined(CONFIG_BOARDCTL_RESET)
  CMD_MAP("shutdown", cmd_shutdown, 2, 2, "--reboot"),
#endif
#endif

#ifndef CONFIG_NSH_DISABLE_SLEEP
  CMD_MAP("sleep",    cmd_sleep,    2, 2, "<sec>"),
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_SOURCE)
  CMD_MAP("source",   cmd_source,   2, 2, "<script-path>"),
#endif

#if defined(CONFIG_BOARDCTL_SWITCH_BOOT) && !defined(CONFIG_NSH_DISABLE_SWITCHBOOT)
  CMD_MAP("swtichboot", cmd_switchboot, 2, 2, "<image path>"),
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_TEST)
  CMD_MAP("test",     cmd_test,
          3, CONFIG_NSH_MAXARGUMENTS, "<expression>"),
#endif

#if !defined(CONFIG_NSH_DISABLE_TOP) && defined(NSH_HAVE_CPULOAD)
  CMD_MAP("top",       cmd_top,       1, 5,
          "[ -n <num> ][ -d <delay>] [ -p <pidlist>] [-h]"),
#endif

#ifndef CONFIG_NSH_DISABLE_TIME
  CMD_MAP("time",     cmd_time,     2, 2, "\"<command>\""),
#endif

#ifndef CONFIG_NSH_DISABLE_TIMEDATECTL
  CMD_MAP("timedatectl", cmd_timedatectl, 1, 3, "[set-timezone TZ]"),
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  CMD_MAP("true",     cmd_true,     1, 1, NULL),
#endif

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  ifndef CONFIG_NSH_DISABLE_TRUNCATE
  CMD_MAP("truncate", cmd_truncate, 4, 4, "-s <length> <file-path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_UNAME
#  ifdef CONFIG_NET
  CMD_MAP("uname",    cmd_uname,    1, 7, "[-a | -imnoprsv]"),
#  else
  CMD_MAP("uname",    cmd_uname,    1, 7, "[-a | -imoprsv]"),
#  endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  ifndef CONFIG_NSH_DISABLE_UMOUNT
  CMD_MAP("umount",   cmd_umount,   2, 2, "<dir-path>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_UNSET
  CMD_MAP("unset",    cmd_unset,    2, 2, "<name>"),
#endif

#ifndef CONFIG_NSH_DISABLE_UPTIME
  CMD_MAP("uptime",   cmd_uptime,   1, 2, "[-sph]"),
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_URLCODE)
#  ifndef CONFIG_NSH_DISABLE_URLDECODE
  CMD_MAP("urldecode", cmd_urldecode, 2, 3, "[-f] <string or filepath>"),
#  endif
#  ifndef CONFIG_NSH_DISABLE_URLENCODE
  CMD_MAP("urlencode", cmd_urlencode, 2, 3, "[-f] <string or filepath>"),
#  endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && \
     defined(CONFIG_NSH_LOGIN_PASSWD) && \
    !defined(CONFIG_FSUTILS_PASSWD_READONLY)
#  ifndef CONFIG_NSH_DISABLE_USERADD
  CMD_MAP("useradd",  cmd_useradd,  3, 3, "<username> <password>"),
#  endif
#  ifndef CONFIG_NSH_DISABLE_USERDEL
  CMD_MAP("userdel",  cmd_userdel,  2, 2, "<username>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_USLEEP
  CMD_MAP("usleep",   cmd_usleep,   2, 2, "<usec>"),
#endif

#ifndef CONFIG_NSH_DISABLE_WATCH
  CMD_MAP("watch",     cmd_watch,
          2, 6, "[-n] interval [-c] count <command>"),
#endif

#ifdef CONFIG_NET_TCP
#  ifndef CONFIG_NSH_DISABLE_WGET
  CMD_MAP("wget",     cmd_wget,     2, 4, "[-o <local-path>] <url>"),
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_XD
  CMD_MAP("xd",       cmd_xd,       3, 3, "<hex-address> <byte-count>"),
#endif
#if !defined(CONFIG_NSH_DISABLE_WAIT) && defined(CONFIG_SCHED_WAITPID) && \
    !defined(CONFIG_DISABLE_PTHREAD) && defined(CONFIG_FS_PROCFS) && \
    !defined(CONFIG_FS_PROCFS_EXCLUDE_PROCESS)
  CMD_MAP("wait",     cmd_wait,     1, CONFIG_NSH_MAXARGUMENTS,
          "pid1 [pid2 [pid3] ...]"),
#endif
  CMD_MAP(NULL,       NULL,         1, 1, NULL)
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: help_cmdlist
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static inline void help_cmdlist(FAR struct nsh_vtbl_s *vtbl)
{
  unsigned int colwidth;
  unsigned int cmdwidth;
  unsigned int cmdsperline;
  unsigned int ncmdrows;
  unsigned int i;
  unsigned int j;
  unsigned int k;
  unsigned int offset;

  /* Extra 5 bytes for tab before newline and '\0' */

  char line[HELP_LINELEN + HELP_TABSIZE + 1];

  /* Pick an optimal column width */

  for (k = 0, colwidth = 0; k < NUM_CMDS; k++)
    {
      cmdwidth = strlen(g_cmdmap[k].cmd);
      if (cmdwidth > colwidth)
        {
          colwidth = cmdwidth;
        }
    }

  colwidth += HELP_TABSIZE;

  /* Determine the number of commands to put on one line */

  if (colwidth > HELP_LINELEN)
    {
      cmdsperline = 1;
    }
  else
    {
      cmdsperline = HELP_LINELEN / colwidth;
    }

  /* Determine the total number of lines to output */

  ncmdrows = (NUM_CMDS + (cmdsperline - 1)) / cmdsperline;

  /* Print the command name in 'ncmdrows' rows with 'cmdsperline' commands
   * on each line.
   */

  for (i = 0; i < ncmdrows; i++)
    {
      /* Tab before a new line */

      offset = HELP_TABSIZE;
      memset(line, ' ', offset);

      for (j = 0, k = i;
           j < cmdsperline && k < NUM_CMDS;
           j++, k += ncmdrows)
        {
          /* Copy the cmd name to line buffer */

          offset += strlcpy(line + offset, g_cmdmap[k].cmd,
                            sizeof(line) - offset);

          /* Add space between commands */

          for (cmdwidth = strlen(g_cmdmap[k].cmd);
               cmdwidth < colwidth;
               cmdwidth++)
            {
              line[offset++] = ' ';
            }
        }

      line[offset++] = '\n';
      nsh_write(vtbl, line, offset);
    }
}
#endif

/****************************************************************************
 * Name: help_usage
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_HELP) && !defined(CONFIG_NSH_HELP_TERSE)
static inline void help_usage(FAR struct nsh_vtbl_s *vtbl)
{
  nsh_output(vtbl, "NSH command forms:\n");
#ifndef CONFIG_NSH_DISABLEBG
  nsh_output(vtbl, "  [nice [-d <niceness>>]] <cmd> "
                   "[> <file>|>> <file>] [&]\n\n");
#else
  nsh_output(vtbl, "  <cmd> [> <file>|>> <file>]\n\n");
#endif
#ifndef CONFIG_NSH_DISABLESCRIPT
#ifndef CONFIG_NSH_DISABLE_ITEF
  nsh_output(vtbl, "OR\n");
  nsh_output(vtbl, "  if <cmd>\n");
  nsh_output(vtbl, "  then\n");
  nsh_output(vtbl, "    [sequence of <cmd>]\n");
  nsh_output(vtbl, "  else\n");
  nsh_output(vtbl, "    [sequence of <cmd>]\n");
  nsh_output(vtbl, "  fi\n\n");
#endif
#ifndef CONFIG_NSH_DISABLE_LOOPS
  nsh_output(vtbl, "OR\n");
  nsh_output(vtbl, "  while <cmd>\n");
  nsh_output(vtbl, "  do\n");
  nsh_output(vtbl, "    [sequence of <cmd>]\n");
  nsh_output(vtbl, "  done\n\n");
  nsh_output(vtbl, "OR\n");
  nsh_output(vtbl, "  until <cmd>\n");
  nsh_output(vtbl, "  do\n");
  nsh_output(vtbl, "    [sequence of <cmd>]\n");
  nsh_output(vtbl, "  done\n\n");
#endif
#endif
}
#endif

/****************************************************************************
 * Name: help_showcmd
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static void help_showcmd(FAR struct nsh_vtbl_s *vtbl,
                         FAR const struct cmdmap_s *cmdmap)
{
  if (cmdmap->usage)
    {
      nsh_output(vtbl, "  %s %s\n", cmdmap->cmd, cmdmap->usage);
    }
  else
    {
      nsh_output(vtbl, "  %s\n", cmdmap->cmd);
    }
}
#endif

/****************************************************************************
 * Name: help_cmd
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static int help_cmd(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd)
{
  FAR const struct cmdmap_s *cmdmap;

  /* Find the command in the command table */

  for (cmdmap = g_cmdmap; cmdmap->cmd; cmdmap++)
    {
      /* Is this the one we are looking for? */

      if (strcmp(cmdmap->cmd, cmd) == 0)
        {
          /* Yes... show it */

          nsh_output(vtbl, "%s usage:", cmd);
          help_showcmd(vtbl, cmdmap);
          return OK;
        }
    }

  nsh_error(vtbl, g_fmtcmdnotfound, cmd);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: help_allcmds
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_HELP) && !defined(CONFIG_NSH_HELP_TERSE)
static inline void help_allcmds(FAR struct nsh_vtbl_s *vtbl)
{
  FAR const struct cmdmap_s *cmdmap;

  /* Show all of the commands in the command table */

  for (cmdmap = g_cmdmap; cmdmap->cmd; cmdmap++)
    {
      help_showcmd(vtbl, cmdmap);
    }
}
#endif

/****************************************************************************
 * Name: help_builtins
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static inline void help_builtins(FAR struct nsh_vtbl_s *vtbl)
{
  UNUSED(vtbl);

#ifdef CONFIG_NSH_BUILTIN_APPS
  FAR const struct builtin_s *builtin;
  unsigned int builtins_per_line;
  unsigned int num_builtin_rows;
  unsigned int builtin_width;
  unsigned int num_builtins;
  unsigned int column_width;
  unsigned int i;
  unsigned int j;
  unsigned int k;
  unsigned int offset;

  /* Extra 5 bytes for tab before newline and '\0' */

  char line[HELP_LINELEN + HELP_TABSIZE + 1];

  static FAR const char *const g_builtin_prompt = "\nBuiltin Apps:\n";

  /* Count the number of built-in commands and get the optimal column width */

  num_builtins = 0;
  column_width = 0;

  for (i = 0; (builtin = builtin_for_index(i)) != NULL; i++)
    {
      if (builtin->main == NULL)
        {
          continue;
        }

      num_builtins++;

      builtin_width = strlen(builtin->name);
      if (builtin_width > column_width)
        {
          column_width = builtin_width;
        }
    }

  /* Skip the printing if no available built-in commands */

  if (num_builtins == 0)
    {
      return;
    }

  column_width += HELP_TABSIZE;

  /* Determine the number of commands to put on one line */

  if (column_width > HELP_LINELEN)
    {
      builtins_per_line = 1;
    }
  else
    {
      builtins_per_line = HELP_LINELEN / column_width;
    }

  /* Determine the total number of lines to output */

  num_builtin_rows = ((num_builtins + (builtins_per_line - 1)) /
                      builtins_per_line);

  /* List the set of available built-in commands */

  nsh_write(vtbl, g_builtin_prompt, strlen(g_builtin_prompt));
  for (i = 0; i < num_builtin_rows; i++)
    {
      offset = HELP_TABSIZE;
      memset(line, ' ', offset);

      for (j = 0, k = i;
           j < builtins_per_line &&
           (builtin = builtin_for_index(k));
           j++, k += num_builtin_rows)
        {
          if (builtin->main == NULL)
            {
              continue;
            }

          offset += strlcpy(line + offset, builtin->name,
                            sizeof(line) - offset);

          for (builtin_width = strlen(builtin->name);
               builtin_width < column_width;
               builtin_width++)
            {
              line[offset++] = ' ';
            }
        }

      line[offset++] = '\n';
      nsh_write(vtbl, line, offset);
    }
#endif
}
#endif

/****************************************************************************
 * Name: cmd_help
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static int cmd_help(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR const char *cmd = NULL;
#ifndef CONFIG_NSH_HELP_TERSE
  bool verbose = false;
  int i;
#endif

  /* The command may be followed by a verbose option */

#ifndef CONFIG_NSH_HELP_TERSE
  i = 1;
  if (argc > i)
    {
      if (strcmp(argv[i], "-v") == 0)
        {
          verbose = true;
          i++;
        }
    }

  /* The command line may end with a command name */

  if (argc > i)
    {
      cmd = argv[i];
    }

  /* Show the generic usage if verbose is requested */

  if (verbose)
    {
      help_usage(vtbl);
    }
#else
  if (argc > 1)
    {
      cmd = argv[1];
    }
#endif

  /* Are we showing help on a single command? */

  if (cmd)
    {
      /* Yes.. show the single command */

      help_cmd(vtbl, cmd);
    }
  else
    {
      /* In verbose mode, show detailed help for all commands */

#ifndef CONFIG_NSH_HELP_TERSE
      if (verbose)
        {
          nsh_output(vtbl, "Where <cmd> is one of:\n");
          help_allcmds(vtbl);
        }

      /* Otherwise, just show the list of command names */

      else
#endif
        {
          help_cmd(vtbl, "help");
          nsh_output(vtbl, "\n");
          help_cmdlist(vtbl);
        }

      /* And show the list of built-in applications */

      help_builtins(vtbl);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_unrecognized
 ****************************************************************************/

static int cmd_unrecognized(FAR struct nsh_vtbl_s *vtbl, int argc,
                            FAR char **argv)
{
  UNUSED(argc);

  nsh_error(vtbl, g_fmtcmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: cmd_true
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLESCRIPT
static int cmd_true(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(vtbl);
  UNUSED(argc);
  UNUSED(argv);

  return OK;
}

#endif

/****************************************************************************
 * Name: cmd_false
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLESCRIPT
static int cmd_false(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(vtbl);
  UNUSED(argc);
  UNUSED(argv);

  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_exit
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_EXIT
static int cmd_exit(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);
  UNUSED(argv);

  nsh_exit(vtbl, 0);
  return OK;
}
#endif

#ifndef CONFIG_NSH_DISABLE_EXPR
static int cmd_expr(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  int operand1;
  int operand2;
  int result;
  FAR char *endptr;

  if (argc != 4)
    {
      nsh_output(vtbl, "Usage: %s <operand1> <operator> <operand2>\n",
                 argv[0]);
      return ERROR;
    }

  operand1 = strtol(argv[1], &endptr, 0);
  if (*endptr != '\0')
    {
      nsh_output(vtbl, "operand1 invalid\n");
      return ERROR;
    }

  operand2 = strtol(argv[3], &endptr, 0);
  if (*endptr != '\0')
    {
      nsh_output(vtbl, "operand2 invalid\n");
      return ERROR;
    }

  switch (argv[2][0])
    {
      case '+':
        result = operand1 + operand2;
        break;
      case '-':
        result = operand1 - operand2;
        break;
      case '*':
        result = operand1 * operand2;
        break;
      case '/':
        if (operand2 == 0)
          {
            nsh_output(vtbl, "operand2 invalid\n");
            return ERROR;
          }

        result = operand1 / operand2;
        break;
      case '%':
        if (operand2 == 0)
          {
            nsh_output(vtbl, "operand2 invalid\n");
            return ERROR;
          }

        result = operand1 % operand2;
        break;
      default:
        nsh_output(vtbl, "Unknown operator\n");
        return ERROR;
    }

  nsh_output(vtbl, "%d\n", result);
  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_command
 *
 * Description:
 *   Execute the command in argv[0]
 *
 * Returned Value:
 *   -1 (ERROR) if the command was unsuccessful
 *    0 (OK)     if the command was successful
 *
 ****************************************************************************/

int nsh_command(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char *argv[])
{
  const struct cmdmap_s *cmdmap;
  const char            *cmd;
  nsh_cmd_t              handler = cmd_unrecognized;
  int                    ret;

  /* The form of argv is:
   *
   * argv[0]:      The command name.  This is argv[0] when the arguments
   *               are, finally, received by the command vtblr
   * argv[1]:      The beginning of argument (up to CONFIG_NSH_MAXARGUMENTS)
   * argv[argc]:   NULL terminating pointer
   */

  cmd = argv[0];

  /* See if the command is one that we understand */

  for (cmdmap = g_cmdmap; cmdmap->cmd; cmdmap++)
    {
      if (strcmp(cmdmap->cmd, cmd) == 0)
        {
          /* Check if a valid number of arguments was provided.  We
           * do this simple, imperfect checking here so that it does
           * not have to be performed in each command.
           */

          if (argc < cmdmap->minargs)
            {
              /* Fewer than the minimum number were provided */

              nsh_error(vtbl, g_fmtargrequired, cmd);
              return ERROR;
            }
          else if (argc > cmdmap->maxargs)
            {
              /* More than the maximum number were provided */

              nsh_error(vtbl, g_fmttoomanyargs, cmd);
              return ERROR;
            }
          else
            {
              /* A valid number of arguments were provided (this does
               * not mean they are right).
               */

              handler = cmdmap->handler;
              break;
            }
        }
    }

  ret = handler(vtbl, argc, argv);
  vtbl->np.np_lastpid = getpid();
  return ret;
}

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
 *   namelen - The length of the name to match
 *
 * Returned Values:
 *   The number commands that match to the first namelen characters.
 *
 ****************************************************************************/

#if defined(CONFIG_NSH_READLINE) && defined(CONFIG_READLINE_TABCOMPLETION) && \
    defined(CONFIG_READLINE_HAVE_EXTMATCH)
int nsh_extmatch_count(FAR char *name, FAR int *matches, int namelen)
{
  int nr_matches = 0;
  int i;

  for (i = 0; i < (int)NUM_CMDS; i++)
    {
      if (strncmp(name, g_cmdmap[i].cmd, namelen) == 0)
        {
          matches[nr_matches] = i;
          nr_matches++;

          if (nr_matches >= CONFIG_READLINE_MAX_EXTCMDS)
            {
              break;
            }
        }
    }

  return nr_matches;
}
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
FAR const char *nsh_extmatch_getname(int index)
{
  DEBUGASSERT(index > 0 && index <= (int)NUM_CMDS);
  return  g_cmdmap[index].cmd;
}
#endif
