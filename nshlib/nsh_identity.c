/****************************************************************************
 * apps/nshlib/nsh_identity.c
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

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#ifdef CONFIG_NSH_CLE
#  include "system/cle.h"
#else
#  include "system/readline.h"
#endif

#ifdef CONFIG_NSH_LOGIN_PASSWD
#  include "fsutils/passwd.h"
#endif

#include "nshlib/nshlib.h"
#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_is_privileged
 *
 * Description:
 *   Return true if the caller has root privileges (effective UID or GID is
 *   zero).  NuttX does not support supplementary groups; membership in the
 *   root group is indicated by a primary GID of zero.
 *
 ****************************************************************************/

static bool nsh_is_privileged(void)
{
  return geteuid() == 0 || getegid() == 0;
}

#ifdef CONFIG_LIBC_PASSWD_FILE

/****************************************************************************
 * Name: nsh_lookup_user
 *
 * Description:
 *   Look up a passwd entry by name using re-entrant libc helpers.
 *
 ****************************************************************************/

static int nsh_lookup_user(FAR const char *username, uid_t *uid, gid_t *gid)
{
  struct passwd pwd;
  struct passwd *result;
  char buf[CONFIG_LIBC_PASSWD_LINESIZE];
  int ret;

  ret = getpwnam_r(username, &pwd, buf, sizeof(buf), &result);
  if (ret != 0)
    {
      return -ret;
    }

  if (result == NULL)
    {
      return -ENOENT;
    }

  *uid = result->pw_uid;
  *gid = result->pw_gid;
  return OK;
}
#endif /* CONFIG_LIBC_PASSWD_FILE */

#ifdef CONFIG_NSH_LOGIN

/****************************************************************************
 * Name: nsh_read_password
 ****************************************************************************/

static int nsh_read_password(FAR struct nsh_vtbl_s *vtbl,
                             FAR char *password, size_t buflen)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  struct termios cfg;
  int ret;

  write(OUTFD(pstate), g_passwordprompt, strlen(g_passwordprompt));

  if (isatty(INFD(pstate)))
    {
      if (tcgetattr(INFD(pstate), &cfg) == 0)
        {
          cfg.c_lflag &= ~ECHO;
          tcsetattr(INFD(pstate), TCSANOW, &cfg);
        }
    }

  password[0] = '\0';
  pstate->cn_line[0] = '\0';

#ifdef CONFIG_NSH_CLE
  ret = cle_fd(pstate->cn_line, "", LINE_MAX,
               INFD(pstate), OUTFD(pstate));
#else
  ret = readline_fd(pstate->cn_line, LINE_MAX, INFD(pstate), -1);
#endif

  if (isatty(INFD(pstate)))
    {
      if (tcgetattr(INFD(pstate), &cfg) == 0)
        {
          cfg.c_lflag |= ECHO;
          tcsetattr(INFD(pstate), TCSANOW, &cfg);
        }
    }

  if (ret > 0)
    {
      strlcpy(password, pstate->cn_line, buflen);
      write(OUTFD(pstate), "\n", 1);
      return OK;
    }

  return ERROR;
}

/****************************************************************************
 * Name: nsh_verify_credentials
 ****************************************************************************/

static bool nsh_verify_credentials(FAR const char *username,
                                   FAR const char *password)
{
#ifdef CONFIG_NSH_LOGIN_PASSWD
  return PASSWORD_VERIFY_MATCH(passwd_verify(username, password));
#elif defined(CONFIG_NSH_LOGIN_PLATFORM)
  return PASSWORD_VERIFY_MATCH(platform_user_verify(username, password));
#else
  UNUSED(username);
  UNUSED(password);
  return false;
#endif
}
#endif /* CONFIG_NSH_LOGIN */

/****************************************************************************
 * Name: nsh_switch_credentials
 *
 * Description:
 *   Switch the session to the given UID/GID.  NSH starts with real UID/GID
 *   zero; file permission checks use the effective identity.  When the real
 *   UID is still zero, only the effective UID/GID are changed so that a
 *   later 'su' can regain root via seteuid(0) after password verification.
 *
 ****************************************************************************/

