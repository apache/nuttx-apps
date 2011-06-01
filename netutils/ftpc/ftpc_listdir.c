/****************************************************************************
 * apps/netutils/ftpc/ftpc_listdir.c
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
#include <errno.h>
#include <debug.h>

#include <apps/ftpc.h>

#include "ftpc_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef void (*callback_t)(FAR const char *name, FAR void *arg);

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
 * Name: ftpc_abspath
 *
 * Description:
 *   Get the absolute path to a file, handling tilde expansion.
 *
 ****************************************************************************/

static FAR char *ftpc_abspath(FAR struct ftpc_session_s *session,
                              FAR const char *relpath)
{
  FAR char *ptr = NULL;
  int ret = OK;

  /* If no relative path was provide, then use the current working directory */

  if (!relpath)
    {
      return strdup(session->curdir);
    }

  /* Handle tilde expansion */

  if (relpath[0] == '~')
    {
      /* Is the relative path only '~' */
 
      if (relpath[1] == '\0')
        {
          return strdup(session->homedir);
        }

      /* No... then a '/' better follow the tilde */

      else if (relpath[1] == '/')
        {
          ret = asprintf(&ptr, "%s%s", session->homedir, &relpath[1]);
        }

      /* Hmmm... this prety much guaranteed to fail */

      else
        {
          ptr = strdup(relpath);
        }
    }

  /* No tilde expansion.  Check for a path relative to the current
   * directory.
   */
  
  else if (strncmp(relpath, "./", 2) == 0)
    {
      ret = asprintf(&ptr, "%s%s", session->curdir, relpath+1);
    }

  /* Check for an absolute path */

  else if (relpath[0] == '/' && relpath[1] == ':' && relpath[2] == '\\')
    {
      ptr = strdup(relpath);
    }

  /* Take a wild guess */

  else
    {
      ret = asprintf(&ptr, "%s/%s", session->curdir, relpath);
    }

  return ptr;
}

/****************************************************************************
 * Name: ftpc_dircount
 *
 * Description:
 *   This callback simply counts the number of names in the directory.
 *
 ****************************************************************************/

static void ftpc_dircount(FAR const char *name, FAR void *arg)
{
  FAR unsigned int *dircount = (FAR unsigned int *)arg;
  (*dircount)++;
}

/****************************************************************************
 * Name: ftpc_addname
 *
 * Description:
 *   This callback adds a name to the directory listing.
 *
 ****************************************************************************/

static void ftpc_addname(FAR const char *name, FAR void *arg)
{
  FAR struct ftpc_dirlist_s *dirlist = (FAR struct ftpc_dirlist_s *)arg;
  unsigned int nnames   = dirlist->nnames;
  dirlist->name[nnames] = strdup(name);
  dirlist->nnames       = nnames + 1;
}

/****************************************************************************
 * Name: ftpc_nlstparse
 *
 * Description:
 *   Parse the NLST directory response.  The NLST response consists of a
 *   sequence of pathnames. Each pathname is terminated by \r\n.
 *
 *   If a pathname starts with a slash, it represents the pathname. If a
 *   pathname does not start with a slash, it represents the pathname obtained
 *   by concatenating the pathname of the directory and the pathname.
 *
 *   IF NLST of directory /pub produces foo\r\nbar\r\n, it refers to the
 *   pathnames /pub/foo and /pub/bar.
 *
 ****************************************************************************/

static void ftpc_nlstparse(FAR FILE *instream, callback_t callback,
                           FAR void *arg)
{
  char buffer[CONFIG_FTP_MAXPATH+1];

  /* Read every filename from the temporary file */

  for (;;)
    {
      /* Read the next line from the file */

      if (!fgets(buffer, CONFIG_FTP_MAXPATH, instream))
        {
          break;
        }

      /* Remove any trailing CR-LF from the line */

      ftpc_stripcrlf(buffer);

      /* Check for empty file names */

      if (buffer[0] == '\0')
        {
          break;
        }
      nvdbg("File: %s\n", buffer);

      /* Perform the callback operation */

      callback(buffer, arg);
    }
}

/****************************************************************************
 * Name: ftpc_recvdir
 *
 * Description:
 *   Get the directory listing.
 *
 ****************************************************************************/

