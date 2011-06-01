/****************************************************************************
 * examples/ftpc/ftpc_cmds.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include <stdio.h>
#include <stdlib.h>

#include <apps/ftpc.h>

#include "ftpc.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_login
 ****************************************************************************/

int cmd_login(SESSION handle, int argc, char **argv)
{
  struct ftpc_login_s login = {0, 0, 0, true};

  login.uname = argv[1];
  if (argc > 2)
    {
      login.pwd = argv[2];
    }

  return ftpc_login(handle, &login);
}

/****************************************************************************
 * Name: cmd_quit
 ****************************************************************************/

int cmd_quit(SESSION handle, int argc, char **argv)
{
  int ret = ftpc_quit(handle);
  if (ret < 0)
    {
      printf("quit failed: %d\n", errno);
    }
  printf("Exitting...\n");
  exit(0);
  return ERROR;
}

/****************************************************************************
 * Name: cmd_chdir
 ****************************************************************************/

int cmd_chdir(SESSION handle, int argc, char **argv)
{
  return ftpc_chdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_rpwd
 ****************************************************************************/

int cmd_rpwd(SESSION handle, int argc, char **argv)
{
  FAR char *pwd = ftpc_pwd(handle);
  if (pwd)
    {
      printf("PWD: %s\n", pwd);
      free(pwd);
      return OK;
    }
  return ERROR;
}

/****************************************************************************
 * Name: cmd_cdup
 ****************************************************************************/

int cmd_cdup(SESSION handle, int argc, char **argv)
{
  return ftpc_cdup(handle);
}

/****************************************************************************
 * Name: cmd_mkdir
 ****************************************************************************/

int cmd_mkdir(SESSION handle, int argc, char **argv)
{
  return ftpc_mkdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_rmdir
 ****************************************************************************/

int cmd_rmdir(SESSION handle, int argc, char **argv)
{
  return ftpc_rmdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_unlink
 ****************************************************************************/

int cmd_unlink(SESSION handle, int argc, char **argv)
{
  return ftpc_unlink(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_chmod
 ****************************************************************************/

int cmd_chmod(SESSION handle, int argc, char **argv)
{
  return ftpc_chmod(handle, argv[1], argv[2]);
}

/****************************************************************************
 * Name: cmd_rename
 ****************************************************************************/

int cmd_rename(SESSION handle, int argc, char **argv)
{
  return ftpc_rename(handle, argv[1], argv[2]);
}

/****************************************************************************
 * Name: cmd_size
 ****************************************************************************/

int cmd_size(SESSION handle, int argc, char **argv)
{
  uint64_t size = ftpc_filesize(handle, argv[1]);
  printf("SIZE: %ull\n", size);
  return OK;
}

/****************************************************************************
 * Name: cmd_time
 ****************************************************************************/

int cmd_time(SESSION handle, int argc, char **argv)
{
  time_t filetime = ftpc_filetime(handle, argv[1]);
  printf("TIME: %ul\n", (long)filetime);
  return OK;
}

/****************************************************************************
 * Name: cmd_idle
 ****************************************************************************/

int cmd_idle(SESSION handle, int argc, char **argv)
{
  unsigned int idletime = 0;

  if (argc > 1)
    {
      idletime = atoi(argv[1]);
    }

  return ftpc_idle(handle, idletime);
}

/****************************************************************************
 * Name: cmd_noop
 ****************************************************************************/

int cmd_noop(SESSION handle, int argc, char **argv)
{
  return ftpc_noop(handle);
}

/****************************************************************************
 * Name: cmd_rhelp
 ****************************************************************************/

int cmd_rhelp(SESSION handle, int argc, char **argv)
{
  FAR const char *cmd = NULL;
  int ret;

  if (argc > 1)
    {
      cmd = argv[1];
    }

 ret = ftpc_help(handle, cmd);
 if (ret == OK)
   {
     FAR char *msg = ftpc_response(handle);
     puts(msg);
     free(msg);
   }

 return ret;
}

/****************************************************************************
 * Name: cmd_rls
 ****************************************************************************/

int cmd_rls(SESSION handle, int argc, char **argv)
{
  FAR struct ftpc_dirlist_s *dirlist;
  int i;

  dirlist = ftpc_listdir(handle, argv[1]);
  if (!dirlist)
    {
      return ERROR;
    }

  printf("%s/\n", argv[1]);
  for (i = 0; i < dirlist->nnames; i++)
    {
      printf("  %s\n", dirlist->name[i]);
    }

  ftpc_dirfree(dirlist);
  return OK;
}

/****************************************************************************
 * Name: cmd_get
 ****************************************************************************/

int cmd_get(SESSION handle, int argc, char **argv)
{
  FAR const char *lname = argv[1];

  if (argc > 2)
    {
      lname = argv[2];
    }
  return ftpc_getfile(handle, argv[1], lname, FTPC_GET_NORMAL, FTPC_XFRMODE_ASCII);
}

/****************************************************************************
 * Name: cmd_put
 ****************************************************************************/

int cmd_put(SESSION handle, int argc, char **argv)
{
  FAR const char *rname = argv[1];

  if (argc > 2)
    {
      rname = argv[2];
    }
  return ftp_putfile(handle, argv[1], rname, FTPC_PUT_NORMAL, FTPC_XFRMODE_ASCII);
}
