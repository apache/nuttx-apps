/****************************************************************************
 * apps/netutils/ftpc/ftpc_getfile.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>

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
 * Name: ftpc_recvinit
 *
 * Description:
 *   Initialize to receive a file
 *
 ****************************************************************************/

static int ftpc_recvinit(struct ftpc_session_s *session,
                         FAR const char *path,
                         uint8_t xfrmode, off_t offset)
{
  int ret;

  /* Reset transfer related variables */

  ftpc_xfrreset(session);

  ret = ftpc_xfrinit(session);
  if (ret != OK)
    {
      return ERROR;
    }

  /* Configure the transfer:  Initial file offset and transfer mode */

  session->offset = 0;
  ftpc_xfrmode(session, xfrmode);

  /* Handle the resume offset (caller is responsible for fseeking in the
   * file)
   */

  if (offset > 0)
    {
      /* Send the REST command.  This command sets the offset where the
       * transfer should start.  This must come after PORT or PASV commands.
       */

      ret = ftpc_cmd(session, "REST %ld", offset);
      if (ret < 0)
        {
          nwarn("WARNING: REST command failed: %d\n", errno);
          return ERROR;
        }

      session->size = offset;
    }

  /* Send the RETR (Retrieve a remote file) command.  Normally the server
   * responds with a mark using code 150:
   *
   * - "150 File status okay; about to open data connection"
   *
   * It then stops accepting new connections, attempts to send the contents
   * of the file over the data connection, and closes the data connection.
   * Finally it either accepts the RETR request with:
   *
   * - "226 Closing data connection" if the entire file was successfully
   *    written to the server's TCP buffers
   *
   * Or rejects the RETR request with:
   *
   * - "425 Can't open data connection" if no TCP connection was established
   * - "426 Connection closed; transfer aborted" if the TCP connection was
   *    established but then broken by the client or by network failure
   * - "451 Requested action aborted: local error in processing" or
   *   "551 Requested action aborted: page type unknown" if the server had
   *   trouble reading the file from disk.
   */

  ret = ftpc_cmd(session, "RETR %s", path);
  if (ret < 0)
    {
      nwarn("WARNING: RETR command failed: %d\n", errno);
      return ERROR;
    }

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
        }
    }

  return ret;
}

/****************************************************************************
 * Name: ftpc_recvbinary
 *
 * Description:
 *   Receive a binary file.
 *
 ****************************************************************************/

