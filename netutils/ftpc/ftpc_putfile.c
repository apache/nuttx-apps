/****************************************************************************
 * apps/netutils/ftpc/ftpc_putfile.c
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

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "netutils/ftpc.h"

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_sendbinary
 *
 * Description:
 *   Send a binary file to the remote host.
 *
 ****************************************************************************/

#ifdef CONFIG_FTPC_OVER_SENDFILE
static int ftpc_sendbinary(FAR struct ftpc_session_s *session,
                           FAR FILE *linstream)
{
  struct stat stat_buf;
  off_t offset = session->offset;
  ssize_t result;
  ssize_t len;
  int linfd = fileno(linstream);

  if (linfd == -1 || fstat(linfd, &stat_buf) == -1)
    {
      ftpc_xfrabort(session, NULL);
      return ERROR;
    }

  /* Loop until the entire file is sent */

  len = stat_buf.st_size - offset;

  while (len > 0)
    {
      result = sendfile(session->data.sd, linfd, &offset, len);

      if (result == -1 && errno == EAGAIN)
        {
          continue;
        }
      else if (result == -1)
        {
          ftpc_xfrabort(session, NULL);
          return ERROR;
        }

      len -= result;

      /* Increment the size of the file sent */

      session->size += result;
    }

  /* Return success */

  return OK;
}

#else

static int ftpc_sendbinary(FAR struct ftpc_session_s *session,
                           FAR FILE *linstream)
{
  ssize_t nread;
  ssize_t nwritten;
  FILE *routstream = session->data.outstream;

  /* Loop until the entire file is sent */

  for (; ; )
    {
      /* Read data from the file */

      nread = fread(session->buffer, sizeof(char), CONFIG_FTP_BUFSIZE,
                    linstream);
      if (nread <= 0)
        {
          /* nread == 0 is just EOF */

          if (nread < 0)
            {
              ftpc_xfrabort(session, linstream);
              return ERROR;
            }

          /* Return success */

          return OK;
        }

      /* Send the data */

      nwritten = fwrite(session->buffer, sizeof(char), nread, routstream);
      if (nwritten != nread)
        {
          ftpc_xfrabort(session, routstream);

          /* Return failure */

          return ERROR;
        }

      /* Increment the size of the file sent */

      session->size += nread;
    }
}
#endif

/****************************************************************************
 * Name: ftpc_sendtext
 *
 * Description:
 *   Send a text file to the remote host.
 *
 ****************************************************************************/

static int ftpc_sendtext(FAR struct ftpc_session_s *session,
                         FAR FILE *linstream)
{
  int ch;
  int ret = OK;
  FILE *routstream = session->data.outstream;

  /* Write characters one at a time. */

  while ((ch = fgetc(linstream)) != EOF)
    {
      /* If it is a newline, send a carriage return too */

      if (ch == '\n')
        {
          if (fputc('\r', routstream) == EOF)
            {
              ftpc_xfrabort(session, routstream);
              ret = ERROR;
              break;
            }

          /* Increment the size of the file sent */

          session->size++;
        }

      /* Send the character */

      if (fputc(ch, routstream) == EOF)
        {
          ftpc_xfrabort(session, routstream);
          ret = ERROR;
          break;
        }

      /* Increment the size of the file sent */

      session->size++;
    }

  return ret;
}

/****************************************************************************
 * Name: ftpc_sendfile
 *
 * Description:
 *   Send the file to the remote host.
 *
 ****************************************************************************/

