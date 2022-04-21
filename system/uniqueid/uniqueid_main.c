/****************************************************************************
 * apps/system/uniqueid/uniqueid_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/boardctl.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct cfg_s
{
  FAR const char *format;
  FAR const char *delim;
  uint8_t positions[CONFIG_BOARDCTL_UNIQUEID_SIZE];
  size_t count;
  FAR const char *prefix;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int parse_positions(FAR char *arg, FAR uint8_t *positions,
                           FAR size_t *count)
{
  FAR char *list_save;
  FAR char *list_item;
  FAR char *range_save;
  FAR char *range_item;
  FAR char *endptr;
  FAR int pos;
  FAR int pos2;

#define APPEND_POSITION(pos) \
  do \
    { \
      if ((*count) >= CONFIG_BOARDCTL_UNIQUEID_SIZE) \
        { \
          fprintf(stderr, "ERROR: Too many bytes selected\n"); \
          return -1; \
        } \
      \
      positions[*count] = pos; \
      (*count)++; \
    } \
  while (0);

  list_item = strtok_r(arg, ",", &list_save);
  while (list_item)
    {
      range_item = strtok_r(list_item, "-", &range_save);
      pos = strtol(range_item, &endptr, 0);
      if (endptr == range_item || *endptr != '\0')
        {
          fprintf(stderr, "ERROR: Invalid byte position: '%s'\n",
                  range_item);
          return -1;
        }

      pos--;
      if (pos < 0 || pos >= CONFIG_BOARDCTL_UNIQUEID_SIZE)
        {
          fprintf(stderr, "ERROR: Invalid byte position: '%s'\n",
                  range_item);
          return -1;
        }

      range_item = strtok_r(0, "-", &range_save);
      if (range_item)
        {
          pos2 = strtol(range_item, &endptr, 0);
          if (endptr == range_item || *endptr != '\0')
            {
              fprintf(stderr, "ERROR: Invalid byte position: '%s'\n",
                      range_item);
              return -1;
            }

          pos2--;
          if (pos2 < 0 || pos2 >= CONFIG_BOARDCTL_UNIQUEID_SIZE)
            {
              fprintf(stderr, "ERROR: Invalid byte position: '%s'\n",
                      range_item);
              return -1;
            }

          range_item = strtok_r(0, "-", &range_save);
          if (range_item)
            {
              fprintf(stderr, "ERROR: Invalid byte range: '%d-%d-%s'\n",
                      pos + 1, pos2 + 1, range_item);
              return -1;
            }

          if (pos2 > pos)
            {
              for (; pos <= pos2; pos++)
                {
                  APPEND_POSITION(pos);
                }
            }
          else
            {
              for (; pos >= pos2; pos--)
                {
                  APPEND_POSITION(pos);
                }
            }
        }
      else
        {
          APPEND_POSITION(pos);
        }

      list_item = strtok_r(0, ",", &list_save);
    }

  return 0;
}

static int parsearg(int argc, FAR char *argv[], FAR struct cfg_s *cfg)
{
  int opt;
  int len;

  while ((opt = getopt(argc, argv, ":b:d:f:p:")) != ERROR)
    {
      switch (opt)
        {
          case 'b':
            if (parse_positions(optarg, cfg->positions, &cfg->count) != 0)
              {
                return -1;
              }
            break;

          case 'd':
            cfg->delim = optarg;
            break;

          case 'f':
            len = strlen(optarg);
            if (len == 0)
              {
                fprintf(stderr, "ERROR: Invalid format: '%s'\n", optarg);
                return -1;
              }

            switch (optarg[len - 1])
              {
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'c':
                  cfg->format = optarg;
                  break;

                default:
                  fprintf(stderr, "ERROR: Invalid format: '%s'\n", optarg);
                  return -1;
              }

            break;

          case 'p':
            cfg->prefix = optarg;
            break;

          case ':':
            fprintf(stderr, "ERROR: Option needs a value: '%c'\n", optopt);
            return -1;

          case '?':
            fprintf(stderr, "ERROR: Unrecognized option: '%c'\n", optopt);
            return -1;

          default:
            fprintf(stderr, "ERROR: Unhandled getopt: '%c'\n", opt);
            return -1;
        }
    }

  if (optind != argc)
    {
      fprintf(stderr, "ERROR: Too many arguments\n");
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  uint8_t uniqueid[CONFIG_BOARDCTL_UNIQUEID_SIZE];
  FAR char *formatter;
  int i;

  struct cfg_s cfg =
    {
      "02x"
    };

  if (parsearg(argc, argv, &cfg) != 0)
    {
      return -1;
    }

  if (cfg.count == 0)
    {
      /* Print all bytes if none were specified. */

      cfg.count = CONFIG_BOARDCTL_UNIQUEID_SIZE;
      for (i = 0; i < cfg.count; i++)
        {
          cfg.positions[i] = i;
        }
    }

  memset(uniqueid, 0, sizeof(uniqueid));
  if (boardctl(BOARDIOC_UNIQUEID, (uintptr_t)&uniqueid) != OK)
    {
      fprintf(stderr, "ERROR! board_uniqueid() failed\n");
      return -1;
    }

  formatter = malloc(strlen(cfg.format) + 2);
  sprintf(formatter, "%%%s", cfg.format);

  if (cfg.prefix != NULL)
    {
      printf("%s", cfg.prefix);
    }

  for (i = 0; i < cfg.count; i++)
    {
      if (i > 0 && cfg.delim != NULL)
        {
          printf("%s", cfg.delim);
        }

      printf(formatter, uniqueid[cfg.positions[i]]);
    }

  putchar('\n');
  free(formatter);

  return EXIT_SUCCESS;
}
