/****************************************************************************
 * apps/system/sudo/sudo_main.c
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

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <fsutils/passwd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SUDO_PROBE_ARG         "--probe"
#define SUDO_MAX_PASSWORD      256
#define SUDO_SUDOERS_LINE      128

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sudo_read_password
 ****************************************************************************/

static int sudo_read_password(FAR char *password, size_t buflen)
{
  struct termios saved;
  struct termios cfg;
  ssize_t nread;
  int errcode = 0;
  bool restore = false;

  if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved) == 0)
    {
      cfg = saved;
      cfg.c_lflag &= (tcflag_t)~ECHO;
      if (tcsetattr(STDIN_FILENO, TCSANOW, &cfg) == 0)
        {
          restore = true;
        }
    }

  password[0] = '\0';
  nread = read(STDIN_FILENO, password, buflen - 1);
  if (nread < 0)
    {
      errcode = errno;
    }

  if (restore)
    {
      tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    }

  if (nread < 0)
    {
      return -errcode;
    }

  if (nread == 0)
    {
      return -ENODATA;
    }

  password[nread] = '\0';

  if (password[nread - 1] == '\n')
    {
      password[nread - 1] = '\0';
    }

  fputc('\n', stderr);
  return 0;
}

/****************************************************************************
 * Name: sudo_token_eq
 ****************************************************************************/

static bool sudo_token_eq(FAR const char *name, FAR const char *tok,
                          size_t toklen)
{
  return strncmp(name, tok, toklen) == 0 && name[toklen] == '\0';
}

/****************************************************************************
 * Name: sudo_name_in_csv
 ****************************************************************************/

static bool sudo_name_in_csv(FAR const char *name, FAR const char *list)
{
  FAR const char *p = list;
  FAR const char *start;

  if (name == NULL || list == NULL)
    {
      return false;
    }

  while (*p != '\0')
    {
      while (*p == ',' || isspace((unsigned char)*p))
        {
          p++;
        }

      if (*p == '\0')
        {
          break;
        }

      start = p;
      while (*p != '\0' && *p != ',' && !isspace((unsigned char)*p))
        {
          p++;
        }

      if (sudo_token_eq(name, start, p - start))
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: sudo_name_in_file
 ****************************************************************************/

static bool sudo_name_in_file(FAR const char *name, FAR const char *path)
{
  FAR FILE *fp;
  char line[SUDO_SUDOERS_LINE];
  FAR char *tok;
  FAR char *end;

  fp = fopen(path, "r");
  if (fp == NULL)
    {
      return false;
    }

  while (fgets(line, sizeof(line), fp) != NULL)
    {
      tok = line;
      while (*tok != '\0' && isspace((unsigned char)*tok))
        {
          tok++;
        }

      if (*tok == '\0' || *tok == '#')
        {
          continue;
        }

      end = tok;
      while (*end != '\0' && !isspace((unsigned char)*end))
        {
          end++;
        }

      if (sudo_token_eq(name, tok, end - tok))
        {
          fclose(fp);
          return true;
        }
    }

  fclose(fp);
  return false;
}

/****************************************************************************
 * Name: sudo_user_allowed
 *
 * Description:
 *   Real UID 0 may always run sudo.  Other users must appear in the sudoers
 *   file and/or CONFIG_SYSTEM_SUDO_ALLOWED_USERS.
 *
 ****************************************************************************/

static bool sudo_user_allowed(FAR const char *name, uid_t ruid)
{
  if (ruid == 0)
    {
      return true;
    }

  if (sudo_name_in_csv(name, CONFIG_SYSTEM_SUDO_ALLOWED_USERS))
    {
      return true;
    }

  return sudo_name_in_file(name, CONFIG_SYSTEM_SUDO_SUDOERS_PATH);
}

/****************************************************************************
 * Name: sudo_lookup_invoker
 ****************************************************************************/

static int sudo_lookup_invoker(FAR struct passwd *result,
                               FAR char *buf, size_t buflen)
{
  FAR struct passwd *found;
  int ret;

  ret = getpwuid_r(getuid(), result, buf, buflen, &found);
  if (ret != 0)
    {
      return -ret;
    }

  if (found == NULL)
    {
      return -ENOENT;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sudo_main
 *
 * Description:
 *   Linux-style setuid-root helper: the kernel raises effective UID to the
 *   file owner on exec (see nx_uid/nx_mode in the application build).
 *   This program checks the sudoers allowlist, verifies the invoking user's
 *   password, becomes fully root with setresuid/setresgid, then execvp()s
 *   the requested command.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct passwd invoker;
  char pwbuf[CONFIG_LIBC_PASSWD_LINESIZE];
  char password[SUDO_MAX_PASSWORD + 1];
  int ret;

  if (geteuid() != 0)
    {
      fprintf(stderr, "sudo: effective uid is not 0\n");
      return 1;
    }

  if (argc >= 2 && strcmp(argv[1], SUDO_PROBE_ARG) == 0)
    {
      printf("ruid=%d euid=%d\n", getuid(), geteuid());
      return geteuid() == 0 && getuid() != 0 ? 0 : 1;
    }

  if (argc < 2)
    {
      fprintf(stderr, "usage: sudo <command> [args...]\n");
      return 1;
    }

  ret = sudo_lookup_invoker(&invoker, pwbuf, sizeof(pwbuf));
  if (ret < 0)
    {
      fprintf(stderr, "sudo: cannot resolve invoking user: %d\n", -ret);
      return 1;
    }

  if (!sudo_user_allowed(invoker.pw_name, getuid()))
    {
      fprintf(stderr, "sudo: %s is not in the sudoers file\n",
              invoker.pw_name);
      return 1;
    }

  fprintf(stderr, "[sudo] password for %s: ", invoker.pw_name);

  ret = sudo_read_password(password, sizeof(password));
  if (ret < 0)
    {
      fprintf(stderr, "sudo: password read failed: %d\n", -ret);
      return 1;
    }

  ret = passwd_verify(invoker.pw_name, password);
  explicit_bzero(password, sizeof(password));
  if (!PASSWORD_VERIFY_MATCH(ret))
    {
      fprintf(stderr, "sudo: authentication failure\n");
      return 1;
    }

  if (setresuid(0, 0, 0) != 0 || setresgid(0, 0, 0) != 0)
    {
      fprintf(stderr, "sudo: cannot set root identity: %d\n", errno);
      return 1;
    }

  initgroups("root", 0);
  execvp(argv[1], &argv[1]);
  fprintf(stderr, "sudo: exec failed: %d\n", errno);
  return 1;
}
