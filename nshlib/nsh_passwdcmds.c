/****************************************************************************
 * apps/nshlib/nsh_passwdcmds.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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

#include "fsutils/passwd.h"

#include "nsh.h"
#include "nsh_console.h"

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && CONFIG_NFILE_DESCRIPTORS > 0 && \
    defined(CONFIG_FS_WRITABLE) && defined(CONFIG_NSH_LOGIN_PASSWD) && \
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
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "passwd_adduser",
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
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "passwd_deluser",
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
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "passwd_update",
                 NSH_ERRNO_OF(-ret));
      return ERROR;
    }

  return OK;
}
#endif /* !CONFIG_NSH_DISABLE_USERADD */

#endif /* !CONFIG_DISABLE_MOUNTPOINT && CONFIG_NFILE_DESCRIPTORS > 0 &&
        * CONFIG_FS_WRITABLE && CONFIG_NSH_LOGIN_PASSWD &&
        * !CONFIG_FSUTILS_PASSWD_READONLY */
