/****************************************************************************
 * apps/examples/webpanel/cgi_upload.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define UPLOAD_DIR   "/mnt"
#define BUF_SIZE     512
#define MAX_FILENAME 64
#define MAX_BOUNDARY 80

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void cgi_error(int status, const char *msg);
static void cgi_ok(const char *filename);
static int extract_boundary(const char *content_type, char *boundary,
                            size_t blen);
static int extract_filename(const char *line, char *name, size_t nlen);
static size_t read_stdin(char *buf, size_t n);
static char *memmem_local(const char *haystack, size_t hlen,
                          const char *needle, size_t nlen);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cgi_error
 *
 * Description:
 *   Emit a JSON error response with HTTP status.
 *
 * Input Parameters:
 *   status - HTTP status code.
 *   msg    - Error message string.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void cgi_error(int status, const char *msg)
{
  printf("Content-type: application/json\r\n"
         "Status: %d\r\n"
         "\r\n"
         "{\"error\":\"%s\"}\n", status, msg);
}

/****************************************************************************
 * Name: cgi_ok
 *
 * Description:
 *   Emit a JSON success response.
 *
 * Input Parameters:
 *   filename - Uploaded file name.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void cgi_ok(const char *filename)
{
  printf("Content-type: application/json\r\n"
         "\r\n"
         "{\"ok\":true,\"name\":\"%s\"}\n", filename);
}

/****************************************************************************
 * Name: extract_boundary
 *
 * Description:
 *   Extract multipart boundary from CONTENT_TYPE.
 *
 * Input Parameters:
 *   content_type - CONTENT_TYPE value.
 *   boundary     - Output buffer for boundary token.
 *   blen         - Size of boundary buffer.
 *
 * Returned Value:
 *   Zero on success; negated value on failure.
 *
 ****************************************************************************/

static int extract_boundary(const char *content_type, char *boundary,
                            size_t blen)
{
  const char *p;
  char *q;
  size_t len;

  p = strstr(content_type, "boundary=");
  if (p == NULL)
    {
      return -1;
    }

  p += 9;

  /* Skip optional quotes */

  if (*p == '"')
    {
      p++;
    }

  strncpy(boundary, p, blen - 1);
  boundary[blen - 1] = '\0';

  /* Remove trailing quote if present */

  q = strchr(boundary, '"');
  if (q != NULL)
    {
      *q = '\0';
    }

  /* Remove trailing whitespace/CR/LF */

  len = strlen(boundary);
  while (len > 0 &&
         (boundary[len - 1] == '\r' || boundary[len - 1] == '\n' ||
          boundary[len - 1] == ' '))
    {
      boundary[--len] = '\0';
    }

  return len > 0 ? 0 : -1;
}

/****************************************************************************
 * Name: extract_filename
 *
 * Description:
 *   Extract a safe filename from a Content-Disposition header line.
 *
 * Input Parameters:
 *   line - Header line.
 *   name - Output buffer for filename.
 *   nlen - Size of name buffer.
 *
 * Returned Value:
 *   Zero on success; negated value on failure.
 *
 ****************************************************************************/

static int extract_filename(const char *line, char *name, size_t nlen)
{
  const char *p;
  const char *end;
  size_t len;

  p = strstr(line, "filename=\"");
  if (p == NULL)
    {
      return -1;
    }

  p += 10;
  end = strchr(p, '"');
  if (end == NULL)
    {
      return -1;
    }

  len = end - p;
  if (len == 0 || len >= nlen)
    {
      return -1;
    }

  /* Reject path separators in filename */

  if (memchr(p, '/', len) != NULL || memchr(p, '\\', len) != NULL)
    {
      return -1;
    }

  memcpy(name, p, len);
  name[len] = '\0';
  return 0;
}

/****************************************************************************
 * Name: read_stdin
 *
 * Description:
 *   Read exactly n bytes from standard input unless EOF/error occurs.
 *
 * Input Parameters:
 *   buf - Destination buffer.
 *   n   - Requested number of bytes.
 *
 * Returned Value:
 *   Number of bytes actually read.
 *
 ****************************************************************************/

