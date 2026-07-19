/****************************************************************************
 * apps/system/curl/curl_main.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

#include "netutils/webclient.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_SYSTEM_CURL_BUFFERSIZE
#  define CONFIG_SYSTEM_CURL_BUFFERSIZE 512
#endif

/* Limits for user-provided -H headers and -F form fields. */

#define CURL_MAX_HEADERS 12
#define CURL_MAX_FORMS   8

/* Each form field expands to at most 3 body chunks (part header, content,
 * trailing CRLF), plus one final closing-boundary chunk.
 */

#define CURL_MAX_CHUNKS (CURL_MAX_FORMS * 3 + 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* A single piece of the request body. It is either a block of memory
 * (fd < 0, data in "mem") or the contents of an open file (fd >= 0).
 */

struct curl_chunk_s
{
  int        fd;   /* >= 0: read from this file; < 0: use "mem"         */
  FAR char  *mem;  /* Heap buffer owned by this chunk (freed on cleanup) */
  size_t     len;  /* Total number of bytes in this chunk               */
  size_t     off;  /* Bytes already consumed                            */
};

/* The full request body, streamed chunk by chunk. */

struct curl_body_s
{
  struct curl_chunk_s chunks[CURL_MAX_CHUNKS];
  int nchunks;
  int cur;
};

/* Destination for the response body. */

struct curl_sink_s
{
  int fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl_guess_mime
 *
 * Description:
 *   Very small extension-to-MIME map for the common cases. Anything else
 *   defaults to application/octet-stream, exactly like curl's fallback.
 *
 ****************************************************************************/

static FAR const char *curl_guess_mime(FAR const char *name)
{
  FAR const char *dot = strrchr(name, '.');

  if (dot != NULL)
    {
      if (strcasecmp(dot, ".png") == 0)
        {
          return "image/png";
        }

      if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0)
        {
          return "image/jpeg";
        }

      if (strcasecmp(dot, ".gif") == 0)
        {
          return "image/gif";
        }

      if (strcasecmp(dot, ".json") == 0)
        {
          return "application/json";
        }

      if (strcasecmp(dot, ".txt") == 0)
        {
          return "text/plain";
        }
    }

  return "application/octet-stream";
}

/****************************************************************************
 * Name: curl_add_mem_chunk
 *
 * Description:
 *   Append a memory chunk. Takes ownership of "mem" (heap allocated).
 *
 ****************************************************************************/

static int curl_add_mem_chunk(FAR struct curl_body_s *body, FAR char *mem,
                              size_t len)
{
  FAR struct curl_chunk_s *c;

  if (body->nchunks >= CURL_MAX_CHUNKS)
    {
      return -ENOSPC;
    }

  c = &body->chunks[body->nchunks++];
  c->fd  = -1;
  c->mem = mem;
  c->len = len;
  c->off = 0;
  return OK;
}

/****************************************************************************
 * Name: curl_add_file_chunk
 *
 * Description:
 *   Append a file chunk. Takes ownership of the open descriptor "fd".
 *
 ****************************************************************************/

static int curl_add_file_chunk(FAR struct curl_body_s *body, int fd,
                               size_t len)
{
  FAR struct curl_chunk_s *c;

  if (body->nchunks >= CURL_MAX_CHUNKS)
    {
      return -ENOSPC;
    }

  c = &body->chunks[body->nchunks++];
  c->fd  = fd;
  c->mem = NULL;
  c->len = len;
  c->off = 0;
  return OK;
}

/****************************************************************************
 * Name: curl_add_str_chunk
 *
 * Description:
 *   Convenience wrapper: duplicate a string and append it as a memory
 *   chunk.
 *
 ****************************************************************************/

static int curl_add_str_chunk(FAR struct curl_body_s *body,
                              FAR const char *str, size_t len)
{
  FAR char *dup = strdup(str);
  int ret;

  if (dup == NULL)
    {
      return -ENOMEM;
    }

  ret = curl_add_mem_chunk(body, dup, len);
  if (ret < 0)
    {
      free(dup);
    }

  return ret;
}

