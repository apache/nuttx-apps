/****************************************************************************
 * apps/netutils/dropbear/port/nuttx_auth.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fsutils/passwd.h"

#include "dbutil.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct passwd g_dropbear_pw;
static char g_dropbear_name[64];
static char g_dropbear_dir[] = "/";
static char g_dropbear_shell[] = "/bin/sh";
static char g_dropbear_password_marker[] = "x";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR struct passwd *dropbear_fill_pw(FAR const char *name, uid_t uid)
{
  memset(&g_dropbear_pw, 0, sizeof(g_dropbear_pw));

  snprintf(g_dropbear_name, sizeof(g_dropbear_name), "%s", name);

  g_dropbear_pw.pw_uid = uid;
  g_dropbear_pw.pw_gid = 0;
  g_dropbear_pw.pw_name = g_dropbear_name;
  g_dropbear_pw.pw_passwd = g_dropbear_password_marker;
  g_dropbear_pw.pw_gecos = g_dropbear_name;
  g_dropbear_pw.pw_dir = g_dropbear_dir;
  g_dropbear_pw.pw_shell = g_dropbear_shell;

  return &g_dropbear_pw;
}

FAR struct passwd *dropbear_getpwuid(uid_t uid)
{
  return dropbear_fill_pw("root", uid);
}

FAR struct passwd *dropbear_getpwnam(FAR const char *name)
{
  if (name == NULL || name[0] == '\0')
    {
      return NULL;
    }

  return dropbear_fill_pw(name, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uid_t dropbear_getuid(void)
{
  return 0;
}

uid_t dropbear_geteuid(void)
{
  return 0;
}

int dropbear_auth_initialize(void)
{
  dropbear_log(LOG_INFO, "using NuttX passwd auth at %s",
               CONFIG_FSUTILS_PASSWD_PATH);
  return OK;
}

int dropbear_verify_password(FAR const char *username,
                             FAR const char *password)
{
  int ret;

  ret = passwd_verify(username, password);
  if (PASSWORD_VERIFY_MATCH(ret))
    {
      return DROPBEAR_SUCCESS;
    }

  if (PASSWORD_VERIFY_ERROR(ret) && ret != -ENOENT)
    {
      dropbear_log(LOG_WARNING, "passwd_verify failed for '%s': %d",
                   username, ret);
    }

  return DROPBEAR_FAILURE;
}
