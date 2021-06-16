/****************************************************************************
 * apps/examples/uid/uid_main.c
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
