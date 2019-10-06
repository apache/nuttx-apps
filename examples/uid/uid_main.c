/****************************************************************************
 * examples/uid/uid_main.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

/****************************************************************************
 * Pre-processor definitions
 ****************************************************************************/

#define IOBUFFER_SIZE (196)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, FAR FILE *stream,
                       int exit_code)
{
  fprintf(stream, "USAGE:\n");
  fprintf(stream, "\t%s -uid <uid>    - Show user info by ID\n", progname);
  fprintf(stream, "\t%s -uname <name> - Show user info by name\n", progname);
  fprintf(stream, "\t%s -gid <gid>    - Show group info by ID\n", progname);
  fprintf(stream, "\t%s -gname <name> - Show group info by name\n", progname);
  fprintf(stream, "\t%s -h            - Show this help info\n", progname);
  exit(exit_code);
}

/****************************************************************************
 * show_pwd
 ****************************************************************************/

static void show_pwd(FAR struct passwd *pwd)
{
  printf("Name:   %s\n", pwd->pw_name);
  printf("UID:    %d\n", pwd->pw_uid);
  printf("GID:    %d\n", pwd->pw_gid);
  printf("Home:   %s\n", pwd->pw_dir);
  printf("Shell:  %s\n", pwd->pw_shell);
}

/****************************************************************************
 * show_grp
 ****************************************************************************/

static void show_grp(FAR struct group *grp)
{
  printf("Name:    %s\n", grp->gr_name);
  printf("Passwd:  %s\n", grp->gr_passwd);
  printf("GID:     %d\n", grp->gr_gid);

  printf("Members: ");
  if (grp->gr_mem != NULL)
    {
      bool first = true;
      int i;

      for (i = 0; grp->gr_mem[i] != NULL; i++)
        {
          if (first)
            {
              printf("%s", grp->gr_mem[i]);
              first = false;
            }
          else
            {
              printf(", %s", grp->gr_mem[i]);
            }
        }

      putchar('\n');
    }
}

/****************************************************************************
 * show_user_by_id
 ****************************************************************************/

static void show_user_by_id(uid_t uid)
{
  FAR struct passwd *result;
  struct passwd pwd;
  char buffer[IOBUFFER_SIZE];
  int ret;

  ret = getpwuid_r(uid, &pwd, buffer, IOBUFFER_SIZE, &result);
  if (ret != 0)
    {
      fprintf(stderr, "ERPOR: getpwuid_r failed: %d\n", ret);
    }
  else if (result == NULL)
    {
      fprintf(stderr, "No such user ID: %d\n", uid);
    }
  else
    {
      show_pwd(&pwd);
    }
}

/****************************************************************************
 * show_user_by_name
 ****************************************************************************/

static void show_user_by_name(FAR const char *uname)
{
  FAR struct passwd *result;
  struct passwd pwd;
  char buffer[IOBUFFER_SIZE];
  int ret;

  ret = getpwnam_r(uname, &pwd, buffer, IOBUFFER_SIZE, &result);
  if (ret != 0)
    {
      fprintf(stderr, "ERPOR: getpwnam_r failed: %d\n", ret);
    }
  else if (result == NULL)
    {
      fprintf(stderr, "No such user name: %s\n", uname);
    }
  else
    {
      show_pwd(&pwd);
    }
}

/****************************************************************************
 * show_group_by_id
 ****************************************************************************/

static void show_group_by_id(gid_t gid)
{
  FAR struct group *result;
  struct group grp;
  char buffer[IOBUFFER_SIZE];
  int ret;

  ret = getgrgid_r(gid, &grp, buffer, IOBUFFER_SIZE, &result);
  if (ret != 0)
    {
      fprintf(stderr, "ERPOR: getgrgid_r failed: %d\n", ret);
    }
  else if (result == NULL)
    {
      fprintf(stderr, "No such group ID: %d\n", gid);
    }
  else
    {
      show_grp(&grp);
    }
}

/****************************************************************************
 * show_group_by_name
 ****************************************************************************/

static void show_group_by_name(FAR const char *gname)
{
  FAR struct group *result;
  struct group grp;
  char buffer[IOBUFFER_SIZE];
  int ret;

  ret = getgrnam_r(gname, &grp, buffer, IOBUFFER_SIZE, &result);
  if (ret != 0)
    {
      fprintf(stderr, "ERPOR: getgrnam_r failed: %d\n", ret);
    }
  else if (result == NULL)
    {
      fprintf(stderr, "No such group name: %s\n", gname);
    }
  else
    {
      show_grp(&grp);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * uid_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      fprintf(stderr, "ERROR:  Missing options\n");
      show_usage(argv[0], stderr, EXIT_FAILURE);
    }

  /* Check for help */

  if (strcmp(argv[1], "-h") == 0)
    {
      if (argc != 2)
        {
          fprintf(stderr,
                  "ERROR:  Invalid number of arguments for help: %d\n",
                  argc - 1);
          show_usage(argv[0], stderr, EXIT_FAILURE);
        }

      show_usage(argv[0], stdout, EXIT_SUCCESS);
    }

  if (argc != 3)
    {
      fprintf(stderr, "ERROR:  Invalid number of arguments: %d\n",
              argc - 1);
      show_usage(argv[0], stderr, EXIT_FAILURE);
    }

  if (strcmp(argv[1], "-uid") == 0)
    {
      int uid = atoi(argv[2]);
      show_user_by_id((uid_t)uid);
    }
  else if (strcmp(argv[1], "-uname") == 0)
    {
      show_user_by_name(argv[2]);
    }
  else if (strcmp(argv[1], "-gid") == 0)
    {
      int gid = atoi(argv[2]);
      show_group_by_id((gid_t)gid);
    }
  else if (strcmp(argv[1], "-gname") == 0)
    {
      show_group_by_name(argv[2]);
    }
  else
    {
      fprintf(stderr, "ERROR:  Unrecognized option: %s\n", argv[1]);
      show_usage(argv[0], stderr, EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}
