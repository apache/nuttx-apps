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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "init.h"
#include "parser.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_parse_arguments(FAR char *buf, bool dup, int argc, FAR char **argv)
{
  bool quote = false;
  bool new = true;
  int i = 0;

  while (*buf != '\0')
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

      if (*buf == '-' && *(buf + 1) == '-')
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
            }
          else
            {
              quote = true;
              new = true;
              buf++;
            }
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
  bool create = false;
  FAR char *nl;
  size_t n = 0;
  int ret = 0;
  int fd;
#ifdef CONFIG_SYSTEM_NXINIT_DEBUG
  int line = 0;
#endif

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
      while ((nl = memchr(buf, '\n', n)))
        {
          *(nl++) = '\0';
          n -= nl - buf;
          init_debug("Line %3d: '%s'", ++line, buf);

          for (ret = 0; parser[ret].key; ret++)
            {
              if (!strncmp(parser[ret].key, buf, strlen(parser[ret].key)))
                {
                  create = true;
                  cur = &parser[ret];
                  init_debug("New section (%s)", parser[ret].key);
                  break;
                }
            }

          if (cur == NULL)
            {
              ret = -EINVAL;
              goto out;
            }

          ret = cur->parse(cur, create, buf);
          create = false;
          if (ret < 0)
            {
              goto out;
            }

          memmove(buf, nl, n);
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

int init_parse_configs(FAR const struct parser_s *parser,
                       FAR const char *path)
{
  FAR struct dirent *entry;
  char file[PATH_MAX];
  struct stat sb;
  FAR DIR *dir;
  int ret = 0;
  size_t i;

  if (stat(path, &sb) < 0)
    {
      init_err("Stat %s", path);
      return -errno;
    }

  if (S_ISDIR(sb.st_mode))
    {
      dir = opendir(path);
      if (dir == NULL)
        {
          init_err("Opening directory %s", path);
          return -errno;
        }

      while ((entry = readdir(dir)) != NULL)
        {
          if (DIRENT_ISFILE(entry->d_type))
            {
              i = strlen(entry->d_name);
              if (i >= 3 && !strcmp(entry->d_name + i - 3, ".rc"))
                {
                  snprintf(file, sizeof(file), "%s/%s", path, entry->d_name);
                  ret = init_parse_config_file(parser, file);
                  if (ret < 0)
                    {
                      break;
                    }
                }
            }
        }

      closedir(dir);
    }
  else if (S_ISREG(sb.st_mode))
    {
      ret = init_parse_config_file(parser, path);
    }

  return ret;
}