/****************************************************************************
 * Name: curl_body_cleanup
 ****************************************************************************/

static void curl_body_cleanup(FAR struct curl_body_s *body)
{
  int i;

  for (i = 0; i < body->nchunks; i++)
    {
      FAR struct curl_chunk_s *c = &body->chunks[i];

      if (c->fd >= 0)
        {
          close(c->fd);
        }

      free(c->mem);
    }

  body->nchunks = 0;
}

/****************************************************************************
 * Name: curl_body_total
 ****************************************************************************/

static size_t curl_body_total(FAR const struct curl_body_s *body)
{
  size_t total = 0;
  int i;

  for (i = 0; i < body->nchunks; i++)
    {
      total += body->chunks[i].len;
    }

  return total;
}

/****************************************************************************
 * Name: curl_body_callback
 *
 * Description:
 *   webclient request-body provider. Fills "buffer" with up to *sizep bytes
 *   pulled sequentially from the chunk list. Called repeatedly by webclient
 *   until exactly bodylen bytes have been produced.
 *
 ****************************************************************************/

static int curl_body_callback(FAR void *buffer, FAR size_t *sizep,
                              FAR const void * FAR *datap, size_t reqsize,
                              FAR void *arg)
{
  FAR struct curl_body_s *body = (FAR struct curl_body_s *)arg;
  FAR char *out = (FAR char *)buffer;
  size_t cap = *sizep;
  size_t done = 0;

  (void)reqsize;
  (void)datap;

  while (done < cap && body->cur < body->nchunks)
    {
      FAR struct curl_chunk_s *c = &body->chunks[body->cur];
      size_t remain = c->len - c->off;
      size_t want;

      if (remain == 0)
        {
          body->cur++;
          continue;
        }

      want = cap - done;
      if (want > remain)
        {
          want = remain;
        }

      if (c->fd < 0)
        {
          memcpy(out + done, c->mem + c->off, want);
          c->off += want;
          done   += want;
        }
      else
        {
          ssize_t n = read(c->fd, out + done, want);
          if (n < 0)
            {
              return -errno;
            }

          if (n == 0)
            {
              /* Short file: the pre-computed Content-Length can no longer
               * be honoured, so fail rather than send a truncated body.
               */

              return -EIO;
            }

          c->off += n;
          done   += n;
        }
    }

  *sizep = done;
  return OK;
}

/****************************************************************************
 * Name: curl_sink_callback
 *
 * Description:
 *   webclient response-body consumer. Writes each chunk to the destination
 *   file descriptor (stdout or the -o file).
 *
 ****************************************************************************/

static int curl_sink_callback(FAR char **buffer, int offset, int datend,
                              FAR int *buflen, FAR void *arg)
{
  FAR struct curl_sink_s *sink = (FAR struct curl_sink_s *)arg;
  int len = datend - offset;

  (void)buflen;

  while (len > 0)
    {
      ssize_t n = write(sink->fd, *buffer + offset, len);
      if (n < 0)
        {
          return -errno;
        }

      offset += n;
      len    -= n;
    }

  return OK;
}

/****************************************************************************
 * Name: curl_header_callback
 ****************************************************************************/

static int curl_header_callback(FAR const char *line, bool truncated,
                                FAR void *arg)
{
  (void)truncated;
  (void)arg;

  fprintf(stderr, "< %s\n", line);
  return OK;
}

/****************************************************************************
 * Name: curl_add_form
 *
 * Description:
 *   Parse a single -F "name=value" or -F "name=@path[;type=mime]" spec and
 *   append the corresponding multipart chunks to the body.
 *
 ****************************************************************************/

