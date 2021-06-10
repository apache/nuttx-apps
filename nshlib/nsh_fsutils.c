/****************************************************************************
 * apps/nshlib/nsh_fsutils.c
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_catfile
 *
 * Description:
 *   Dump the contents of a file to the current NSH terminal.
 *
 * Input Paratemets:
 *   vtbl     - session vtbl
 *   cmd      - NSH command name to use in error reporting
 *   filepath - The full path to the file to be dumped
 *
 * Returned Value:
 *   Zero (OK) on success; -1 (ERROR) on failure.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_CATFILE
int nsh_catfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR const char *filepath)
{
  FAR char *buffer;
  int fd;
  int ret = OK;

  /* Open the file for reading */

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
#if defined(CONFIG_NSH_PROC_MOUNTPOINT)
      if (strncmp(filepath, CONFIG_NSH_PROC_MOUNTPOINT,
                  strlen(CONFIG_NSH_PROC_MOUNTPOINT)) == 0)
        {
          nsh_error(vtbl,
                    "nsh: %s: Could not open %s (is procfs mounted?): %d\n",
                    cmd, filepath, NSH_ERRNO);
        }
      else
#endif
        {
          nsh_error(vtbl, g_fmtcmdfailed, cmd, "open", NSH_ERRNO);
        }

      return ERROR;
    }

  buffer = (FAR char *)malloc(IOBUFFERSIZE);
  if (buffer == NULL)
    {
      close(fd);
      nsh_error(vtbl, g_fmtcmdfailed, cmd, "malloc", NSH_ERRNO);
      return ERROR;
    }

  /* And just dump it byte for byte into stdout */

  for (; ; )
    {
      int nbytesread = read(fd, buffer, IOBUFFERSIZE);

      /* Check for read errors */

      if (nbytesread < 0)
        {
          int errval = errno;

          /* EINTR is not an error (but will stop stop the cat) */

          if (errval == EINTR)
            {
              nsh_error(vtbl, g_fmtsignalrecvd, cmd);
            }
          else
            {
              nsh_error(vtbl, g_fmtcmdfailed, cmd, "read",
                        NSH_ERRNO_OF(errval));
            }

          ret = ERROR;
          break;
        }

      /* Check for data successfully read */

      else if (nbytesread > 0)
        {
          int nbyteswritten = 0;

          while (nbyteswritten < nbytesread)
            {
              ssize_t n = nsh_write(vtbl, buffer + nbyteswritten,
                                    nbytesread - nbyteswritten);
              if (n < 0)
                {
                  int errcode = errno;

                  /* EINTR is not an error (but will stop stop the cat) */

                  if (errcode == EINTR)
                    {
                      nsh_error(vtbl, g_fmtsignalrecvd, cmd);
                    }
                  else
                    {
                      nsh_error(vtbl, g_fmtcmdfailed, cmd, "write",
                                 NSH_ERRNO_OF(errcode));
                    }

                  ret = ERROR;
                  break;
                }
              else
                {
                  nbyteswritten += n;
                }
            }
        }

      /* Otherwise, it is the end of file */

      else
        {
          break;
        }
    }

  /* NOTE that the following NSH prompt may appear on the same line as file
   * content.  The IEEE Std requires that "The standard output shall
   * contain the sequence of bytes read from the input files. Nothing else
   * shall be written to the standard output." Reference:
   * https://pubs.opengroup.org/onlinepubs/009695399/utilities/cat.html.
   */

  /* Close the input file and return the result */

  close(fd);
  free(buffer);
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_readfile
 *
 * Description:
 *   Read a small file into a user-provided buffer.  The data is assumed to
 *   be a string and is guaranteed to be NUL-termined.  An error occurs if
 *   the file content (+terminator)  will not fit into the provided 'buffer'.
 *
 * Input Parameters:
 *   vtbl     - The console vtable
 *   filepath - The full path to the file to be read
 *   buffer   - The user-provided buffer into which the file is read.
 *   buflen   - The size of the user provided buffer
 *
 * Returned Value:
 *   Zero (OK) is returned on success; -1 (ERROR) is returned on any
 *   failure to read the fil into the buffer.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_READFILE