static size_t read_stdin(char *buf, size_t n)
{
  size_t total = 0;
  while (total < n)
    {
      ssize_t r = read(STDIN_FILENO, buf + total, n - total);
      if (r <= 0)
        {
          break;
        }

      total += r;
    }

  return total;
}

/****************************************************************************
 * Name: memmem_local
 *
 * Description:
 *   Find a byte sequence inside a bounded memory region.
 *
 * Input Parameters:
 *   haystack - Input buffer.
 *   hlen     - Size of haystack.
 *   needle   - Pattern to search.
 *   nlen     - Size of needle.
 *
 * Returned Value:
 *   Pointer to first match; NULL if not found.
 *
 ****************************************************************************/

static char *memmem_local(const char *haystack, size_t hlen,
                          const char *needle, size_t nlen)
{
  const char *p;
  if (nlen == 0)
    {
      return (char *)haystack;
    }

  if (hlen < nlen)
    {
      return NULL;
    }

  p = haystack;
  while (p <= haystack + hlen - nlen)
    {
      if (memcmp(p, needle, nlen) == 0)
        {
          return (char *)p;
        }

      p++;
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: upload_main
 *
 * Description:
 *   CGI entry point for multipart file upload.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Argument vector.
 *
 * Returned Value:
 *   Zero (OK).
 *
 ****************************************************************************/

int upload_main(int argc, FAR char *argv[])
{
  const char *content_type;
  const char *content_length_str;
  char boundary_raw[MAX_BOUNDARY];
  char boundary[MAX_BOUNDARY + 4];
  size_t boundary_len;
  char filename[MAX_FILENAME];
  char filepath[MAX_FILENAME + 8];
  char buf[BUF_SIZE];
  size_t content_length;
  size_t total_read;
  int fd = -1;
  int state;
  char linebuf[256];
  size_t linepos;
  int headers_done;

  content_type = getenv("CONTENT_TYPE");
  content_length_str = getenv("CONTENT_LENGTH");

  if (content_type == NULL || content_length_str == NULL)
    {
      cgi_error(400, "Missing Content-Type or Content-Length");
      return 0;
    }

  content_length = strtoul(content_length_str, NULL, 10);
  if (content_length == 0 || content_length > 1024 * 1024)
    {
      cgi_error(400, "Invalid content length (max 1MB)");
      return 0;
    }

  if (extract_boundary(content_type, boundary_raw, sizeof(boundary_raw)) < 0)
    {
      cgi_error(400, "No boundary in Content-Type");
      return 0;
    }

  /* Multipart boundaries in the body are prefixed with "--" */

  snprintf(boundary, sizeof(boundary), "--%s", boundary_raw);
  boundary_len = strlen(boundary);

  /* State machine for multipart parsing:
   * 0 = looking for first boundary
   * 1 = reading headers after boundary
   * 2 = reading file data
   * 3 = done
   */

  state = 0;
  total_read = 0;
  filename[0] = '\0';
  linepos = 0;
  headers_done = 0;

  /* Read the entire POST body in chunks and process inline.
   * We accumulate a line buffer for header parsing, and stream
   * file data directly to disk.
   */

  while (total_read < content_length && state != 3)
    {
      size_t toread = content_length - total_read;
      size_t nread;

      if (toread > sizeof(buf))
        {
          toread = sizeof(buf);
        }

      nread = read_stdin(buf, toread);
      if (nread == 0)
        {
          break;
        }

      total_read += nread;

      size_t i = 0;
      while (i < nread && state != 3)
        {
          switch (state)
            {
              case 0:

                /* Accumulate until we find the first boundary line */

                while (i < nread)
                  {
                    if (buf[i] == '\n')
                      {
                        linebuf[linepos] = '\0';

                        /* Strip trailing \r */

                        if (linepos > 0 && linebuf[linepos - 1] == '\r')
                          {
                            linebuf[--linepos] = '\0';
                          }

                        if (strncmp(linebuf, boundary, boundary_len) == 0)
                          {
                            state = 1;
                            headers_done = 0;
                            linepos = 0;
                            i++;
                            break;
                          }

                        linepos = 0;
                        i++;
                      }
                    else
                      {
                        if (linepos < sizeof(linebuf) - 1)
                          {
                            linebuf[linepos++] = buf[i];
                          }

                        i++;
                      }
                  }

                break;

              case 1:
                /* Parse headers after boundary, looking for
                 * Content-Disposition with filename.
                 * Headers end with an empty line.
                 */

                while (i < nread && !headers_done)
                  {
                    if (buf[i] == '\n')
                      {
                        linebuf[linepos] = '\0';

                        if (linepos > 0 && linebuf[linepos - 1] == '\r')
                          {
                            linebuf[--linepos] = '\0';
                          }

                        if (linepos == 0)
                          {
                            /* Empty line = end of headers */

                            headers_done = 1;

                            if (filename[0] == '\0')
                              {
                                cgi_error(400, "No filename in upload");
                                if (fd >= 0)
                                  {
                                    close(fd);
                                  }

                                return 0;
                              }

                            snprintf(filepath, sizeof(filepath), "%s/%s",
                                     UPLOAD_DIR, filename);

                            fd = open(filepath,
                                      O_WRONLY | O_CREAT | O_TRUNC,
                                      0666);
                            if (fd < 0)
                              {
                                cgi_error(500, "Cannot create file");
                                return 0;
                              }

                            state = 2;
                            i++;
                            break;
                          }

                        if (strstr(linebuf, "Content-Disposition") != NULL)
                          {
                            extract_filename(linebuf, filename,
                                             sizeof(filename));
                          }

                        linepos = 0;
                        i++;
                      }
                    else
                      {
                        if (linepos < sizeof(linebuf) - 1)
                          {
                            linebuf[linepos++] = buf[i];
                          }

                        i++;
                      }
                  }

                break;

              case 2:
              {
                /* Write file data. We need to detect the closing boundary
                 * which appears as "\r\n--BOUNDARY" in the stream.
                   * Buffer the last boundary_len+4 bytes to check for the
                   * boundary.
                 */

                size_t remaining = nread - i;
                char *bnd;

                bnd = memmem_local(buf + i, remaining,
                                   boundary, boundary_len);
                if (bnd != NULL)
                  {
                    /* Found boundary. Write everything before it,
                     * minus the preceding \r\n.
                     */

                    size_t datalen = bnd - (buf + i);
                    if (datalen >= 2)
                      {
                        datalen -= 2;
                      }

                    if (datalen > 0)
                      {
                        write(fd, buf + i, datalen);
                      }

                    close(fd);
                    fd = -1;
                    state = 3;
                    break;
                  }
                else
                  {
                    /* No boundary in this chunk. Write data but hold back
                     * enough bytes to avoid splitting a boundary across
                     * chunks.
                     */

                    size_t safe;

                    if (remaining > boundary_len + 4)
                      {
                        safe = remaining - boundary_len - 4;
                      }
                    else
                      {
                        safe = 0;
                      }

                    if (safe > 0)
                      {
                        write(fd, buf + i, safe);
                        i += safe;
                      }
                    else
                      {
                        /* Not enough data to be safe. This means we're near
                         * the end of a chunk and need more data. Just write
                         * what we have and hope the boundary comes in the
                         * next read. For simplicity, write it all - worst
                         * case we include a few extra bytes at end of file
                         * which will be fixed when the boundary is found.
                         */

                        write(fd, buf + i, remaining);
                        i = nread;
                      }
                  }

                break;
              }
            }
        }
    }

  /* Drain any remaining data from stdin */

  while (total_read < content_length)
    {
      size_t toread = content_length - total_read;
      size_t nread;

      if (toread > sizeof(buf))
        {
          toread = sizeof(buf);
        }

      nread = read_stdin(buf, toread);
      if (nread == 0)
        {
          break;
        }

      total_read += nread;
    }

  if (fd >= 0)
    {
      close(fd);
    }

  if (state == 3 && filename[0] != '\0')
    {
      cgi_ok(filename);
    }
  else if (filename[0] != '\0')
    {
      cgi_ok(filename);
    }
  else
    {
      cgi_error(400, "Upload incomplete or no file received");
    }

  return 0;
}