static int curl_add_form(FAR struct curl_body_s *body,
                         FAR const char *boundary, FAR const char *spec)
{
  char hdr[256];
  FAR const char *eq;
  FAR const char *value;
  FAR char name[64];
  size_t namelen;
  int len;
  int ret;

  eq = strchr(spec, '=');
  if (eq == NULL)
    {
      fprintf(stderr, "curl: bad -F spec '%s' (want name=value)\n", spec);
      return -EINVAL;
    }

  namelen = eq - spec;
  if (namelen >= sizeof(name))
    {
      return -E2BIG;
    }

  memcpy(name, spec, namelen);
  name[namelen] = '\0';
  value = eq + 1;

  if (value[0] == '@')
    {
      /* File upload: name=@path[;type=mime] */

      FAR const char *path = value + 1;
      FAR const char *mime;
      FAR char *typesep;
      char pathbuf[CONFIG_WEBCLIENT_MAXFILENAME];
      struct stat st;
      int fd;

      strlcpy(pathbuf, path, sizeof(pathbuf));
      typesep = strchr(pathbuf, ';');
      if (typesep != NULL && strncmp(typesep, ";type=", 6) == 0)
        {
          *typesep = '\0';
          mime = typesep + 6;
        }
      else
        {
          mime = curl_guess_mime(pathbuf);
        }

      fd = open(pathbuf, O_RDONLY | O_CLOEXEC);
      if (fd < 0)
        {
          fprintf(stderr, "curl: cannot open '%s': %d\n", pathbuf, errno);
          return -errno;
        }

      if (fstat(fd, &st) < 0)
        {
          fprintf(stderr, "curl: stat '%s' failed: %d\n", pathbuf, errno);
          close(fd);
          return -errno;
        }

      len = snprintf(hdr, sizeof(hdr),
                     "--%s\r\n"
                     "Content-Disposition: form-data; name=\"%s\"; "
                     "filename=\"%s\"\r\n"
                     "Content-Type: %s\r\n\r\n",
                     boundary, name, basename(pathbuf), mime);
      if (len < 0 || len >= sizeof(hdr))
        {
          close(fd);
          return -E2BIG;
        }

      ret = curl_add_str_chunk(body, hdr, len);
      if (ret < 0)
        {
          close(fd);
          return ret;
        }

      ret = curl_add_file_chunk(body, fd, st.st_size);
      if (ret < 0)
        {
          close(fd);
          return ret;
        }
    }
  else
    {
      /* Plain text field: name=value */

      len = snprintf(hdr, sizeof(hdr),
                     "--%s\r\n"
                     "Content-Disposition: form-data; name=\"%s\"\r\n\r\n",
                     boundary, name);
      if (len < 0 || len >= sizeof(hdr))
        {
          return -E2BIG;
        }

      ret = curl_add_str_chunk(body, hdr, len);
      if (ret < 0)
        {
          return ret;
        }

      ret = curl_add_str_chunk(body, value, strlen(value));
      if (ret < 0)
        {
          return ret;
        }
    }

  /* Trailing CRLF that terminates this part's content. */

  return curl_add_str_chunk(body, "\r\n", 2);
}

/****************************************************************************
 * Name: curl_usage
 ****************************************************************************/

