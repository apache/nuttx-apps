/****************************************************************************
 * apps/examples/webpanel/cgi_files.c
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
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FILES_DIR "/mnt"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void url_decode(char *dst, const char *src, size_t dstsize);
static const char *get_query_param(const char *qs, const char *key,
                                   char *val, size_t vallen);
static void handle_list(void);
static void handle_delete(const char *name);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: url_decode
 *
 * Description:
 *   Decode a URL-encoded string into a destination buffer.
 *
 * Input Parameters:
 *   dst     - Destination buffer.
 *   src     - Source URL-encoded string.
 *   dstsize - Size of destination buffer.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void url_decode(char *dst, const char *src, size_t dstsize)
{
  size_t i = 0;

  while (*src && i < dstsize - 1)
    {
      if (*src == '%' && src[1] && src[2])
        {
          char hex[3];

          hex[0] = src[1];
          hex[1] = src[2];
          hex[2] = '\0';
          dst[i++] = (char)strtol(hex, NULL, 16);
          src += 3;
        }
      else if (*src == '+')
        {
          dst[i++] = ' ';
          src++;
        }
      else
        {
          dst[i++] = *src++;
        }
    }

  dst[i] = '\0';
}

/****************************************************************************
 * Name: get_query_param
 *
 * Description:
 *   Find and decode a key=value parameter from the QUERY_STRING.
 *
 * Input Parameters:
 *   qs     - Query string.
 *   key    - Parameter name to search for.
 *   val    - Output buffer for decoded value.
 *   vallen - Size of output buffer.
 *
 * Returned Value:
 *   Pointer to val on success; NULL if not found or invalid input.
 *
 ****************************************************************************/

static const char *get_query_param(const char *qs, const char *key,
                                   char *val, size_t vallen)
{
  const char *p;
  size_t klen;

  if (qs == NULL || key == NULL)
    {
      return NULL;
    }

  klen = strlen(key);
  p = qs;

  while (*p)
    {
      if (strncmp(p, key, klen) == 0 && p[klen] == '=')
        {
          const char *start = p + klen + 1;
          const char *end = strchr(start, '&');
          size_t len;

          if (end == NULL)
            {
              end = start + strlen(start);
            }

          len = end - start;
          if (len >= vallen)
            {
              len = vallen - 1;
            }

          url_decode(val, start, len + 1);
          return val;
        }

      p = strchr(p, '&');
      if (p == NULL)
        {
          break;
        }

      p++;
    }

  return NULL;
}

/****************************************************************************
 * Name: handle_list
 *
 * Description:
 *   Emit a JSON response listing files in FILES_DIR.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void handle_list(void)
{
  DIR *dir;
  struct dirent *ent;
  struct stat st;
  char path[128];
  int first = 1;

  puts("Content-type: application/json\r\n"
       "\r\n");

  printf("{\"files\":[");

  dir = opendir(FILES_DIR);
  if (dir != NULL)
    {
      while ((ent = readdir(dir)) != NULL)
        {
          if (ent->d_name[0] == '.')
            {
              continue;
            }

          snprintf(path, sizeof(path), "%s/%s", FILES_DIR, ent->d_name);

          if (!first)
            {
              printf(",");
            }

          first = 0;

          if (stat(path, &st) == 0)
            {
              printf("{\"name\":\"%s\",\"size\":%ld}",
                     ent->d_name, (long)st.st_size);
            }
          else
            {
              printf("{\"name\":\"%s\",\"size\":0}", ent->d_name);
            }
        }

      closedir(dir);
    }

  printf("]}\n");
}

/****************************************************************************
 * Name: handle_delete
 *
 * Description:
 *   Delete the requested file from FILES_DIR and emit a JSON response.
 *
 * Input Parameters:
 *   name - File name to delete.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void handle_delete(const char *name)
{
  char path[128];

  if (name == NULL || name[0] == '\0' || strchr(name, '/') != NULL)
    {
      puts("Content-type: application/json\r\n"
           "Status: 400\r\n"
           "\r\n");
      printf("{\"error\":\"Invalid filename\"}\n");
      return;
    }

  snprintf(path, sizeof(path), "%s/%s", FILES_DIR, name);

  if (unlink(path) == 0)
    {
      puts("Content-type: application/json\r\n"
           "\r\n");
      printf("{\"ok\":true}\n");
    }
  else
    {
      puts("Content-type: application/json\r\n"
           "Status: 404\r\n"
           "\r\n");
      printf("{\"error\":\"File not found\"}\n");
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: files_main
 *
 * Description:
 *   Entry point for file list/delete CGI handling.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Argument vector.
 *
 * Returned Value:
 *   Zero (OK).
 *
 ****************************************************************************/

int files_main(int argc, FAR char *argv[])
{
  const char *qs;
  char action[16];
  char name[64];

  qs = getenv("QUERY_STRING");

  if (qs != NULL && get_query_param(qs, "action", action, sizeof(action)))
    {
      if (strcmp(action, "delete") == 0)
        {
          get_query_param(qs, "name", name, sizeof(name));
          handle_delete(name);
          return 0;
        }
    }

  handle_list();
  return 0;
}