static int ftpc_recvbinary(FAR struct ftpc_session_s *session,
                           FAR FILE *rinstream, FAR FILE *loutstream)
{
  ssize_t nread;
  ssize_t nwritten;

  /* Loop until the entire file is received */

  for (; ; )
    {
      /* Read the data from the socket */

      nread = fread(session->buffer, sizeof(char), CONFIG_FTP_BUFSIZE,
                    rinstream);
      if (nread <= 0)
        {
          /* nread < 0 is an error */

          if (nread < 0)
            {
              /* errno should already be set by fread */

              ftpc_xfrabort(session, rinstream);
              return ERROR;
            }

          /* nread == 0 means end of file. Return success */

          return OK;
        }

      /* Write the data to the file */

      nwritten = fwrite(session->buffer, sizeof(char), nread, loutstream);
      if (nwritten != nread)
        {
          ftpc_xfrabort(session, loutstream);

          /* If nwritten < 0 errno should already be set by fwrite.
           * What would a short write mean?
           */

          return ERROR;
        }

      /* Increment the size of the file written */

      session->size += nwritten;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_getfile
 *
 * Description:
 *   Get a file from the remote host.
 *
 ****************************************************************************/

int ftpc_getfile(SESSION handle, FAR const char *rname,
                 FAR const char *lname,
                 uint8_t how, uint8_t xfrmode)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  struct stat statbuf;
  FILE *loutstream;
  FAR char *abslpath;
  off_t offset;
  int ret;

  /* Don't call this with a NULL remote file name */

  DEBUGASSERT(rname);

  /* If the local name is not specified, then it is assumed to the same as
   * the remote file name.
   */

  if (!lname)
    {
      lname = rname;
    }

  /* Get the full path to the local file */

  abslpath = ftpc_abslpath(session, lname);
  if (!abslpath)
    {
      nwarn("WARNING: ftpc_abslpath(%s) failed: %d\n", lname, errno);
      goto errout;
    }

  offset = 0;

  /* Get information about the local file */

  ret = stat(abslpath, &statbuf);
  if (ret == 0)
    {
      /* It already exists.  Is it a directory? */

      if (S_ISDIR(statbuf.st_mode))
        {
          nwarn("WARNING: '%s' is a directory\n", abslpath);
          goto errout_with_abspath;
        }

      /* Is it write-able? */

#ifdef S_IWRITE
      if (!(statbuf.st_mode & S_IWRITE))
        {
          nwarn("WARNING: '%s' permission denied\n", abslpath);
          goto errout_with_abspath;
        }
#endif

      /* Are we resuming the transfers?  Is so then the starting offset is
       * the size of the existing, partial file.
       */

      if (how == FTPC_GET_RESUME)
        {
          offset = statbuf.st_size;
        }
    }

  /* Setup to receive the file */

  ret = ftpc_recvinit(session, rname, xfrmode, offset);
  if (ret != OK)
    {
      nerr("ERROR: ftpc_recvinit failed\n");
      goto errout_with_abspath;
    }

  loutstream = fopen(abslpath,
                     (offset > 0 || (how == FTPC_GET_APPEND)) ? "a" : "w");
  if (!loutstream)
    {
      nerr("ERROR: fopen failed: %d\n", errno);
      goto errout_with_abspath;
    }

  /* If the offset is non-zero, then seek to that offset in the file */

  if (offset > 0)
    {
      ret = fseek(loutstream, offset, SEEK_SET);
      if (ret != OK)
        {
          nerr("ERROR: fseek failed: %d\n", errno);
          goto errout_with_outstream;
        }
    }

  /* And receive the new file data */

  if (xfrmode == FTPC_XFRMODE_ASCII)
    {
      ret = ftpc_recvtext(session, session->data.instream, loutstream);
    }
  else
    {
      ret = ftpc_recvbinary(session, session->data.instream, loutstream);
    }

  ftpc_sockclose(&session->data);

  if (ret == 0)
    {
      fptc_getreply(session);
    }

  /* Check for success */

  if (ret == OK && !FTPC_INTERRUPTED(session))
    {
      fclose(loutstream);
      free(abslpath);
      return OK;
    }

  /* Various error exits */

errout_with_outstream:
  fclose(loutstream);
errout_with_abspath:
  free(abslpath);
  session->offset = 0;
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_recvtext
 *
 * Description:
 *   Receive a text file.
 *
 ****************************************************************************/

int ftpc_recvtext(FAR struct ftpc_session_s *session,
                  FAR FILE *rinstream, FAR FILE *loutstream)
{
  int ch;

  /* Read the next character from the incoming data stream */

  while ((ch = fgetc(rinstream)) != EOF)
    {
      /* Is it a carriage return? Compress \r\n to \n */

      if (ch == '\r')
        {
          /* Get the next character */

          ch = fgetc(rinstream);
          if (ch == EOF)
            {
              /* Ooops... */

              ftpc_xfrabort(session, rinstream);
              return ERROR;
            }

          /* If its not a newline, then keep the carriage return */

          if (ch != '\n')
            {
              ungetc(ch, rinstream);
              ch = '\r';
            }
        }

      /* Then write the character to the output file */

      if (fputc(ch, loutstream) == EOF)
        {
          ftpc_xfrabort(session, loutstream);
          return ERROR;
        }

      /* Increase the actual size of the file by one */

      session->size++;
    }

  return OK;
}
