/****************************************************************************
 * apps/examples/ftpc/ftpc_cmds.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "netutils/ftpc.h"

#include "ftpc.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_rlogin
 ****************************************************************************/

int cmd_rlogin(SESSION handle, int argc, char **argv)
{
  struct ftpc_login_s login =
    {
      NULL, NULL, NULL, true
    };

  login.uname = argv[1];
  if (argc > 2)
    {
      login.pwd = argv[2];
    }

  return ftpc_login(handle, &login);
}

/****************************************************************************
 * Name: cmd_rquit
 ****************************************************************************/

int cmd_rquit(SESSION handle, int argc, char **argv)
{
  int ret = ftpc_quit(handle);
  if (ret < 0)
    {
      fprintf(stderr, "quit failed: %d\n", errno);
    }

  printf("Exiting...\n");
  exit(0);
  return ERROR;
}

/****************************************************************************
 * Name: cmd_rchdir
 ****************************************************************************/

int cmd_rchdir(SESSION handle, int argc, char **argv)
{
  return ftpc_chdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_rpwd
 ****************************************************************************/

int cmd_rpwd(SESSION handle, int argc, char **argv)
{
  FAR char *pwd = ftpc_rpwd(handle);
  if (pwd)
    {
      printf("PWD: %s\n", pwd);
      free(pwd);
      return OK;
    }

  return ERROR;
}

/****************************************************************************
 * Name: cmd_rcdup
 ****************************************************************************/

int cmd_rcdup(SESSION handle, int argc, char **argv)
{
  return ftpc_cdup(handle);
}

/****************************************************************************
 * Name: cmd_rmkdir
 ****************************************************************************/

int cmd_rmkdir(SESSION handle, int argc, char **argv)
{
  return ftpc_mkdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_rrmdir
 ****************************************************************************/

int cmd_rrmdir(SESSION handle, int argc, char **argv)
{
  return ftpc_rmdir(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_runlink
 ****************************************************************************/

int cmd_runlink(SESSION handle, int argc, char **argv)
{
  return ftpc_unlink(handle, argv[1]);
}

/****************************************************************************
 * Name: cmd_rchmod
 ****************************************************************************/

int cmd_rchmod(SESSION handle, int argc, char **argv)
{
  return ftpc_chmod(handle, argv[1], argv[2]);
}

/****************************************************************************
 * Name: cmd_rrename
 ****************************************************************************/

int cmd_rrename(SESSION handle, int argc, char **argv)
{
  return ftpc_rename(handle, argv[1], argv[2]);
}

/****************************************************************************
 * Name: cmd_rsize
 ****************************************************************************/

int cmd_rsize(SESSION handle, int argc, char **argv)
{
  off_t size = ftpc_filesize(handle, argv[1]);
  printf("SIZE: %lu\n", (unsigned long)size);
  return OK;
}

/****************************************************************************
 * Name: cmd_rtime
 ****************************************************************************/

int cmd_rtime(SESSION handle, int argc, char **argv)
{
  time_t filetime = ftpc_filetime(handle, argv[1]);
  printf("TIME: %lu\n", (long)filetime);
  return OK;
}

/****************************************************************************
 * Name: cmd_ridle
 ****************************************************************************/

int cmd_ridle(SESSION handle, int argc, char **argv)
{
  unsigned int idletime = 0;

  if (argc > 1)
    {
      idletime = atoi(argv[1]);
    }

  return ftpc_idle(handle, idletime);
}

/****************************************************************************
 * Name: cmd_rnoop
 ****************************************************************************/

int cmd_rnoop(SESSION handle, int argc, char **argv)
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
  FAR char *dirname = NULL;
  int i;

  /* Get the directory listing */

  if (argc > 1)
    {
      dirname = argv[1];
    }

  dirlist = ftpc_listdir(handle, dirname);
  if (!dirlist)
    {
      return ERROR;
    }

  /* Print the directory listing */

  printf("%s/\n", dirname ? dirname : ".");
  for (i = 0; i < dirlist->nnames; i++)
    {
      printf("  %s\n", dirlist->name[i]);
    }

  FFLUSH();

  /* We are responsible for freeing the directory structure allocated by
   * ftpc_listdir().
   */

  ftpc_dirfree(dirlist);
  return OK;
}

/****************************************************************************
 * Name: cmd_rget
 ****************************************************************************/

int cmd_rget(SESSION handle, int argc, char **argv)
{
  FAR const char *rname;
  FAR const char *lname = NULL;
  int xfrmode = FTPC_XFRMODE_ASCII;
  int option;
  bool badarg = false;

  while ((option = getopt(argc, argv, "ab")) != ERROR)
    {
      if (option == 'a')
        {
          xfrmode = FTPC_XFRMODE_ASCII;
        }
      else if (option == 'b')
        {
          xfrmode = FTPC_XFRMODE_BINARY;
        }
      else
        {
          fprintf(stderr, "%s: Unrecognized option: '%c'\n", "rget", option);
          badarg = true;
        }
    }

  if (badarg)
    {
      return ERROR;
    }

  /* There should be one or two parameters remaining on the command line */

  if (optind >= argc)
    {
      fprintf(stderr, "%s: Missing required arguments\n", "rget");
      return ERROR;
    }

  rname = argv[optind];
  optind++;

  if (optind < argc)
    {
      lname = argv[optind];
      optind++;
    }

  if (optind != argc)
    {
      fprintf(stderr, "%s: Too many arguments\n", "rget");
      return ERROR;
    }

  /* Perform the transfer */

  return ftpc_getfile(handle, rname, lname, FTPC_GET_NORMAL, xfrmode);
}

/****************************************************************************
 * Name: cmd_rput
 ****************************************************************************/

int cmd_rput(SESSION handle, int argc, char **argv)
{
  FAR const char *lname;
  FAR const char *rname = NULL;
  int xfrmode = FTPC_XFRMODE_ASCII;
  int option;
  bool badarg = false;

  while ((option = getopt(argc, argv, "ab")) != ERROR)
    {
      if (option == 'a')
        {
          xfrmode = FTPC_XFRMODE_ASCII;
        }
      else if (option == 'b')
        {
          xfrmode = FTPC_XFRMODE_BINARY;
        }
      else
        {
          fprintf(stderr, "%s: Unrecognized option: '%c'\n", "rput", option);
          badarg = true;
        }
    }

  if (badarg)
    {
      return ERROR;
    }

  /* There should be one or two parameters remaining on the command line */

  if (optind >= argc)
    {
      fprintf(stderr, "%s: Missing required arguments\n", "rput");
      return ERROR;
    }

  lname = argv[optind];
  optind++;

  if (optind < argc)
    {
      rname = argv[optind];
      optind++;
    }

  if (optind != argc)
    {
      fprintf(stderr, "%s: Too many arguments\n", "rput");
      return ERROR;
    }

  /* Perform the transfer */

  return ftp_putfile(handle, lname, rname, FTPC_PUT_NORMAL, xfrmode);
}
