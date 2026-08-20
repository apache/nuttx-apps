/****************************************************************************
 * apps/system/grep/grep_main.c
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <limits.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GREP_F_INVERT  (1u << 0)   /* -v: invert match               */
#define GREP_F_ICASE   (1u << 1)   /* -i: case-insensitive           */
#define GREP_F_LINENO  (1u << 2)   /* -n: print line number          */
#define GREP_F_RECURSE (1u << 3)   /* -r: recurse into directories   */
#define GREP_F_NAME    (1u << 4)   /* -H: always print filename      */
#define GREP_F_NONAME  (1u << 5)   /* -h: never print filename       */

#define GREP_LINE_MAX  512

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int grep_stream(FILE *in, FAR const char *label,
                       FAR const regex_t *re, unsigned int flags)
{
  char line[GREP_LINE_MAX];
  unsigned long lineno = 0;
  int matched = 0;

  while (fgets(line, sizeof(line), in) != NULL)
    {
      size_t n = strlen(line);
      int hit;

      if (n > 0 && line[n - 1] == '\n')
        {
          line[n - 1] = '\0';
        }

      lineno++;

      hit = (regexec(re, line, 0, NULL, 0) == 0);
      if (flags & GREP_F_INVERT)
        {
          hit = !hit;
        }

      if (hit)
        {
          matched = 1;
          if (label != NULL && !(flags & GREP_F_NONAME))
            {
              printf("%s:", label);
            }

          if (flags & GREP_F_LINENO)
            {
              printf("%lu:", lineno);
            }

          printf("%s\n", line);
        }
    }

  return matched ? 0 : 1;
}

static int grep_file(FAR const char *path, FAR const regex_t *re,
                     unsigned int flags, int multi)
{
  FILE *f = fopen(path, "r");
  FAR const char *label = (multi || (flags & GREP_F_NAME)) ? path : NULL;
  int rc;

  if (f == NULL)
    {
      fprintf(stderr, "grep: %s: %s\n", path, strerror(errno));
      return 2;
    }

  rc = grep_stream(f, label, re, flags);
  fclose(f);
  return rc;
}

static int grep_dir(FAR const char *path, FAR const regex_t *re,
                    unsigned int flags, int multi)
{
  FAR DIR *d = opendir(path);
  char full[PATH_MAX];
  int rc = 1;
  FAR struct dirent *e;

  if (d == NULL)
    {
      fprintf(stderr, "grep: %s: %s\n", path, strerror(errno));
      return 2;
    }

  while ((e = readdir(d)) != NULL)
    {
      struct stat st;

      if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
        {
          continue;
        }

      snprintf(full, sizeof(full), "%s/%s", path, e->d_name);

      if (stat(full, &st) != 0)
        {
          continue;
        }

      if (S_ISDIR(st.st_mode))
        {
          if (flags & GREP_F_RECURSE)
            {
              int r = grep_dir(full, re, flags, multi);

              if (r == 0)
                {
                  rc = 0;
                }
              else if (r == 2 && rc != 0)
                {
                  rc = 2;
                }
            }
        }
      else if (S_ISREG(st.st_mode))
        {
          int r = grep_file(full, re, flags, multi);

          if (r == 0)
            {
              rc = 0;
            }
        }
    }

  closedir(d);
  return rc;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  unsigned int flags = 0;
  FAR const char *pattern;
  int idx;
  int cflags = REG_NOSUB;
  int nfiles;
  int multi;
  int rc = 1;
  regex_t re;
  int err;

  for (idx = 1; idx < argc; idx++)
    {
      FAR const char *p;

      if (argv[idx][0] != '-' || argv[idx][1] == '\0')
        {
          break;
        }

      for (p = argv[idx] + 1; *p != '\0'; p++)
        {
          switch (*p)
            {
              case 'i':
                flags |= GREP_F_ICASE;
                break;
              case 'v':
                flags |= GREP_F_INVERT;
                break;
              case 'n':
                flags |= GREP_F_LINENO;
                break;
              case 'r':
                flags |= GREP_F_RECURSE;
                break;
              case 'H':
                flags |= GREP_F_NAME;
                break;
              case 'h':
                flags |= GREP_F_NONAME;
                break;
              default:
                fprintf(stderr, "grep: invalid option -%c\n", *p);
                return 2;
            }
        }
    }

  if (idx >= argc)
    {
      fprintf(stderr, "usage: grep [-invrHh] <pattern> [<file>...]\n");
      return 2;
    }

  pattern = argv[idx++];

  if (flags & GREP_F_ICASE)
    {
      cflags |= REG_ICASE;
    }

  err = regcomp(&re, pattern, cflags);
  if (err != 0)
    {
      char errbuf[128];

      regerror(err, &re, errbuf, sizeof(errbuf));
      fprintf(stderr, "grep: %s\n", errbuf);
      return 2;
    }

  nfiles = argc - idx;
  multi  = (nfiles > 1) || (flags & GREP_F_RECURSE);

  if (nfiles == 0)
    {
      rc = grep_stream(stdin, NULL, &re, flags);
    }
  else
    {
      for (; idx < argc; idx++)
        {
          struct stat st;
          int r;

          if (stat(argv[idx], &st) != 0)
            {
              fprintf(stderr, "grep: %s: %s\n", argv[idx], strerror(errno));
              rc = 2;
              continue;
            }

          if (S_ISDIR(st.st_mode))
            {
              if (flags & GREP_F_RECURSE)
                {
                  r = grep_dir(argv[idx], &re, flags, multi);
                }
              else
                {
                  fprintf(stderr, "grep: %s: Is a directory\n", argv[idx]);
                  r = 2;
                }
            }
          else
            {
              r = grep_file(argv[idx], &re, flags, multi);
            }

          if (r == 0)
            {
              rc = 0;
            }
        }
    }

  regfree(&re);
  return rc;
}
