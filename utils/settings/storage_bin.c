/****************************************************************************
 * apps/utils/settings/storage_bin.c
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

#include "utils/settings.h"
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/crc32.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VALID          0x600d
#define BUFFER_SIZE    256     /* Note alignment for Flash writes! */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

FAR static setting_t *getsetting(char *key);
extern state_t g_settings;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: load_bin
 *
 * Description:
 *    Loads binary data from a storage file.
 *
 * Input Parameters:
 *    file             - the filename of the storage to use
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

int load_bin(FAR char *file)
{
  static int fd;
  int i;
  int ret = OK;
  uint16_t valid;
  uint16_t count;

  fd = open(file, O_RDONLY);
  if (fd < 0)
    {
      return -ENOENT;
    }

  valid = 0;
  read(fd, &valid, sizeof(uint16_t));

  if (valid != VALID)
    {
      goto abort; /* Just exit - the settings aren't valid. What to do? */
    }

  count = 0;
  read(fd, &count, sizeof(uint16_t));

  uint32_t calc_crc = 0;
  uint32_t exp_crc = 0;

  for (i = 0; i < count; i++)
    {
      setting_t setting;
      read(fd, &setting, sizeof(setting_t));
      calc_crc = crc32part((FAR uint8_t *)&setting, sizeof(setting_t),
                            calc_crc);
    }

  read(fd, &exp_crc, sizeof(uint32_t));

  if (calc_crc != exp_crc)
    {
      ret = -EIO;
      goto abort;
    }

  lseek(fd, (sizeof(uint16_t) *2), SEEK_SET);  /* Get after valid & size */

  for (i = 0; i < count; i++)
    {
      setting_t setting;
      read(fd, &setting, sizeof(setting_t));

      FAR setting_t *slot = getsetting(setting.key);
      if (slot == NULL)
        {
          continue;
        }

      memcpy(slot, &setting, sizeof(setting_t));
    }

abort:
  close(fd);
  return ret;
}

/****************************************************************************
 * Name: save_bin
 *
 * Description:
 *    Saves binary data to a storage file.
 *
 * Input Parameters:
 *    file             - the filename of the storage to use
 *
 * Returned Value:
 *   Success or negated failure code
 *
 ****************************************************************************/

int save_bin(FAR char *file)
{
  FAR uint8_t *buffer = malloc(BUFFER_SIZE);
  int count;
  int fd;
  int ret = OK;

  if (buffer == NULL)
    {
      return -ENOMEM;
    }

  count = 0;
  int i;
  for (i = 0; i < CONFIG_SETTINGS_MAP_SIZE; i++)
    {
      if (g_settings.map[i].type == SETTING_EMPTY)
        {
          break;
        }

      count++;
    }

  fd = open(file, (O_RDWR | O_TRUNC), 0666);
  if (fd < 0)
    {
      ret = -ENODEV;
      goto abort;
    }

  memset(buffer, 0xff, BUFFER_SIZE);
  *((FAR uint16_t *)buffer) = VALID;
  *(((FAR uint16_t *)buffer) + 1) = count;

  off_t offset = sizeof(uint16_t) *2;  /* Valid & count */
  size_t data_size = count *sizeof(setting_t);
  size_t rem_data = data_size;
  uint32_t crc = crc32((FAR uint8_t *)g_settings.map, data_size);
  size_t rem_crc = sizeof(crc);

  while ((offset + rem_data + rem_crc) > 0)
    {
      size_t to_write = ((offset + rem_data) > BUFFER_SIZE) ?
                         (size_t)(BUFFER_SIZE - offset) : rem_data;
      memcpy((buffer + offset), (((FAR uint8_t *)g_settings.map) +
             (data_size - rem_data)), to_write);

      size_t j;
      for (j = (to_write + offset);
           (j < BUFFER_SIZE) && (rem_crc > 0); j++, rem_crc--)
        {
          off_t crc_byte = (off_t)(sizeof(crc) - rem_crc);
          buffer[j] = (crc >> (8 *crc_byte)) & 0xff;
        }

      write(fd, buffer, BUFFER_SIZE);

      rem_data -= to_write;
      offset = 0;
    }

  close(fd);

abort:
  free(buffer);

  return ret;
}

FAR setting_t *getsetting(FAR char *key)
{
  int i;

  for (i = 0; i < CONFIG_SETTINGS_MAP_SIZE; i++)
    {
      FAR setting_t *setting = &g_settings.map[i];

      if (strcmp(key, setting->key) == 0)
        {
          return setting;
        }

      if (setting->type == SETTING_EMPTY)
        {
          return setting;
        }
    }

  return NULL;
}