int nsh_readfile(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                 FAR const char *filepath, FAR char *buffer, size_t buflen)
{
  FAR char *bufptr;
  size_t remaining;
  ssize_t nread;
  ssize_t ntotal;
  int fd;
  int ret;

  /* Open the file */

  fd = open(filepath, O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, cmd, "open", NSH_ERRNO);
      return ERROR;
    }

  /* Read until we hit the end of the file, until we have exhausted the
   * buffer space, or until some irrecoverable error occurs
   */

  ntotal    = 0;          /* No bytes read yet */
  *buffer   = '\0';       /* NUL terminate the empty buffer */
  bufptr    = buffer;     /* Working pointer */
  remaining = buflen - 1; /* Reserve one byte for a NUL terminator */
  ret       = ERROR;      /* Assume failure */

  do
    {
      nread = read(fd, bufptr, remaining);
      if (nread < 0)
        {
          /* Read error */

          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          /* EINTR is not a read error.  It simply means that a signal was
           * received while waiting for the read to complete.
           */

          if (errcode != EINTR)
            {
              /* Fatal error */

              nsh_error(vtbl, g_fmtcmdfailed, cmd, "read", NSH_ERRNO);
              break;
            }
        }
      else if (nread == 0)
        {
          /* End of file */

          ret = OK;
          break;
        }
      else
        {
          /* Successful read.  Make sure that the buffer is null terminated */

          DEBUGASSERT(nread <= remaining);
          ntotal += nread;
          buffer[ntotal] = '\0';

          /* Bump up the read count and continuing reading to the end of
           * file.
           */

          bufptr    += nread;
          remaining -= nread;
        }
    }
  while (buflen > 0);

  /* Close the file and return. */

  close(fd);
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_foreach_direntry
 *
 * Description:
 *    Call the provided 'handler' for each entry found in the directory at
 *    'dirpath'.
 *
 * Input Parameters
 *   vtbl     - The console vtable
 *   cmd      - NSH command name to use in error reporting
 *   dirpath  - The full path to the directory to be traversed
 *   handler  - The handler to be called for each entry of the directory
 *   pvarg    - User provided argument to be passed to the 'handler'
 *
 * Returned Value:
 *   Zero (OK) returned on success; -1 (ERROR) returned on failure.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_FOREACH_DIRENTRY
int nsh_foreach_direntry(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                         FAR const char *dirpath,
                         nsh_direntry_handler_t handler, void *pvarg)
{
  DIR *dirp;
  int ret = OK;

  /* Open the directory */

  dirp = opendir(dirpath);
  if (dirp == NULL)
    {
      /* Failed to open the directory */

#if defined(CONFIG_NSH_PROC_MOUNTPOINT)
      if (strncmp(dirpath, CONFIG_NSH_PROC_MOUNTPOINT,
                  strlen(CONFIG_NSH_PROC_MOUNTPOINT)) == 0)
        {
          nsh_error(vtbl,
                    "nsh: %s: Could not open %s (is procfs mounted?): %d\n",
                    cmd, dirpath, NSH_ERRNO);
        }
      else
#endif
        {
          nsh_error(vtbl, g_fmtnosuch, cmd, "directory", dirpath);
        }

      return ERROR;
    }

  /* Read each directory entry */

  for (; ; )
    {
      FAR struct dirent *entryp = readdir(dirp);
      if (entryp == NULL)
        {
          /* Finished with this directory */

          break;
        }

      /* Call the handler with this directory entry */

      if (handler(vtbl, dirpath, entryp, pvarg) <  0)
        {
          /* The handler reported a problem */

          ret = ERROR;
          break;
        }
    }

  closedir(dirp);
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_trimdir
 *
 * Description:
 *   Skip any trailing '/' characters (unless it is also the leading '/')
 *
 * Input Parameters:
 *   dirpath - The directory path to be trimmed.  May be modified!
 *
 * Returned value:
 *   None
 *
 ****************************************************************************/

#ifdef NSH_HAVE_TRIMDIR
void nsh_trimdir(FAR char *dirpath)
{
  /* Skip any trailing '/' characters (unless it is also the leading '/') */

  int len = strlen(dirpath) - 1;
  while (len > 0 && dirpath[len] == '/')
    {
      dirpath[len] = '\0';
      len--;
    }
}
#endif

/****************************************************************************
 * Name: nsh_trimspaces
 *
 * Description:
 *   Trim any leading or trailing spaces from a string.
 *
 * Input Parameters:
 *   str - The string to be trimmed.  May be modified!
 *
 * Returned value:
 *   The new string pointer.
 *
 ****************************************************************************/

#ifdef NSH_HAVE_TRIMSPACES
FAR char *nsh_trimspaces(FAR char *str)
{
  FAR char *trimmed;
  int ndx;

  /* Strip leading whitespace from the value */

  for (trimmed = str;
       *trimmed != '\0' && isspace(*trimmed);
       trimmed++);

  /* Strip trailing whitespace from the value */

  for (ndx = strlen(trimmed) - 1;
       ndx >= 0 && isspace(trimmed[ndx]);
       ndx--)
    {
      trimmed[ndx] = '\0';
    }

  return trimmed;
}
#endif
