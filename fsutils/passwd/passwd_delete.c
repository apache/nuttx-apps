/****************************************************************************
 * apps/fsutils/passwd/passwd_delete.c
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "passwd.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_copyfile
 *
 * Description:
 *  Copy copysize from instream to outstream (or until an error or EOF is
 *  encountered)
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

static int passwd_copyfile(FAR char *iobuffer, FILE *instream,
                           FILE *outstream, size_t copysize)
{
  FAR char *buffer;
  ssize_t nxfrd;
  size_t nwritten;
  size_t nread;
  size_t nbytes;
  size_t gulpsize;
  size_t ncopied;

  /* Copy 'offset' bytes from the instream to the outstream */

  for (ncopied = 0; ncopied < copysize; ncopied += nwritten)
    {
      /* How big of a gulp can we take on this pass through the loop */

      gulpsize = copysize;
      if (gulpsize > CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE)
        {
          gulpsize = CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE;
        }

      /* Read a buffer of data from the instream */

      buffer = iobuffer;
      nbytes = gulpsize;
      nread  = 0;

      do
        {
          nxfrd = fread(buffer, 1, nbytes, instream);
          if (nxfrd < 0)
            {
              int errcode = errno;
              DEBUGASSERT(errcode > 0);

              if (errcode != EINTR)
                {
                  return -errcode;
                }
            }
          else
            {
              nread     += nxfrd;
              buffer    += nxfrd;
              nbytes    -= nxfrd;
            }
        }
      while (nread < gulpsize);

      /* Write the buffer of data to outstream */

      buffer   = iobuffer;
      nbytes   = nread;
      nwritten = 0;

      do
        {
          nxfrd = fwrite(buffer, 1, nbytes, instream);
          if (nxfrd < 0)
            {
              int errcode = errno;
              DEBUGASSERT(errcode > 0);

              if (errcode != EINTR)
                {
                  return -errcode;
                }
            }
          else
            {
              nwritten  += nxfrd;
              buffer    += nxfrd;
              nbytes    -= nxfrd;
            }
        }
      while (nwritten < nread);
      copysize -= nwritten;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: passwd_delete
 *
 * Description:
 *  Delete on record from the password file at offset.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_delete(off_t offset)
{
  FAR char *iobuffer;
  FILE *instream;
  FILE *outstream;
  int ret;

  /* Allocate an I/O buffer for the transfer */

  iobuffer = (FAR char *)malloc(CONFIG_FSUTILS_PASSWD_IOBUFFER_SIZE);
  if (iobuffer == NULL)
    {
      return -ENOMEM;
    }

  /* Rename the /set/password file */

  ret = rename(CONFIG_FSUTILS_PASSWD_PATH, CONFIG_FSUTILS_PASSWD_PATH ".tmp");
  if (ret < 0)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);
      goto errout_with_iobuffer;
    }

  /* Open the renamed file for reading; re-create the /etc/passwd file for
   * writing.
   */

  instream = fopen(CONFIG_FSUTILS_PASSWD_PATH ".tmp", "rt");
  if (instream == NULL)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);
      goto errout_with_iobuffer;
    }

  outstream = fopen(CONFIG_FSUTILS_PASSWD_PATH, "wt");
  if (outstream == NULL)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);
      goto errout_with_instream;
    }

  /* Copy 'offset' bytes from the renamed file to the original file */

  ret = passwd_copyfile(iobuffer, instream, outstream, offset);
  if (ret < 0)
    {
      goto errout_with_outstream;
    }

  /* Now read from the instream and discard the current line */

  for (; ; )
    {
      int ch = fgetc(instream);
      if (ch == EOF)
        {
          if (feof(instream))
            {
              /* Could this really happen without encountering the
               * newline terminator?
               */

              break;
            }
          else
            {
              ret = -errno;
              DEBUGASSERT(ret < 0);
              goto errout_with_instream;
            }
        }
      else if (ch == '\n')
        {
          break;
        }
    }

  /* Copy the rest of the file */

  ret = passwd_copyfile(iobuffer, instream, outstream, SIZE_MAX);

errout_with_outstream:
  (void)fclose(outstream);

errout_with_instream:
  (void)fclose(instream);

errout_with_iobuffer:
  free(iobuffer);
  return ret;
}
