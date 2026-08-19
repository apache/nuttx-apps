/****************************************************************************
 * apps/system/nxinit/parser.c
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

#include <ctype.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>

#include "init.h"
#include "parser.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int init_parse_config_lines(FAR const struct parser_s *parser,
                                   FAR const struct parser_s **cur,
                                   FAR size_t *line,
                                   FAR char *buf, FAR size_t *len)
{
  bool create = false;
  FAR char *nl;
  int ret;

  while ((nl = memchr(buf, '\n', *len)))
    {
      *(nl++) = '\0';
      *len -= nl - buf;
      init_debug("Line %-3zu '%s'", ++*line, buf);
      if (*buf == '\0')
        {
          continue;
        }

      /* Skip empty lines and lines containing only whitespace */

      for (ret = 0; buf[ret] && isblank(buf[ret]); ret++);

      if (buf[ret] == '\0')
        {
          memmove(buf, nl, *len);
          continue;
        }

      for (ret = 0; parser[ret].key; ret++)
        {
          if (!strncmp(parser[ret].key, buf, strlen(parser[ret].key)))
            {
              create = true;
              *cur = &parser[ret];
              init_debug("New section (%s)", parser[ret].key);
              break;
            }
        }

      if (*cur == NULL)
        {
          return -EINVAL;
        }

      ret = (*cur)->parse(*cur, create, buf);
      create = false;
      if (ret < 0)
        {
          return ret;
        }

      memmove(buf, nl, *len);
    }

  return 0;
}

static int init_parse_config_buffer(FAR const struct parser_s *parser,
                                    FAR const char *buf, size_t len)
{
  char tmp[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];
  FAR const struct parser_s *cur = NULL;
  size_t line = 0;
  size_t off = 0;
  size_t n = 0;
  size_t r;
  int ret;

  for (; ; )
    {
      r = MIN(len - off, sizeof(tmp));
      memcpy(&tmp[n], &buf[off], r);
      if (r == 0)
        {
          if (n == 0)
            {
              break;
            }

          tmp[n++] = '\n';
        }

      n += r;
      off += r;
      ret = init_parse_config_lines(parser, &cur, &line, tmp, &n);
      if (ret < 0)
        {
          return ret;
        }

      if (n == sizeof(tmp))
        {
          return -E2BIG;
        }
    }

  for (n = 0; parser[n].key; n++)
    {
      if (parser[n].check)
        {
          ret = parser[n].check(&parser[n]);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_parse_arguments(FAR char *buf, bool dup, int argc, FAR char **argv)
{
  bool quote = false;
  bool new = true;
  int i = 0;

  for (; ; )
    {
      while (isblank(*buf))
        {
          if (!quote)
            {
              *buf = '\0';
              new = true;
            }

          buf++;
        }

      if (*buf == '-' && *(buf + 1) == '-'
          && (isblank(*(buf + 2)) || *(buf + 2) == '\0'))
        {
          argv[i++] = buf;
          if (i >= argc || *(buf += 2) == '\0')
            {
              break;
            }

          while (isblank(*buf))
            {
              *buf++ = '\0';
            }

          argv[i++] = buf;
          break;
        }

      if (*buf == '\"')
        {
          *buf = '\0';
          if (quote)
            {
              quote = false;
              buf++;
            }
          else
            {
              quote = true;
              new = true;
              buf++;
            }
        }

      if (*buf == '\0')
        {
          break;
        }

      if (new)
        {
          argv[i++] = buf;
          if (i >= argc)
            {
              break;
            }

          new = false;
        }

      buf++;
    }

  if (dup && i > 0)
    {
      argc = i;
      for (i = 0; i < argc; i++)
        {
          argv[i] = strdup(argv[i]);
          if (!argv[i])
            {
              while (i-- > 0)
                {
                  free(argv[i]);
                }

              return -errno;
            }
        }
    }

  return i;
}

int init_parse_config_file(FAR const struct parser_s *parser,
                           FAR const char *file)
{
  char buf[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX];
  FAR const struct parser_s *cur = NULL;
  size_t line = 0;
  size_t n = 0;
  int ret = 0;
  int fd;

  init_debug("Parsing %s", file);

  fd = open(file, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      init_err("Opening %s %d", file, errno);
      return -errno;
    }

  for (; ; )
    {
      ssize_t r = read(fd, &buf[n], sizeof(buf) - n);
      if (r < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          ret = -errno;
          goto out;
        }
      else if (r == 0)
        {
          if (n == 0)
            {
              break;
            }

          buf[n++] = '\n';
        }

      n += r;
      ret = init_parse_config_lines(parser, &cur, &line, buf, &n);
      if (ret < 0)
        {
          goto out;
        }

      if (n == sizeof(buf))
        {
          ret = -E2BIG;
          goto out;
        }
    }

  for (n = 0; parser[n].key; n++)
    {
      if (parser[n].check)
        {
          ret = parser[n].check(&parser[n]);
          if (ret < 0)
            {
              break;
            }
        }
    }

out:
  close(fd);
  if (ret < 0)
    {
      init_err("Parse %s %d", file, ret);
    }

  return ret;
}

int init_parse_configs(FAR const struct parser_s *parser)
{
  static const char preset[] =
    "on boot\n"
    "   trigger init\n"
#if defined(CONFIG_NETUTILS_NETINIT) || defined(CONFIG_SYSTEM_NXINIT_FINALINIT)
    "on init\n"
#ifdef CONFIG_NETUTILS_NETINIT
    "   trigger netinit\n"
#endif
#ifdef CONFIG_SYSTEM_NXINIT_FINALINIT
    "   trigger finalinit\n"
#endif
#endif
#ifdef CONFIG_NETUTILS_NETINIT
    "on netinit\n"
    "   netinit\n"
#endif
    ;
  FAR const char *path = CONFIG_SYSTEM_NXINIT_RC_FILE_PATH;
  FAR const char *ext;
  char file[PATH_MAX];
  int ret;

  ret = init_parse_config_buffer(parser, preset, sizeof(preset));
  if (ret < 0)
    {
      return ret;
    }

  ret = init_parse_config_file(parser, path);
  if (ret < 0)
    {
      return ret;
    }

  /* Parse the optional cpu-specific config derived from the rc path, e.g.
   * "/etc/init.d/init.rc" -> "/etc/init.d/init.cpu0.rc".
   */

  ext = strrchr(path, '.');
  if (ext == NULL)
    {
      return 0;
    }

  snprintf(file, sizeof(file), "%.*s.cpu%d%s",
           (int)(ext - path), path, sched_getcpu(), ext);

  if (access(file, F_OK) < 0)
    {
      init_debug("skipping non-exist file %s", file);
      return 0;
    }

  return init_parse_config_file(parser, file);
}
