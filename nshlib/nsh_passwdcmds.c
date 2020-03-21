/****************************************************************************
 * apps/nshlib/nsh_passwdcmds.c
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

#include "fsutils/passwd.h"

#include "nsh.h"
#include "nsh_console.h"

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && \
     defined(CONFIG_NSH_LOGIN_PASSWD) && \
    !defined(CONFIG_FSUTILS_PASSWD_READONLY)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_useradd
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_USERADD
int cmd_useradd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  int ret;

  ret = passwd_adduser(argv[1], argv[2]);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "passwd_adduser",
                NSH_ERRNO_OF(-ret));
      return ERROR;
    }

  return OK;
}
#endif /* !CONFIG_NSH_DISABLE_USERADD */

/****************************************************************************
 * Name: cmd_userdel
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_USERDEL
int cmd_userdel(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  int ret;

  ret = passwd_deluser(argv[1]);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "passwd_deluser",
                 NSH_ERRNO_OF(-ret));
      return ERROR;
    }

  return OK;
}
#endif /* !CONFIG_NSH_DISABLE_USERDEL */

/****************************************************************************
 * Name: cmd_useradd
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_PASSWD
int cmd_passwd(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  int ret;

  ret = passwd_update(argv[1], argv[2]);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "passwd_update",
                 NSH_ERRNO_OF(-ret));
      return ERROR;
    }

  return OK;
}
#endif /* !CONFIG_NSH_DISABLE_USERADD */

#endif /* !CONFIG_DISABLE_MOUNTPOINT &&  CONFIG_NSH_LOGIN_PASSWD &&
        * !CONFIG_FSUTILS_PASSWD_READONLY */