static void curl_usage(FAR const char *progname)
{
  fprintf(stderr,
    "Usage: %s [options] <url>\n"
    "  -X <method>        HTTP method (default GET, or POST with -d/-F).\n"
    "  -d <data>          Request body. Use -d @file to read from a file.\n"
    "  --data-binary <d>  Like -d but never interprets @-less data.\n"
    "  -F <name=content>  multipart/form-data field. Use name=@file to\n"
    "                     upload a file. Repeatable. Implies POST.\n"
    "  -H <header>        Add a request header. Repeatable.\n"
    "  -o, --output <f>   Write response body to <f> instead of stdout.\n"
    "  -v                 Verbose: print response headers to stderr.\n"
    "  -h                 Show this help.\n"
    "\n"
    "Only http:// URLs are supported (no HTTPS). A scheme-less URL such\n"
    "as host:port/path is treated as http://host:port/path.\n",
    progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *headers[CURL_MAX_HEADERS];
  FAR const char *forms[CURL_MAX_FORMS];
  FAR const char *method  = NULL;
  FAR const char *data    = NULL;
  FAR const char *rawurl  = NULL;
  FAR const char *outfile = NULL;
  struct webclient_context ctx;
  struct curl_body_s body;
  struct curl_sink_s sink;
  char boundary[48];

  /* Sized so the compiler can prove the Content-Type snprintf never
   * truncates: the fixed prefix (including its NUL) plus the largest
   * possible boundary string.
   */

  char ctype[sizeof("Content-Type: multipart/form-data; boundary=") +
             sizeof(boundary)];
  char urlbuf[CONFIG_WEBCLIENT_MAXHOSTNAME + CONFIG_WEBCLIENT_MAXFILENAME +
              16];
  FAR const char *url;
  FAR char *buffer = NULL;
  unsigned int nheaders = 0;
  unsigned int nforms   = 0;
  bool have_ctype = false;
  bool verbose    = false;
  int ret;
  int opt;
  unsigned int i;

  struct option long_options[] =
  {
    {"data",        required_argument, NULL, 'd'},
    {"data-binary", required_argument, NULL, 'B'},
    {"form",        required_argument, NULL, 'F'},
    {"header",      required_argument, NULL, 'H'},
    {"request",     required_argument, NULL, 'X'},
    {"output",      required_argument, NULL, 'o'},
    {"verbose",     no_argument,       NULL, 'v'},
    {"help",        no_argument,       NULL, 'h'},
    {NULL,          0,                 NULL, 0}
  };

  body.nchunks = 0;
  body.cur     = 0;

  while ((opt = getopt_long(argc, argv, "X:d:F:H:o:vh",
                            long_options, NULL)) != ERROR)
    {
      switch (opt)
        {
          case 'X':
            method = optarg;
            break;

          case 'd':
          case 'B':
            data = optarg;
            break;

          case 'F':
            if (nforms >= CURL_MAX_FORMS)
              {
                fprintf(stderr, "curl: too many -F fields (max %d)\n",
                        CURL_MAX_FORMS);
                return EXIT_FAILURE;
              }

            forms[nforms++] = optarg;
            break;

          case 'H':
            if (nheaders >= CURL_MAX_HEADERS - 1)
              {
                fprintf(stderr, "curl: too many -H headers (max %d)\n",
                        CURL_MAX_HEADERS - 1);
                return EXIT_FAILURE;
              }

            if (strncasecmp(optarg, "Content-Type:", 13) == 0)
              {
                have_ctype = true;
              }

            headers[nheaders++] = optarg;
            break;

          case 'o':
            outfile = optarg;
            break;

          case 'v':
            verbose = true;
            break;

          case 'h':
            curl_usage(argv[0]);
            return EXIT_SUCCESS;

          case '?':
          default:
            curl_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if (optind >= argc)
    {
      fprintf(stderr, "curl: missing URL\n");
      curl_usage(argv[0]);
      return EXIT_FAILURE;
    }

  rawurl = argv[optind];

  if (data != NULL && nforms > 0)
    {
      fprintf(stderr, "curl: -d and -F cannot be combined\n");
      return EXIT_FAILURE;
    }

  /* Prepend a default http:// scheme when the URL has none. */

  if (strstr(rawurl, "://") == NULL)
    {
      snprintf(urlbuf, sizeof(urlbuf), "http://%s", rawurl);
      url = urlbuf;
    }
  else
    {
      url = rawurl;
    }

  /* Default method: POST when sending a body, GET otherwise. */

  if (method == NULL)
    {
      method = (data != NULL || nforms > 0) ? "POST" : "GET";
    }

  /* Build the request body. */

  if (nforms > 0)
    {
      /* multipart/form-data: derive a boundary and add the Content-Type
       * header ourselves.
       */

      snprintf(boundary, sizeof(boundary), "----NuttXCurl%08x%08x",
               getpid(), nforms);

      for (i = 0; i < nforms; i++)
        {
          ret = curl_add_form(&body, boundary, forms[i]);
          if (ret < 0)
            {
              goto errout_body;
            }
        }

      /* Closing boundary. */

      ret = curl_add_str_chunk(&body, "--", 2);
      if (ret >= 0)
        {
          ret = curl_add_str_chunk(&body, boundary, strlen(boundary));
        }

      if (ret >= 0)
        {
          ret = curl_add_str_chunk(&body, "--\r\n", 4);
        }

      if (ret < 0)
        {
          goto errout_body;
        }

      if (nheaders < CURL_MAX_HEADERS && !have_ctype)
        {
          snprintf(ctype, sizeof(ctype),
                   "Content-Type: multipart/form-data; boundary=%s",
                   boundary);
          headers[nheaders++] = ctype;
          have_ctype = true;
        }
    }
  else if (data != NULL && data[0] == '@')
    {
      /* -d @file : stream the file as the raw request body. */

      struct stat st;
      int fd = open(data + 1, O_RDONLY | O_CLOEXEC);

      if (fd < 0)
        {
          fprintf(stderr, "curl: cannot open '%s': %d\n", data + 1, errno);
          ret = -errno;
          goto errout_body;
        }

      if (fstat(fd, &st) < 0)
        {
          fprintf(stderr, "curl: stat '%s' failed: %d\n", data + 1, errno);
          ret = -errno;
          close(fd);
          goto errout_body;
        }

      ret = curl_add_file_chunk(&body, fd, st.st_size);
      if (ret < 0)
        {
          close(fd);
          goto errout_body;
        }
    }
  else if (data != NULL)
    {
      /* -d 'literal' : the string itself is the body. */

      ret = curl_add_str_chunk(&body, data, strlen(data));
      if (ret < 0)
        {
          goto errout_body;
        }
    }

  /* curl adds a form Content-Type by default for -d without an explicit
   * one. JSON callers are expected to pass -H "Content-Type: ...".
   */

  if (data != NULL && !have_ctype && nheaders < CURL_MAX_HEADERS)
    {
      headers[nheaders++] =
        "Content-Type: application/x-www-form-urlencoded";
    }

  buffer = malloc(CONFIG_SYSTEM_CURL_BUFFERSIZE);
  if (buffer == NULL)
    {
      fprintf(stderr, "curl: out of memory\n");
      ret = -ENOMEM;
      goto errout_body;
    }

  /* Open the destination for the response body. */

  if (outfile != NULL)
    {
      sink.fd = open(outfile,
                     O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
      if (sink.fd < 0)
        {
          fprintf(stderr, "curl: cannot open '%s': %d\n", outfile, errno);
          ret = -errno;
          goto errout_buffer;
        }
    }
  else
    {
      sink.fd = STDOUT_FILENO;
    }

  /* Assemble and run the request. */

  webclient_set_defaults(&ctx);
  ctx.method            = method;
  ctx.url               = url;
  ctx.buffer            = buffer;
  ctx.buflen            = CONFIG_SYSTEM_CURL_BUFFERSIZE;
  ctx.sink_callback     = curl_sink_callback;
  ctx.sink_callback_arg = &sink;

  if (nheaders > 0)
    {
      ctx.headers  = headers;
      ctx.nheaders = nheaders;
    }

  if (verbose)
    {
      ctx.header_callback = curl_header_callback;

      fprintf(stderr, "> %s %s\n", method, url);
      for (i = 0; i < nheaders; i++)
        {
          fprintf(stderr, "> %s\n", headers[i]);
        }
    }

  if (body.nchunks > 0)
    {
      ctx.body_callback     = curl_body_callback;
      ctx.body_callback_arg = &body;
      ctx.bodylen           = curl_body_total(&body);
    }

  ret = webclient_perform(&ctx);
  if (ret < 0)
    {
      fprintf(stderr, "curl: request failed: %d\n", ret);
    }
  else
    {
      fprintf(stderr, "curl: HTTP %u\n", ctx.http_status);
    }

  if (sink.fd != STDOUT_FILENO)
    {
      close(sink.fd);
    }

errout_buffer:
  free(buffer);

errout_body:
  curl_body_cleanup(&body);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