static int ftpc_sendfile(struct ftpc_session_s *session, const char *path,
                         FILE *stream, uint8_t how, uint8_t xfrmode)
{
#ifdef CONFIG_DEBUG_FEATURES
  FAR char *rname;
  FAR char *str;
  int len;
#endif
  int ret;

  /* Were we asked to store a file uniquely?  Does the host support the STOU
   * command?
   */

  if (how == FTPC_PUT_UNIQUE && !FTPC_HAS_STOU(session))
    {
      /* We cannot store a file uniquely */

      return ERROR;
    }

  ftpc_xfrreset(session);
  FTPC_SET_PUT(session);

  /* Initialize for the transfer */

  ret = ftpc_xfrinit(session);
  if (ret != OK)
    {
      return ERROR;
    }

  ftpc_xfrmode(session, xfrmode);

  /* The REST command sets the start position in the file.  Some servers
   * allow REST immediately before STOR for binary files.
   */

  if (session->offset > 0)
    {
      ret = ftpc_cmd(session, "REST %ld", session->offset);
      session->size = session->offset;
    }

  /* Send the file using STOR, STOU, or APPE:
   *
   * - STOR request asks the server to receive the contents of a file from
   *   the data connection already established by the client.
   * - APPE is just like STOR except that, if the file already exists, the
   *   server appends the client's data to the file.
   * - STOU is just like STOR except that it asks the server to create a
   *   file under a new pathname selected by the server. If the server
   *   accepts STOU, it provides the pathname in a human-readable format in
   *   the text of its response.
   */

  switch (how)
    {
    case FTPC_PUT_UNIQUE:
      {
        ret = ftpc_cmd(session, "STOU %s", path);

        /* Check for "502 Command not implemented" */

        if (session->code == 502)
          {
            /* The host does not support the STOU command */

            FTPC_CLR_STOU(session);
            return ERROR;
          }

        /* Get the remote filename from the response */

#ifdef CONFIG_DEBUG_FEATURES
        str = strstr(session->reply, " for ");
        if (str)
          {
            str += 5;
            len = strlen(str);
            if (len)
              {
                if (*str == '\'')
                  {
                    rname = strndup(str + 1, len - 3);
                  }
                else
                  {
                    rname = strndup(str, len - 1);
                    ninfo("Unique filename is: %s\n",  rname);
                  }

                free(rname);
              }
          }
#endif
      }
      break;

    case FTPC_PUT_APPEND:
      ret = ftpc_cmd(session, "APPE %s", path);
      break;

    case FTPC_PUT_NORMAL:
    default:
      ret = ftpc_cmd(session, "STOR %s", path);
      break;
  }

  /* If the server is willing to create a new file under that name, or
   * replace an existing file under that name, it responds with a mark
   * using code 150:
   *
   * - "150 File status okay; about to open data connection"
   *
   * It then attempts to read the contents of the file from the data
   * connection, and closes the data connection. Finally it accepts the STOR
   * with:
   *
   * - "226 Closing data connection" if the entire file was successfully
   *    received and stored
   *
   * Or rejects the STOR with:
   *
   * - "425 Can't open data connection" if no TCP connection was established
   * - "426 Connection closed; transfer aborted" if the TCP connection was
   *    established but then broken by the client or by network failure
   * - "451 Requested action aborted: local error in processing",
   *   "452 - Requested action not taken", or "552 Requested file action
   *   aborted" if the server had trouble saving the file to disk.
   *
   * The server may reject the STOR request with:
   *
   * - "450 Requested file action not taken", "452 - Requested action not
   *   taken" or "553 Requested action not taken" without first responding
   *   with a mark.
   */

  /* In active mode, we need to accept a connection on the data socket
   * (in passive mode, we have already connected the data channel to
   * the FTP server).
   */

  if (!FTPC_IS_PASSIVE(session))
    {
      ret = ftpc_sockaccept(&session->dacceptor, &session->data);
      if (ret != OK)
        {
          nerr("ERROR: Data connection not accepted\n");
          return ERROR;
        }
    }

  /* Then perform the data transfer */

  if (xfrmode == FTPC_XFRMODE_ASCII)
    {
      ret = ftpc_sendtext(session, stream);
    }
  else
    {
      ret = ftpc_sendbinary(session, stream);
    }

  ftpc_sockflush(&session->data);
  ftpc_sockclose(&session->data);

  if (ret == 0)
    {
      fptc_getreply(session);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_putfile
 *
 * Description:
 *   Put a file on the remote host.
 *
 ****************************************************************************/

int ftp_putfile(SESSION handle, const char *lname, const char *rname,
                uint8_t how, uint8_t xfrmode)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  FAR char *abslpath;
  struct stat statbuf;
  FILE *finstream;
  int ret;

  /* Don't call this with a NULL local file name */

  DEBUGASSERT(lname);

  /* If the remote name is not specified, then it is assumed to the same as
   * the local file name.
   */

  if (!rname)
    {
      rname = lname;
    }

  /* Get the full path to the local file */

  abslpath = ftpc_abslpath(session, lname);
  if (!abslpath)
    {
      nwarn("WARNING: ftpc_abslpath(%s) failed: %d\n", lname, errno);
      goto errout;
    }

  /* Make sure that the local file exists */

  ret = stat(abslpath, &statbuf);
  if (ret != OK)
    {
      nwarn("WARNING: stat(%s) failed: %d\n", abslpath, errno);
      goto errout_with_abspath;
    }

  /* Make sure that the local name does not refer to a directory */

  if (S_ISDIR(statbuf.st_mode))
    {
      nwarn("WARNING: %s is a directory\n", abslpath);
      goto errout_with_abspath;
    }

  /* Open the local file for reading */

  finstream = fopen(abslpath, "r");
  if (!finstream)
    {
      nwarn("WARNING: fopen() failed: %d\n", errno);
      goto errout_with_abspath;
    }

  /* Are we resuming a transfer? */

  session->offset = 0;
  if (how == FTPC_PUT_RESUME)
    {
      /* Yes... Get the size of the file.  This will only work if the
       * server supports the SIZE command.
       */

      session->offset = ftpc_filesize(session, rname);
      if (session->offset == (off_t)ERROR)
        {
          nwarn("WARNING: Failed to get size of remote file: %s\n", rname);
          goto errout_with_instream;
        }
      else
        {
          /* Seek to the offset in the file corresponding to the size
           * that we have already sent.
           */

          ret = fseek(finstream, session->offset, SEEK_SET);
          if (ret != OK)
            {
              nerr("ERROR: fseek failed: %d\n", errno);
              goto errout_with_instream;
            }
        }
    }

  /* Send the file */

  ret = ftpc_sendfile(session, rname, finstream, how, xfrmode);
  if (ret == OK)
    {
      fclose(finstream);
      free(abslpath);
      return OK;
    }

  /* Various error exits */

errout_with_instream:
  fclose(finstream);
errout_with_abspath:
  free(abslpath);
errout:
  return ERROR;
}