static int ftpc_recvdir(FAR struct ftpc_session_s *session,
                        FAR FILE *outstream)
{
  int ret;

  /* Verify that we are still connected to the server */

  if (!ftpc_connected(session))
    {
      ndbg("Not connected to server\n");
      return ERROR;
    }

  /* Setup for the transfer */

  ftpc_xfrreset(session);
  ret = ftpc_xfrinit(session);
  if (ret != OK)
    {
      return ERROR;
    }

  /* Send the "NLST" command */

  ret = ftpc_cmd(session, "NLST");
  if (ret != OK)
    {
      return ERROR;
    }

  /* Accept the connection from the server */

  ret = ftpc_sockaccept(&session->data, "r", FTPC_IS_PASSIVE(session));
  if (ret != OK)
    {
      ndbg("ftpc_sockaccept() failed: %d\n", errno);
      return ERROR;
    }

  /* Receive the NLST response */

  ret = ftpc_recvtext(session, session->data.instream, outstream);
  ftpc_sockclose(&session->data);
  if (ret != OK)
    {
      return ERROR;
    }

  /* Get the server reply */

  fptc_getreply(session);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ftpc_listdir
 *
 * Description:
 *   Get a simple directory listing using NLST:
 *
 *     NLST [<SP> <pathname>] <CRLF>
 *
 *   We could do much, much more here using the LIST or MLST/MLSD commands,
 *   but the parsing is a bitch. See http://cr.yp.to/ftpparse.html
 *
 *   NOTE:  We expect to receive only well structured directory paths. Tilde
 *   expansion "~/xyz" and relative pathes (abc/def) because we do have
 *   special knowledge about the home and current directories.  But otherwise
 *   the pathes are expected to be pre-sanitized:  No . or .. in paths,
 *   no '//' in paths, etc.
 *
 ****************************************************************************/

FAR struct ftpc_dirlist_s *ftpc_listdir(SESSION handle,
                                        FAR const char *dirpath)
{
  FAR struct ftpc_session_s *session = (FAR struct ftpc_session_s *)handle;
  struct ftpc_dirlist_s *dirlist;
  FILE *filestream;
  FAR char *abspath;
  FAR char *tmpfname;
  bool iscurdir;
  unsigned int nnames;
  int allocsize;
  int ret;

  /* Get the absolute path to the directory */

  abspath = ftpc_abspath(session, dirpath);
  ftpc_stripslash(abspath);

  /* Is the directory also the remote current working directory? */

  iscurdir = (strcmp(abspath, session->curdir) == 0);

  /* Create a temporary file to hold the directory listing */

  asprintf(&tmpfname, "%s/TMP%s.dat", CONFIG_FTP_TMPDIR, getpid());
  filestream = fopen(tmpfname, "w+");
  if (!filestream)
    {
      ndbg("Failed to create %s: %d\n", tmpfname, errno);
      free(abspath);
      free(tmpfname);
      return NULL;
    }

  /* "CWD" first so that we get the directory contents, not the
   * directory itself.
   */

  if (!iscurdir)
    {
      ret = ftpc_cmd(session, "CWD %s", abspath);
      if (ret != OK)
        {
          ndbg("CWD to %s failed\n", abspath);
        }
    }

  /* Send the NLST command with no arguments to get the entire contents of
   * the directory.
   */

  ret = ftpc_recvdir(session, filestream);

  /* Go back to the correct current working directory */

  if (!iscurdir)
    {
      int tmpret = ftpc_cmd(session, "CWD %s", session->curdir);
      if (tmpret != OK)
        {
          ndbg("CWD back to to %s failed\n", session->curdir);
        }
    }

  /* Did we successfully receive the directory listing? */

  dirlist = NULL;
  if (ret == OK)
    {
      /* Count the number of names in the temporary file */

      rewind(filestream);
      ftpc_nlstparse(filestream, ftpc_dircount, &nnames);
      if (!nnames)
        {
          ndbg("Nothing found in directory\n");
          goto errout;
        }

      /* Allocate and initialize a directory container */

      allocsize = SIZEOF_FTPC_DIRLIST(nnames);
      dirlist = (struct ftpc_dirlist_s *)malloc(allocsize);
      if (!dirlist)
        {
          ndbg("Failed to allocate dirlist\n");
          goto errout;
        }
      dirlist->nnames = 0;

      /* Then copy all of the directory strings into the container */

      rewind(filestream);
      ftpc_nlstparse(filestream, ftpc_dircount, &nnames);
      DEBUGASSERT(nnames == dirlist->nnames);
    }

errout:
  fclose(filestream);
  free(abspath);
  unlink(tmpfname);
  free(tmpfname);
  return dirlist;
}

/****************************************************************************
 * Name: ftpc_dirfree
 *
 * Description:
 *   Release the allocated directory listing.
 *
 ****************************************************************************/

void ftpc_dirfree(FAR struct ftpc_dirlist_s *dirlist)
{
  int i;

  if (dirlist)
    {
      /* Free each directory name in the directory container */

      for (i = 0; i < dirlist->nnames; i++)
        {
          /* NULL means that the caller stole the string */

          if (dirlist->name[i])
            {
              free(dirlist->name[i]);
            }
        }

      /* Then free the container itself */

      free(dirlist);
    }
}

