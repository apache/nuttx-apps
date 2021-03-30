/****************************************************************************
 * apps/netutils/ftpc/ftpc_login.c
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

#include "ftpc_config.h"

#include <string.h>
#include <errno.h>
#include <debug.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_login
 *
 * Description:
 *   Log into the server
 *
 ****************************************************************************/

int ftpc_login(SESSION handle, FAR struct ftpc_login_s *login)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  int errcode;
  int ret;

  /* Verify that we are connected to a server */

  if (!ftpc_connected(session))
    {
      nerr("ERROR: Not connected\n");
      errcode = ENOTCONN;
      goto errout_with_err;
    }

  /* Verify that we are not already logged in to the server */

  if (ftpc_loggedin(session))
    {
      nerr("ERROR: Already logged in\n");
      errcode = EINVAL;
      goto errout_with_err;
    }

  /* Save the login parameter */

  session->uname    = ftpc_dequote(login->uname);
  session->pwd      = ftpc_dequote(login->pwd);
  session->initrdir = ftpc_dequote(login->rdir);

  /* Is passive mode requested? */

  FTPC_CLR_PASSIVE(session);
  if (login->pasv)
    {
      ninfo("Setting passive mode\n");
      FTPC_SET_PASSIVE(session);
    }

  /* The (Re-)login to the server */

  ret = ftpc_relogin(session);
  if (ret != OK)
    {
      nerr("ERROR: login failed: %d\n", errno);
      goto errout;
    }

  return OK;

errout_with_err:
  errno = errcode;
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_relogin
 *
 * Description:
 *   Log in again after a loss of connection
 *
 ****************************************************************************/

int ftpc_relogin(FAR struct ftpc_session_s *session)
{
  int ret;

  /* Log into the server.  First send the USER command.  The server may
   * accept USER with:
   *
   * - "230 User logged in, proceed" meaning that the client has permission
   *    access files under that username
   * - "331 "User name okay, need password" or "332 Need account for login"
   *    meaning that permission might be granted after a PASS request.
   *
   * Or the server may reject USER with:
   *
   * - "530 Not logged in" meaning that the username is unacceptable.
   */

  FTPC_CLR_LOGGEDIN(session);
  ret = ftpc_cmd(session, "USER %s", session->uname);
  if (ret != OK)
    {
      nerr("ERROR: USER %s cmd failed: %d\n", session->uname, errno);
      return ERROR;
    }

  if (session->code == 331)
    {
      /* Send the PASS command with the passed. The server may accept PASS
       * with:
       *
       * - "230 User logged in, proceed" meaning that the client has
       *    permission to access files under that username
       * - "202 Command not implemented, superfluous at this site" meaning
       *    that permission was already granted in response to USER
       * - "332 Need account for login" meaning that permission might be
       *    granted after an ACCT request.
       *
       * The server may reject PASS with:
       *
       * - "503 Bad sequence of commands" if the previous request was not
       *    USER.
       * - "530 - Not logged in" if this username and password are
       *    unacceptable.
       */

      ret = ftpc_cmd(session, "PASS %s", session->pwd);
      if (ret != OK)
        {
          nerr("ERROR: PASS %s cmd failed: %d\n", session->pwd, errno);
          return ret;
        }
    }

  /* We are logged in.. the current working directory on login is our "home"
   * directory.
   */

  FTPC_SET_LOGGEDIN(session);
  session->homerdir = ftpc_rpwd((SESSION)session);
  if (session->homerdir != NULL)
    {
      session->currdir = strdup(session->homerdir);
    }

  /* If the user has requested a special start up directory, then change to
   * that directory now.
   */

  if (session->initrdir)
    {
      ftpc_chdir((SESSION)session, session->initrdir);
    }

  return OK;
}
