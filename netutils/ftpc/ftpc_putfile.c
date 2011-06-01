/****************************************************************************
 * apps/netutils/ftpc/ftpc_putfile.c
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

#include <nuttx/config.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <debug.h>

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
 * Name: ftpc_waitoutput
 *
 * Description:
 *   Wait to send data.
 *
 ****************************************************************************/

static int ftpc_waitoutput(FAR struct ftpc_session_s *session)
{
  int ret;

  do
    {
      ret = ftpc_waitdata(session, session->data.outstream, false);
      if (ret < 0)
        {
          return ERROR;
        }
    }
  while(ret == 0);
  return OK;
}

/****************************************************************************
 * Name: ftpc_sendbinary
 *
 * Description:
 *   Send a binary file to the remote host.
 *
 ****************************************************************************/

static int ftpc_sendbinary(FAR struct ftpc_session_s *session,
                           FAR FILE *linstream, FILE *routstream)
{
  FAR char *buf;
  ssize_t nread;
  ssize_t nwritten;
  int ret = OK;

  buf = (char *)malloc(CONFIG_FTP_BUFSIZE);
  while (!feof(linstream))
    {
      nread = fread(buf, sizeof(char), CONFIG_FTP_BUFSIZE, linstream);
    if (nread <= 0)
      {
        (void)ftpc_xfrabort(session, linstream);
        ret = ERROR;
        break;
      }

    if (ftpc_waitoutput(session) != 0)
      {
        ret = ERROR;
        break;
      }

    nwritten = fwrite(buf, sizeof(char), nread, routstream);
    if (nwritten != nread)
      {
        (void)ftpc_xfrabort(session, routstream);
        ret = ERROR;
         break;
      }

    session->size += nread;
  }

  free(buf);
  return ret;
}

/****************************************************************************
 * Name: ftpc_sendtext
 *
 * Description:
 *   Send a text file to the remote host.
 *
 ****************************************************************************/

static int ftpc_sendtext(FAR struct ftpc_session_s *session,
                         FAR FILE *linstream, FAR FILE *routstream)
{
  char *buf = (char *)malloc(CONFIG_FTP_BUFSIZE);
  int c;
  int ret = OK;

  while((c = fgetc(linstream)) != EOF)
    {
      if (ftpc_waitoutput(session) != 0)
        {
          break;
        }

      if (c == '\n')
        {
          if (fputc('\r', routstream) == EOF)
            {
              (void)ftpc_xfrabort(session, routstream);
              ret = ERROR;
              break;
            }

          session->size++;
        }

      if (fputc(c, routstream) == EOF)
        {
          (void)ftpc_xfrabort(session, routstream);
          ret = ERROR;
          break;
        }

      session->size++;
    }

  free(buf);
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
  long offset = session->offset;
  int ret;

  session->offset = 0;

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

  if (offset > 0)
    {
      ret = ftpc_cmd(session, "REST %ld", offset);
      session->size = offset;
      session->rstrsize = offset;
    }

  switch(how)
    {
    case FTPC_PUT_UNIQUE:
      ret = ftpc_cmd(session, "STOU %s", path);

      /* Check for "502 Command not implemented" */

      if (session->code == 502)
        {
          /* The host does not support the STOU command */

          FTPC_CLR_STOU(session);
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

  if (how == FTPC_PUT_UNIQUE)
    {
      /* Determine the remote filename */
 
      char *str = strstr(session->reply, " for ");
      if (str)
        {
          int len;

          str += 5;
          len = strlen(str);
         if (len)
           {
             free(session->lname);
             if (*str == '\'')
               {
                 session->lname = strndup(str+1, len-3);
               }
             else
               {
                 session->lname = strndup(str, len-1);
                 nvdbg("parsed unique filename as '%s'\n",  session->lname);
               }
           }
        }
    }

  ret = ftpc_sockaccept(&session->data, "w", FTPC_IS_PASSIVE(session));
  if (ret != OK)
    {
      ndbg("Data connection not accepted\n");
      return ERROR;
    }

  if (xfrmode == FTPC_XFRMODE_ASCII)
    {
      ret = ftpc_sendbinary(session, stream, session->data.instream);
    }
  else
    {
      ret = ftpc_sendtext(session, stream, session->data.instream);
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
  struct stat statbuf;
  FILE *finstream;
  int ret;

  /* Make sure that the local file exists */

  ret = stat(lname, &statbuf);
  if (ret != OK)
    {
      ndbg("stat() failed: %d\n", errno);
      return ERROR;
    }

  /* Make sure that the local name does not refer to a directory */

  if (S_ISDIR(statbuf.st_mode))
    {
      ndbg("%s is a directory\n", lname);
      return ERROR;
    }

  /* Open the local file for reading */

  finstream = fopen(lname, "r");
  if (!finstream == 0)
    {
      ndbg("fopen() failed: %d\n", errno);
      return ERROR;
    }

  /* Configure for the transfer */

  session->filesize = statbuf.st_size;
  free(session->rname);
  free(session->lname);
  session->rname = strdup(lname);
  session->lname = strdup(rname);

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
          ndbg("Failed to get size of remote file: %s\n", rname);
        }
      else
        {
          /* Seek to the offset in the file corresponding to the size
           * that we have already sent.
           */

          ret = fseek(finstream, session->offset, SEEK_SET);
          if (ret != OK)
            {
              ndbg("fseek failed: %d\n", errnoo);
              fclose(finstream);
              return ERROR;
            }
        }
    }

  ret = ftpc_sendfile(session, rname, finstream, how, xfrmode);
  fclose(finstream);
  return ret;
}
