/****************************************************************************
 * apps/nshlib/nsh_command.c
 *
 *   Copyright (C) 2007-2015 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <string.h>

#ifdef CONFIG_NSH_BUILTIN_APPS
#  include <nuttx/binfmt/builtin.h>
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Help command summary layout */

#define MAX_CMDLEN    12
#define CMDS_PER_LINE 6

#define NUM_CMDS      ((sizeof(g_cmdmap)/sizeof(struct cmdmap_s)) - 1)
#define NUM_CMD_ROWS  ((NUM_CMDS + (CMDS_PER_LINE-1)) / CMDS_PER_LINE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cmdmap_s
{
  const char *cmd;        /* Name of the command */
  cmd_t       handler;    /* Function that handles the command */
  uint8_t     minargs;    /* Minimum number of arguments (including command) */
  uint8_t     maxargs;    /* Maximum number of arguments (including command) */
  const char *usage;      /* Usage instructions for 'help' command */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static int  cmd_help(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
static int  cmd_true(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
static int  cmd_false(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

#ifndef CONFIG_NSH_DISABLE_EXIT
static int  cmd_exit(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);
#endif

static int  cmd_unrecognized(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct cmdmap_s g_cmdmap[] =
{
#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_TEST)
  { "[",        cmd_lbracket, 4, CONFIG_NSH_MAXARGUMENTS, "<expression> ]" },
#endif

#ifndef CONFIG_NSH_DISABLE_HELP
  { "?",        cmd_help,     1, 1, NULL },
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_ADDROUTE)
  { "addroute", cmd_addroute, 4, 4, "<target> <netmask> <router>" },
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_BASE64)
#  ifndef CONFIG_NSH_DISABLE_BASE64DEC
  { "base64dec", cmd_base64decode, 2, 4, "[-w] [-f] <string or filepath>" },
#  endif
#  ifndef CONFIG_NSH_DISABLE_BASE64ENC
  { "base64enc", cmd_base64encode, 2, 4, "[-w] [-f] <string or filepath>" },
#  endif
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_LOOPS)
  { "break",     cmd_break,   1, 1, NULL },
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_CAT
  { "cat",      cmd_cat,      2, CONFIG_NSH_MAXARGUMENTS, "<path> [<path> [<path> ...]]" },
# endif
#ifndef CONFIG_DISABLE_ENVIRON
# ifndef CONFIG_NSH_DISABLE_CD
  { "cd",       cmd_cd,       1, 2, "[<dir-path>|-|~|..]" },
# endif
#endif
# ifndef CONFIG_NSH_DISABLE_CP
  { "cp",       cmd_cp,       3, 3, "<source-path> <dest-path>" },
# endif
# ifndef CONFIG_NSH_DISABLE_CMP
  { "cmp",      cmd_cmp,      3, 3, "<path1> <path2>" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_DATE
  { "date",     cmd_date,     1, 3, "[-s \"MMM DD HH:MM:SS YYYY\"]" },
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_NSH_DISABLE_DD)
  { "dd",       cmd_dd,       3, 6, "if=<infile> of=<outfile> [bs=<sectsize>] [count=<sectors>] [skip=<sectors>]" },
# endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ROUTE) && !defined(CONFIG_NSH_DISABLE_DELROUTE)
  { "delroute", cmd_delroute, 3, 3, "<target> <netmask>" },
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_DISABLE_MOUNTPOINT) && \
    defined(CONFIG_FS_READABLE) && !defined(CONFIG_NSH_DISABLE_DF)
#ifdef CONFIG_NSH_CMDOPT_DF_H
  { "df",       cmd_df,       1, 2, "[-h]" },
#else
  { "df",       cmd_df,       1, 1, NULL },
#endif
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && defined(CONFIG_SYSLOG) && \
    defined(CONFIG_RAMLOG_SYSLOG) && !defined(CONFIG_NSH_DISABLE_DMESG)
  { "dmesg",    cmd_dmesg,    1, 1, NULL },
#endif

#ifndef CONFIG_NSH_DISABLE_ECHO
# ifndef CONFIG_DISABLE_ENVIRON
  { "echo",     cmd_echo,     0, CONFIG_NSH_MAXARGUMENTS, "[<string|$name> [<string|$name>...]]" },
# else
  { "echo",     cmd_echo,     0, CONFIG_NSH_MAXARGUMENTS, "[<string> [<string>...]]" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_EXEC
  { "exec",     cmd_exec,     2, 3, "<hex-address>" },
#endif

#ifndef CONFIG_NSH_DISABLE_EXIT
  { "exit",     cmd_exit,     1, 1, NULL },
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  { "false",     cmd_false,    1, 1, NULL },
#endif

#ifndef CONFIG_NSH_DISABLE_FREE
  { "free",     cmd_free,     1, 1, NULL },
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_GET
  { "get",      cmd_get,      4, 7, "[-b|-n] [-f <local-path>] -h <ip-address> <remote-path>" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_HELP
# ifdef CONFIG_NSH_HELP_TERSE
  { "help",     cmd_help,     1, 2, "[<cmd>]" },
#else
  { "help",     cmd_help,     1, 3, "[-v] [<cmd>]" },
# endif
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0
#ifndef CONFIG_NSH_DISABLE_HEXDUMP
#ifndef CONFIG_NSH_CMDOPT_HEXDUMP
  { "hexdump",  cmd_hexdump,  2, 2, "<file or device>" },
#else
  { "hexdump",  cmd_hexdump,  2, 4, "<file or device> [skip=<bytes>] [count=<bytes>]" },
#endif
#endif
#endif

#ifdef CONFIG_NET
# ifndef CONFIG_NSH_DISABLE_IFCONFIG
  { "ifconfig", cmd_ifconfig, 1, 11, "[nic_name [<ip-address>|dhcp]] [dr|gw|gateway <dr-address>] [netmask <net-mask>] [dns <dns-address>] [hw <hw-mac>]" },
# endif
# ifndef CONFIG_NSH_DISABLE_IFUPDOWN
  { "ifdown",   cmd_ifdown,   2, 2,  "<nic_name>" },
  { "ifup",     cmd_ifup,     2, 2,  "<nic_name>" },
# endif
#endif

#ifndef CONFIG_DISABLE_SIGNALS
# ifndef CONFIG_NSH_DISABLE_KILL
  { "kill",     cmd_kill,     3, 3, "-<signal> <pid>" },
# endif
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_DISABLE_MOUNTPOINT)
# ifndef CONFIG_NSH_DISABLE_LOSETUP
  { "losetup",   cmd_losetup, 3, 6, "[-d <dev-path>] | [[-o <offset>] [-r] <dev-path> <file-path>]" },
# endif
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_LS
  { "ls",       cmd_ls,       1, 5, "[-lRs] <dir-path>" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_MB
  { "mb",       cmd_mb,       2, 3, "<hex-address>[=<hex-value>][ <hex-byte-count>]" },
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_HASH_MD5)
#  ifndef CONFIG_NSH_DISABLE_MD5
  { "md5",      cmd_md5,      2, 3, "[-f] <string or filepath>" },
#  endif
#endif

#ifdef NSH_HAVE_DIROPTS
# ifndef CONFIG_NSH_DISABLE_MKDIR
  { "mkdir",    cmd_mkdir,    2, 2, "<path>" },
# endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && defined(CONFIG_FS_FAT)
# ifndef CONFIG_NSH_DISABLE_MKFATFS
  { "mkfatfs",  cmd_mkfatfs,  2, 4, "[-F <fatsize>] <block-driver>" },
# endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_MKFIFO
  { "mkfifo",   cmd_mkfifo,   2, 2, "<path>" },
# endif
#endif

#ifdef NSH_HAVE_WRITABLE_MOUNTPOINT
# ifndef CONFIG_NSH_DISABLE_MKRD
  { "mkrd",     cmd_mkrd,     2, 6, "[-m <minor>] [-s <sector-size>] <nsectors>" },
# endif
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && defined(CONFIG_FS_SMARTFS)
# ifndef CONFIG_NSH_DISABLE_MKSMARTFS
#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
  { "mksmartfs",  cmd_mksmartfs,  2, 3, "<path> [<num-root-directories>]" },
#else
  { "mksmartfs",  cmd_mksmartfs,  2, 2, "<path>" },
#endif
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_MH
  { "mh",       cmd_mh,       2, 3, "<hex-address>[=<hex-value>][ <hex-byte-count>]" },
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && defined(CONFIG_FS_READABLE)
# ifndef CONFIG_NSH_DISABLE_MOUNT
#if defined(CONFIG_BUILD_PROTECTED) || defined(CONFIG_BUILD_KERNEL)
  { "mount",    cmd_mount,    5, 5, "-t <fstype> [<block-device>] <mount-point>" },
#    else
  { "mount",    cmd_mount,    1, 5, "[-t <fstype> [<block-device>] <mount-point>]" },
#  endif
# endif
#endif

#ifdef NSH_HAVE_DIROPTS
#  ifndef CONFIG_NSH_DISABLE_MV
  { "mv",       cmd_mv,       3, 3, "<old-path> <new-path>" },
#  endif
#endif

#ifndef CONFIG_NSH_DISABLE_MW
  { "mw",       cmd_mw,       2, 3, "<hex-address>[=<hex-value>][ <hex-byte-count>]" },
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && \
    defined(CONFIG_NET) && defined(CONFIG_NFS)
#  ifndef CONFIG_NSH_DISABLE_NFSMOUNT
  { "nfsmount", cmd_nfsmount, 4, 4, "<server-address> <mount-point> <remote-path>" },
#  endif
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING) && !defined(CONFIG_DISABLE_SIGNALS)
# ifndef CONFIG_NSH_DISABLE_PING
  { "ping",     cmd_ping,     2, 6, "[-c <count>] [-i <interval>] <ip-address>" },
# endif
#endif

#if defined(CONFIG_NET) && defined(CONFIG_NET_ICMPv6) && defined(CONFIG_NET_ICMPv6_PING) && !defined(CONFIG_DISABLE_SIGNALS)
# ifndef CONFIG_NSH_DISABLE_PING6
  { "ping6",    cmd_ping6,    2, 6, "[-c <count>] [-i <interval>] <ip-address>" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_PS
  { "ps",       cmd_ps,       1, 1, NULL },
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_PUT
  { "put",      cmd_put,      4, 7, "[-b|-n] [-f <remote-path>] -h <ip-address> <local-path>" },
# endif
#endif

#if CONFIG_NFILE_DESCRIPTORS > 0 && !defined(CONFIG_DISABLE_ENVIRON)
# ifndef CONFIG_NSH_DISABLE_PWD
  { "pwd",      cmd_pwd,      1, 1, NULL },
# endif
#endif

#ifdef NSH_HAVE_DIROPTS
# ifndef CONFIG_NSH_DISABLE_RM
  { "rm",       cmd_rm,       2, 2, "<file-path>" },
# endif
#endif

#ifdef NSH_HAVE_DIROPTS
# ifndef CONFIG_NSH_DISABLE_RMDIR
  { "rmdir",    cmd_rmdir,    2, 2, "<dir-path>" },
# endif
#endif

#ifndef CONFIG_DISABLE_ENVIRON
# ifndef CONFIG_NSH_DISABLE_SET
  { "set",      cmd_set,      3, 3, "<name> <value>" },
# endif
#endif

#if  CONFIG_NFILE_DESCRIPTORS > 0 && CONFIG_NFILE_STREAMS > 0 && !defined(CONFIG_NSH_DISABLESCRIPT)
# ifndef CONFIG_NSH_DISABLE_SH
  { "sh",       cmd_sh,       2, 2, "<script-path>" },
# endif
#endif

#ifndef CONFIG_DISABLE_SIGNALS
# ifndef CONFIG_NSH_DISABLE_SLEEP
  { "sleep",    cmd_sleep,    2, 2, "<sec>" },
# endif
#endif

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_TEST)
  { "test",     cmd_test,     3, CONFIG_NSH_MAXARGUMENTS, "<expression>" },
#endif

#ifndef CONFIG_NSH_DISABLESCRIPT
  { "true",     cmd_true,    1, 1, NULL },
#endif

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && defined(CONFIG_FS_READABLE)
# ifndef CONFIG_NSH_DISABLE_UMOUNT
  { "umount",   cmd_umount,   2, 2, "<dir-path>" },
# endif
#endif

#ifndef CONFIG_DISABLE_ENVIRON
# ifndef CONFIG_NSH_DISABLE_UNSET
  { "unset",    cmd_unset,    2, 2, "<name>" },
# endif
#endif

#if defined(CONFIG_NETUTILS_CODECS) && defined(CONFIG_CODECS_URLCODE)
#  ifndef CONFIG_NSH_DISABLE_URLDECODE
  { "urldecode", cmd_urldecode, 2, 3, "[-f] <string or filepath>" },
#  endif
#  ifndef CONFIG_NSH_DISABLE_URLENCODE
  { "urlencode", cmd_urlencode, 2, 3, "[-f] <string or filepath>" },
#  endif
#endif

#ifndef CONFIG_DISABLE_SIGNALS
# ifndef CONFIG_NSH_DISABLE_USLEEP
  { "usleep",   cmd_usleep,   2, 2, "<usec>" },
# endif
#endif

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
# ifndef CONFIG_NSH_DISABLE_WGET
  { "wget",     cmd_wget,     2, 4, "[-o <local-path>] <url>" },
# endif
#endif

#ifndef CONFIG_NSH_DISABLE_XD
  { "xd",       cmd_xd,       3, 3, "<hex-address> <byte-count>" },
#endif

  { NULL,       NULL,         1, 1, NULL }
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: help_cmdlist
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static inline void help_cmdlist(FAR struct nsh_vtbl_s *vtbl)
{
  unsigned int i;
  unsigned int j;
  unsigned int k;

  /* Print the command name in NUM_CMD_ROWS rows with CMDS_PER_LINE commands
   * on each line.
   */

  for (i = 0; i < NUM_CMD_ROWS; i++)
    {
      nsh_output(vtbl, "  ");
      for (j = 0, k = i; j < CMDS_PER_LINE && k < NUM_CMDS; j++, k += NUM_CMD_ROWS)
        {
#ifdef CONFIG_NOPRINTF_FIELDWIDTH
          nsh_output(vtbl, "%s\t", g_cmdmap[k].cmd);
#else
          nsh_output(vtbl, "%-12s", g_cmdmap[k].cmd);
#endif
        }

      nsh_output(vtbl, "\n");
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
  nsh_output(vtbl, "  [nice [-d <niceness>>]] <cmd> [> <file>|>> <file>] [&]\n\n");
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

  nsh_output(vtbl, g_fmtcmdnotfound, cmd);
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
#ifdef CONFIG_NSH_BUILTIN_APPS
  FAR const char *name;
  int i;

  /* List the set of available built-in commands */

  nsh_output(vtbl, "\nBuiltin Apps:\n");
  for (i = 0; (name = builtin_getname(i)) != NULL; i++)
    {
      nsh_output(vtbl, "  %s\n", name);
    }
#endif
}
#endif

/****************************************************************************
 * Name: cmd_help
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HELP
static int cmd_help(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
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

static int cmd_unrecognized(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  nsh_output(vtbl, g_fmtcmdnotfound, argv[0]);
  return ERROR;
}

/****************************************************************************
 * Name: cmd_true
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLESCRIPT
static int cmd_true(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  return OK;
}

#endif

/****************************************************************************
 * Name: cmd_false
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLESCRIPT
static int cmd_false(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_exit
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_EXIT
static int cmd_exit(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  nsh_exit(vtbl, 0);
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
 *   -1 (ERRROR) if the command was unsuccessful
 *    0 (OK)     if the command was successful
 *
 ****************************************************************************/

int nsh_command(FAR struct nsh_vtbl_s *vtbl, int argc, char *argv[])
{
  const struct cmdmap_s *cmdmap;
  const char            *cmd;
  cmd_t                  handler = cmd_unrecognized;
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

              nsh_output(vtbl, g_fmtargrequired, cmd);
              return ERROR;
            }
          else if (argc > cmdmap->maxargs)
            {
              /* More than the maximum number were provided */

              nsh_output(vtbl, g_fmttoomanyargs, cmd);
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
   return ret;
}
