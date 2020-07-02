/****************************************************************************
 * apps/nshlib/nsh_mntcmds.c
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

#include <sys/types.h>
#include <sys/mount.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <debug.h>
#include <netdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <nuttx/fs/nfs.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_df
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && !defined(CONFIG_NSH_DISABLE_DF)
#ifdef NSH_HAVE_CATFILE
int cmd_df(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
#if defined(HAVE_DF_HUMANREADBLE) && defined(HAVE_DF_BLOCKOUTPUT)
  if (argc > 1 && strcmp(argv[1], "-h") == 0)
#endif
#ifdef HAVE_DF_HUMANREADBLE
    {
      return nsh_catfile(vtbl, argv[0],
                         CONFIG_NSH_PROC_MOUNTPOINT "/fs/usage");
    }
#endif
#if defined(HAVE_DF_HUMANREADBLE) && defined(HAVE_DF_BLOCKOUTPUT)
  else
#endif
#ifdef HAVE_DF_BLOCKOUTPUT
    {
      return nsh_catfile(vtbl, argv[0],
                         CONFIG_NSH_PROC_MOUNTPOINT "/fs/blocks");
    }
#endif
}
#endif
#endif

/****************************************************************************
 * Name: cmd_mount
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && !defined(CONFIG_NSH_DISABLE_MOUNT)
int cmd_mount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR const char *source;
  FAR char *fullsource;
  FAR const char *target;
  FAR char *fulltarget;
  FAR const char *filesystem = NULL;
  FAR const char *options = NULL;
  bool badarg = false;
  int option;
  int ret;

  /* The mount command behaves differently if no parameters are provided. */

#if defined(NSH_HAVE_CATFILE) && defined(HAVE_MOUNT_LIST)
  if (argc < 2)
    {
      return nsh_catfile(vtbl, argv[0],
                         CONFIG_NSH_PROC_MOUNTPOINT "/fs/mount");
    }
#endif

  /* Get the mount options.  NOTE: getopt() is not thread safe nor re-
   * entrant.  To keep its state proper for the next usage, it is necessary
   * to parse to the end of the line even if an error occurs.  If an error
   * occurs, this logic just sets 'badarg' and continues.
   */

  while ((option = getopt(argc, argv, ":o:t:")) != ERROR)
    {
      switch (option)
        {
          case 't':
            filesystem = optarg;
            break;

          case 'o':
            options = optarg;
            break;

          case ':':
            nsh_error(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_error(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered, then return without processing the
   * command.
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There may be one or two required arguments after the options: the source
   * and target paths.  Some file systems do not require the source parameter
   * so if there is only one parameter left, it must be the target.
   */

  if (optind >= argc)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

  source = NULL;
  target = argv[optind];
  optind++;

  if (optind < argc)
    {
      source = target;
      target = argv[optind];
      optind++;

      if (optind < argc)
        {
          nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
          return ERROR;
        }
    }

  /* While the above parsing for the -t argument looks nice, the -t argument
   * not really optional.
   */

  if (!filesystem)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

  /* The source and target paths might be relative to the current
   * working directory.
   */

  fullsource = NULL;
  fulltarget = NULL;

  if (source)
    {
      fullsource = nsh_getfullpath(vtbl, source);
      if (!fullsource)
        {
          return ERROR;
        }
    }

  fulltarget = nsh_getfullpath(vtbl, target);
  if (!fulltarget)
    {
      ret = ERROR;
      goto errout;
    }

  /* Perform the mount */

  ret = mount(fullsource, fulltarget, filesystem, 0, options);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "mount", NSH_ERRNO);
    }

errout:
  if (fullsource)
    {
      nsh_freefullpath(fullsource);
    }

  if (fulltarget)
    {
      nsh_freefullpath(fulltarget);
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_nfsmount
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_NET) && \
    defined(CONFIG_NFS) && !defined(CONFIG_NSH_DISABLE_NFSMOUNT)
int cmd_nfsmount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct nfs_args data;
  FAR char *address;
  FAR char *lpath;
  FAR char *rpath;
  int ret;

  /* The fist argument on the command line should be the NFS server IP
   * address in standard IPv4 (or IPv6) dot format.
   */

  address = argv[1];
  if (!address)
    {
      return ERROR;
    }

  /* Get the remote mount point path */

  rpath = argv[3];

  /* Place all of the NFS arguments into the nfs_args structure */

  memset(&data, 0, sizeof(data));

  /* Convert the IP address string into its binary form */

#ifdef CONFIG_LIBC_NETDB
  if (data.addrlen == 0)
    {
      FAR struct addrinfo *res;
      char serv[16];

      itoa(NFS_PMAPPORT, serv, 10);
      ret = getaddrinfo(address, serv, NULL, &res);
      if (ret == OK)
        {
          data.addrlen = res->ai_addrlen;
          memcpy(&data.addr, res->ai_addr, res->ai_addrlen);
          freeaddrinfo(res);
        }
    }
#endif

#ifdef CONFIG_NET_IPv6
  if (data.addrlen == 0)
    {
      FAR struct sockaddr_in6 *sin;

      sin = (FAR struct sockaddr_in6 *)&data.addr;
      ret = inet_pton(AF_INET6, address, &sin->sin6_addr);
      if (ret == 1)
        {
          sin->sin6_family = AF_INET6;
          sin->sin6_port   = htons(NFS_PMAPPORT);
          data.addrlen     = sizeof(struct sockaddr_in6);
        }
    }
#endif

#ifdef CONFIG_NET_IPv4
  if (data.addrlen == 0)
    {
      FAR struct sockaddr_in *sin;

      sin = (FAR struct sockaddr_in *)&data.addr;
      ret = inet_pton(AF_INET, address, &sin->sin_addr);
      if (ret == 1)
        {
          sin->sin_family = AF_INET;
          sin->sin_port   = htons(NFS_PMAPPORT);
          data.addrlen    = sizeof(struct sockaddr_in);
        }
    }
#endif

  if (data.addrlen == 0)
    {
      return ERROR;
    }

  if (argc > 4 && strcmp(argv[4], "udp") == 0)
    {
      data.sotype         = SOCK_DGRAM;
    }
  else
    {
      data.sotype         = SOCK_STREAM;
    }

  data.path               = rpath;
  data.flags              = 0;       /* 0=Use all defaults */

  /* The local mount point path (lpath) might be relative to the current
   * working directory.
   */

  lpath = nsh_getfullpath(vtbl, argv[2]);
  if (!lpath)
    {
      return ERROR;
    }

  /* Perform the mount */

  ret = mount(NULL, lpath, "nfs", 0, (FAR void *)&data);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "mount", NSH_ERRNO);
    }

  /* We no longer need the allocated mount point path */

  nsh_freefullpath(lpath);
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_umount
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && !defined(CONFIG_NSH_DISABLE_UMOUNT)
int cmd_umount(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  char *fullpath = nsh_getfullpath(vtbl, argv[1]);
  int ret = ERROR;

  if (fullpath)
    {
      /* Perform the umount */

      ret = umount(fullpath);
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "umount", NSH_ERRNO);
        }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
