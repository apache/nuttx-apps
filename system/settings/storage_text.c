/****************************************************************************
 * apps/system/settings/storage_text.c
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

#include "system/settings.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BUFFER_SIZE 256 /* Note alignment for Flash writes! */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: getsetting
 *
 * Description:
 *    Gets the setting information from a given key.
 *
 * Input Parameters:
 *    key        - key of the required setting
 *
 * Returned Value:
 *   The setting
 *
 ****************************************************************************/

FAR static setting_t *getsetting(char *key);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern setting_t map[CONFIG_SYSTEM_SETTINGS_MAP_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getsetting
 *
 * Description:
 *    Gets the setting information from a given key.
 *
 * Input Parameters:
 *    key        - key of the required setting
 *
 * Returned Value:
 *   The setting
 *
 ****************************************************************************/

FAR setting_t *getsetting(char *key)
{
  int i;
  FAR setting_t *setting;

  if (strlen(key) >= CONFIG_SYSTEM_SETTINGS_KEY_SIZE)
    {
      return NULL;
    }

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAP_SIZE; i++)
    {
      setting = &map[i];

      if (strcmp(key, setting->key) == 0)
        {
          return setting;
        }

      if (setting->type == SETTING_EMPTY)
        {
          strncpy(setting->key, key, CONFIG_SYSTEM_SETTINGS_KEY_SIZE);
          setting->key[CONFIG_SYSTEM_SETTINGS_KEY_SIZE - 1] = '\0';
          return setting;
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: load_text
 *
 * Description:
 *    Loads text from a storage file.
 *
 * Input Parameters:
 *    file             - the filename of the storage to use
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

int load_text(FAR char *file)
{
  int           ret = OK;
  FAR FILE      *f;
  FAR char      *eq;
  FAR char      *key;
  FAR char      *val;
  FAR char      *backup_file;
  FAR char      *buffer;
  FAR setting_t *setting;

  /* Check that the file exists */

  if (access(file, F_OK) != 0)
    {
      /* If not, try the backup file */

      backup_file = malloc(strlen(file) + 2);
      if (backup_file == NULL)
        {
          return -ENODEV;
        }

      strcpy(backup_file, file);
      strcat(backup_file, "~");

      if (access(backup_file, F_OK) == 0)
        {
          /* There is a backup file
           * Restore this as the new settings file
           */

          rename(backup_file, file);
        }

      free(backup_file);
    }

  f = fopen(file, "r");
  if (f == NULL)
    {
      return -ENOENT;
    }

  buffer = malloc(BUFFER_SIZE);
  if (buffer == NULL)
    {
      ret = -ENOMEM;
      goto abort;
    }

  while (fgets(buffer, BUFFER_SIZE, f))
    {
      int  i;

      /* Remove any line terminators */

      for (i = ((int)strlen(buffer) - 1); i > 0; i--)
        {
          if (buffer[i] == ';' || buffer[i] == '\n' || buffer[i] == '\r')
            {
              buffer[i] = '\0';
            }
          else
            {
              break;
            }
        }

      /* Separate the key / value pair */

      eq = strchr(buffer, '=');
      if (eq == NULL)
        {
          continue;
        }

      key = buffer;
      val = eq + 1;
      *eq = '\0';

      /* Check if the key is valid */

      if (!isalpha(key[0]))
        {
          continue;
        }

      /* Get the setting slot */

      setting = getsetting(key);
      if (setting == NULL)
        {
          continue;
        }

      /* Parse the value */

      if (isalpha(val[0]))
        {
          if (strcasecmp(val, "true") == 0)
            {
              /* It's a boolean */

              setting->type = SETTING_BOOL;
              setting->val.i = 1;
            }
          else if (strcasecmp(val, "false") == 0)
            {
              /* It's a boolean */

              setting->type = SETTING_BOOL;
              setting->val.i = 0;
            }
          else
            {
              /* It's a string */

              if (strlen(val) >= CONFIG_SYSTEM_SETTINGS_VALUE_SIZE)
                {
                  continue;
                }

              setting->type = SETTING_STRING;
              strncpy(setting->val.s, val,
                      CONFIG_SYSTEM_SETTINGS_VALUE_SIZE);
              setting->val.s[CONFIG_SYSTEM_SETTINGS_VALUE_SIZE - 1] = '\0';
            }
        }
      else
        {
          if (strchr(val, '.') != NULL)
            {
              FAR char *s = val;
              i = 0;
              while (s[i]) s[i] == '.' ? i++ : *s++;
              if (i == 1)
                {
                  /* It's a float */

                  double d = 0;
                  if (sscanf(val, "%lf", &d) == 1)
                    {
                      setting->type = SETTING_FLOAT;
                      setting->val.f = d;
                    }
                }
              else if (i == 3)
                {
                  /* It's an IP address */

                  setting->type = SETTING_IP_ADDR;
                  inet_pton(AF_INET, val, &setting->val.ip);
                }
            }
          else
            {
              /* It's an integer */

              i = 0;
              if (sscanf(val, "%d", &i) == 1)
                {
                  setting->type = SETTING_INT;
                  setting->val.i = i;
                }
            }
        }

      /* Handle parse errors */

      if (setting->type == SETTING_EMPTY)
        {
          memset(setting->key, 0, CONFIG_SYSTEM_SETTINGS_KEY_SIZE);
        }
    }

  free(buffer);

abort:
  fclose(f);

  return ret;
}

/****************************************************************************
 * Name: save_text
 *
 * Description:
 *    Saves text to a storage file.
 *
 * Input Parameters:
 *    file             - the filename of the storage to use
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

int save_text(FAR char *file)
{
  int      ret = OK;
  FAR char *backup_file = malloc(strlen(file) + 2);
  FAR FILE *f;
  int      i;

  if (backup_file == NULL)
    {
      return -ENODEV;
    }

  strcpy(backup_file, file);
  strcat(backup_file, "~");

  f = fopen(backup_file, "w");
  if (f == NULL)
    {
      ret = -ENODEV;
      goto abort;
    }

  for (i = 0; i < CONFIG_SYSTEM_SETTINGS_MAP_SIZE; i++)
    {
      if (map[i].type == SETTING_EMPTY)
        {
          break;
        }

      switch (map[i].type)
        {
          case SETTING_STRING:
            {
              fprintf(f, "%s=%s\n", map[i].key,
                      map[i].val.s);
            }
            break;

          case SETTING_INT:
            {
              fprintf(f, "%s=%d\n", map[i].key,
                      map[i].val.i);
            }
            break;
          case SETTING_BOOL:
            {
              fprintf(f, "%s=%s\n", map[i].key,
                      map[i].val.i ?
                      "true" : "false");
            }
            break;

          case SETTING_FLOAT:
            {
              fprintf(f, "%s=%.06f\n", map[i].key,
                      map[i].val.f);
            }
            break;

          case SETTING_IP_ADDR:
            {
              char buffer[20];
              inet_ntop(AF_INET, &map[i].val.ip, buffer, 20);
              fprintf(f, "%s=%s\n", map[i].key, buffer);
            }
            break;

          default:
            {
              return -EINVAL;
            }
            break;
        }
    }

  fclose(f);

  remove(file);
  rename(backup_file, file);

abort:
  free(backup_file);

  return ret;
}
