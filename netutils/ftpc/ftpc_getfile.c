/****************************************************************************
 * apps/netutils/ftpc/ftpc_getfile.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include "ftpc_config.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <debug.h>
#include <errno.h>

#include <apps/ftpc.h>

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

static int ftpc_recvinit(struct ftpc_session_s *session, FAR const char *path,
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

  /* Configure the transfer:  Initial file offset and tranfer mode */

  session->offset   = 0;
  session->filesize = -1;
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
          ndbg("REST command failed: %d\n", errno);
          return ERROR;
        }
 
      session->size     = offset;
      session->rstrsize = offset;
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
      ndbg("RETR command failed: %d\n", errno);
      return ERROR;
    }

  /* Accept a connection on the data socket */

  ret = ftpc_sockaccept(&session->data, "r", FTPC_IS_PASSIVE(session));
  if (ret != OK)
  {
    ndbg("data connection not accepted\n");
  }

  return ret;
}

/****************************************************************************
 * Name: ftpc_waitinput
 *
 * Description:
 *   Wait to receive data.
 *
 ****************************************************************************/

static int ftpc_waitinput(FAR struct ftpc_session_s *session)
{
  int ret;
  do {
    ret = ftpc_waitdata(session, session->data.instream, true);
    if (ret == -1) {
      if (errno == EINTR)
        {
          FTPC_SET_INTERRUPT(session);
        }
      return ERROR;
    }
  } while(ret == 0);

  return OK;
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
  FAR char *buf;
  ssize_t nread;
  ssize_t nwritten;
  int err;
  int ret = OK;

  buf = (FAR char *)malloc(CONFIG_FTP_BUFSIZE);
  if (!buf)
    {
      err = ENOMEM;
      goto errout_with_err;
    }

  for (;;)
    {
      if (ftpc_waitinput(session) != 0)
        {
          nvdbg("ftpc_waitinput() failed\n");
          err = EIO;
          goto errout_with_buf;
        }

      nread = fread(buf, sizeof(char), CONFIG_FTP_BUFSIZE, rinstream);
      if (nread <= 0)
        {
          /* nread == 0 means end of file */

          if (nread < 0)
            {
              (void)ftpc_xfrabort(session, rinstream);
              ret = ERROR;
            }
          break;
        }

      nwritten = fwrite(buf, sizeof(char), nread, loutstream);
      if (nwritten != nread)
        {
          (void)ftpc_xfrabort(session, loutstream);
          ret = ERROR;
          break;
        }

      session->size += nread;
    }

  free(buf);
  return ret;

errout_with_buf:
  free(buf);
errout_with_err:
  set_errno(err);
  return ERROR;
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

int ftpc_getfile(SESSION handle, FAR const char *rname, FAR const char *lname,
                 uint8_t how, uint8_t xfrmode)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  struct stat statbuf;
  FILE *loutstream;
  FAR char *abslpath;
  off_t offset;
  int ret;

  /* Get the full path to the local file */

  abslpath = ftpc_abslpath(session, lname);
  if (!abslpath)
    {
      ndbg("ftpc_abslpath(%s) failed: %d\n", errno);
      goto errout;
    }

  /* Get information about the local file */

  ret = stat(abslpath, &statbuf);
  if (ret == 0)
    {
      /* It already exists.  Is it a directory? */

      if (S_ISDIR(statbuf.st_mode))
        {
          ndbg("'%s' is a directory\n", abslpath);
          goto errout_with_abspath;
        }
    }

  /* Is it write-able? */

#ifdef S_IWRITE
  if (!(statbuf.st_mode & S_IWRITE))
    {
      ndbg("'%s' permission denied\n", abslpath);
      goto errout_with_abspath;
    }
#endif

  /* Are we resuming the transfers?  Is so then the starting offset is the
   * size of the existing, partial file.
   */

  if (how == FTPC_GET_RESUME)
    {
      offset = statbuf.st_size;
    }
  else
    {
      offset = 0;
    }

  /* Setup to receive the file */

  ret = ftpc_recvinit(session, rname, xfrmode, offset);
  if (ret != OK)
    {
      ndbg("ftpc_recvinit failed\n");
      goto errout_with_abspath;
    }

  loutstream = fopen(abslpath, (offset > 0 || (how == FTPC_GET_APPEND)) ? "a" : "w");
  if (!loutstream)
    {
      ndbg("fopen failed: %d\n", errno);
      goto errout_with_abspath;
    }

  /* Save the new local and remote file names */

  free(session->rname);
  free(session->lname);
  session->rname = strdup(rname);
  session->lname = abslpath;

  if (offset > 0)
    {
      ret = fseek(loutstream, offset, SEEK_SET);
      if (ret != OK)
        {
          ndbg("fseek failed: %d\n", errno);
          goto errout_with_outstream;
        }
    }

  /* And receive the new file data */

  if (xfrmode == FTPC_XFRMODE_ASCII)
    {
      ret = ftpc_recvbinary(session, session->data.instream, loutstream);
    }
  else
    {
      ret = ftpc_recvtext(session, session->data.instream, loutstream);
    }

  ftpc_sockclose(&session->data);

  if (ret == 0)
    {
      fptc_getreply(session);
    }

  /* On success, the last rname and lname are retained for debug purpose */

  if (ret == OK && !FTPC_INTERRUPTED(session))
    {
      fclose(loutstream);
      return OK;
    }

  /* Various error exits */

errout_with_outstream:
  fclose(loutstream);
  free(session->rname);
  session->rname = NULL;
  session->lname = NULL;
errout_with_abspath:
  free(abslpath);
  session->offset = 0;
errout:
  return ERROR;
}

/****************************************************************************
 * Name: ftpc_recvbinary
 *
 * Description:
 *   Receive a text file.
 *
 ****************************************************************************/

int ftpc_recvtext(FAR struct ftpc_session_s *session,
                  FAR FILE *rinstream, FAR FILE *loutstream)
{
  char *buf = (char *)malloc(CONFIG_FTP_BUFSIZE);
  int c;
  int ret = OK;

  while((c = fgetc(rinstream)) != EOF) {

    if (ftpc_waitinput(session) != 0)
      {
        break;
      }

    if (c == '\r')
    {
      c = fgetc(rinstream);
      if (c == EOF)
        {
         (void)ftpc_xfrabort(session, rinstream);
          ret = ERROR;
          break;
        }
      if (c != '\n')
        {
          ungetc(c, rinstream);
          c = '\r';
        }
    }

    if (fputc(c, loutstream) == EOF)
      {
        (void)ftpc_xfrabort(session, loutstream);
        ret = ERROR;
        break;
      }

    session->size++;
  }

  free(buf);
  return ret;
}