static int nsh_switch_credentials(uid_t uid, gid_t gid)
{
  if (getuid() == 0)
    {
      if (geteuid() != 0 || getegid() != 0)
        {
          if (seteuid(0) != 0 || setegid(0) != 0)
            {
              return -errno;
            }
        }

      if (seteuid(uid) != 0 || setegid(gid) != 0)
        {
          return -errno;
        }

      return OK;
    }

  if (setuid(uid) != 0 || setgid(gid) != 0)
    {
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_setuser_identity
 *
 * Description:
 *   Look up 'username' in the passwd database and set the calling task's
 *   session identity.  When NSH still has real UID zero, only the effective
 *   UID/GID are updated so that 'su' can switch users later.
 *
 * Input Parameters:
 *   username - Login name to assume
 *
 * Returned Value:
 *   OK on success; negated errno on failure.
 *
 ****************************************************************************/

int nsh_setuser_identity(FAR const char *username)
{
#ifdef CONFIG_LIBC_PASSWD_FILE
  FAR struct passwd *pwd;
  uid_t uid;
  gid_t gid;

  /* getpwnam() uses libc static storage; NSH is single-threaded here. */

  pwd = getpwnam(username);
  if (pwd == NULL)
    {
      return -ENOENT;
    }

  uid = pwd->pw_uid;
  gid = pwd->pw_gid;
#else
  uid_t uid;
  gid_t gid;

  if (strcmp(username, "root") != 0)
    {
      return -ENOENT;
    }

  uid = 0;
  gid = 0;
#endif

  return nsh_switch_credentials(uid, gid);
}

/****************************************************************************
 * Name: cmd_su
 *
 * Description:
 *   su [username]
 *
 *   Switch the NSH session to the credentials of 'username'.  Callers with
 *   root privileges (effective UID or GID zero) may switch to any user
 *   without a password.  Other users may switch to their own identity
 *   without a password, or to another user after entering that user's
 *   password.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_SU
int cmd_su(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  char username_buf[48];
  FAR const char *username;
  uid_t target_uid;
  gid_t target_gid;
  int ret;
  bool need_password = false;

  if (argc > 2)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  if (argc == 1)
    {
      username = "root";
    }
  else
    {
      /* argv[1] points into cn_line; copy it before readline reuses that
       * buffer for the password prompt.
       */

      strlcpy(username_buf, argv[1], sizeof(username_buf));
      username = username_buf;
    }

#ifdef CONFIG_LIBC_PASSWD_FILE
  ret = nsh_lookup_user(username, &target_uid, &target_gid);
  if (ret == -ENOENT)
    {
      nsh_error(vtbl, "su: Unknown user '%s'\n", username);
      return ERROR;
    }
  else if (ret < 0)
    {
      nsh_error(vtbl, "su: Permission denied\n");
      return ERROR;
    }
#else
  if (strcmp(username, "root") != 0)
    {
      nsh_error(vtbl, "su: Unknown user '%s'\n", username);
      return ERROR;
    }

  target_uid = 0;
  target_gid = 0;
#endif

  if (!nsh_is_privileged())
    {
#ifdef CONFIG_LIBC_PASSWD_FILE
      if (target_uid != geteuid())
        {
          need_password = true;
        }
#else
      need_password = true;
#endif
    }

#ifdef CONFIG_NSH_LOGIN
  if (need_password)
    {
      char password[LINE_MAX];

      ret = nsh_read_password(vtbl, password, sizeof(password));
      if (ret < 0)
        {
          return ERROR;
        }

      if (!nsh_verify_credentials(username, password))
        {
          nsh_error(vtbl, "su: Authentication failure\n");
          return ERROR;
        }
    }
#else
  if (need_password)
    {
      nsh_error(vtbl, "su: Permission denied\n");
      return ERROR;
    }
#endif

  ret = nsh_switch_credentials(target_uid, target_gid);
  if (ret < 0)
    {
      nsh_error(vtbl, "su: Permission denied\n");
      return ERROR;
    }

  nsh_update_prompt();
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_id
 *
 * Description:
 *   Print real and effective UID/GID for the current session.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_ID
int cmd_id(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  if (argc != 1)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  nsh_output(vtbl, "uid=%d euid=%d gid=%d egid=%d\n",
             getuid(), geteuid(), getgid(), getegid());
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_whoami
 *
 * Description:
 *   Print the user name for the effective UID.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_WHOAMI
int cmd_whoami(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  uid_t euid;

  if (argc != 1)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  euid = geteuid();

#ifdef CONFIG_LIBC_PASSWD_FILE
  FAR struct passwd *pwd;

  pwd = getpwuid(euid);
  if (pwd != NULL && pwd->pw_name != NULL)
    {
      nsh_output(vtbl, "%s\n", pwd->pw_name);
      return OK;
    }
#endif

  if (euid == 0)
    {
      nsh_output(vtbl, "root\n");
    }
  else
    {
      nsh_output(vtbl, "%d\n", (int)euid);
    }

  return OK;
}
#endif
