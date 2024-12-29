/****************************************************************************
 * apps/examples/settings/settings_main.c
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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/param.h>

#include "system/settings.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * settings_main
 ****************************************************************************/

int settings_main(int argc, FAR char *argv[])
{
  int                  ret;
  int                  fd;
  enum settings_type_e stype;
  char                 text_path[CONFIG_PATH_MAX];
  char                 bin_path[CONFIG_PATH_MAX];
  const char           teststr[] = "I'm a string";
  int                  testval = 0x5aa5;
  char                 readstr[CONFIG_SYSTEM_SETTINGS_VALUE_SIZE];
#ifdef CONFIG_EXAMPLES_SETTINGS_USE_TMPFS
  struct stat          sbuf;

#  ifndef CONFIG_FS_TMPFS
#    error TMPFS must be enabled
#  endif
#  ifndef CONFIG_LIBC_TMPDIR
#    error LIBC_TMPDIR must be defined
#  endif
  if (stat(CONFIG_LIBC_TMPDIR, &sbuf))
    {
      ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
      if (ret < 0)
        {
          printf("ERROR: Failed to mount tmpfs at %s: %d\n",
                  CONFIG_LIBC_TMPDIR, ret);
          goto end;
        }
    }

  strcpy(bin_path, CONFIG_LIBC_TMPDIR);
#else
  strcpy(bin_path, CONFIG_EXAMPLES_SETTINGS_EXISTING_STORAGE);
  if (bin_path == NULL)
    {
      printf("Settings filepath is empty!");
      goto end;
    }
#endif

  strcat(bin_path, "/");
  strcat(bin_path, CONFIG_EXAMPLES_SETTINGS_FILENAME);
  strcpy(text_path, bin_path);
  strcat(bin_path, ".bin");
  strcat(text_path, ".txt");

  printf("Example of settings usage to file: %s and %s:",
          bin_path, text_path);
  printf("--------------------------------------------------------------\n");

  settings_init();

  ret = settings_setstorage(bin_path, STORAGE_BINARY);
  if (ret == -ENOENT)
    {
      printf("No existing binary storage file found. Creating it.\n");
      fd = open(bin_path, O_CREAT);
      if (fd < 0)
        {
          printf("Failed to create settings file\n");
          goto end;
        }

      close(fd);
    }
  else if (ret < 0)
    {
      printf("settings setstorage failed: %d\n", ret);
      goto end;
    }
  else
    {
      printf("existing bin settings storage file found\n");
    }

  ret = settings_setstorage(text_path, STORAGE_TEXT);
  if (ret == -ENOENT)
    {
      printf("No existing text storage file found. Creating it.\n");
      fd = open(text_path, O_CREAT);
      if (fd < 0)
        {
          printf("Failed to create settings file\n");
          goto end;
        }

      close(fd);
    }
  else if (ret < 0)
    {
      printf("settings setstorage failed: %d\n", ret);
      goto end;
    }
  else
    {
      printf("existing text settings storage file found\n");
    }

  ret = settings_create("v1", SETTING_STRING, "default value");
  if (ret < 0)
    {
      printf("settings create failed: %d\n", ret);
      goto end;
    }

  /* if this app has been run before, the setting type is likely changed */

  ret = settings_type("v1", &stype);
  if (ret < 0)
    {
      printf("Failed to get settings type: %d\n", ret);
      goto end;
    }

  if (stype == SETTING_STRING)
    {
      ret = settings_get("v1", SETTING_STRING, &readstr, sizeof(readstr));
      if (ret < 0)
        {
          printf("settings retrieve failed: %d\n", ret);
          goto end;
        }

      printf("Retrieved settings value (v1) with value:%s\n", readstr);
    }
  else
    {
      ret = settings_get("v1", SETTING_INT, &testval,
                          CONFIG_SYSTEM_SETTINGS_VALUE_SIZE);
      if (ret < 0)
        {
          printf("settings retrieve failed: %d\n", ret);
          goto end;
        }

      printf("Retrieved settings value (v1) with value:%d\n", testval);
    }

  printf("Trying to (re)create a setting that already exists (v1)\n");

  testval = 0xa5a5;
  ret = settings_create("v1", SETTING_INT, testval);
  if (ret == -EACCES)
    {
      printf("Deliberate fail: setting exists! Error: %d\n", ret);
    }
  else if (ret < 0)
    {
      printf("settings create failed: %d\n", ret);
      goto end;
    }

  ret = settings_type("v1", &stype);
  if (ret < 0)
    {
      printf("failed to read settings type: %d\n", ret);
      goto end;
    }

  printf("Retrieved setting type is: %d\n", stype);

  printf("Trying to change setting (v1) to integer type\n");
  ret = settings_set("v1", SETTING_INT, &testval);
  if (ret < 0)
    {
      printf("Deliberate fail: settings change invalid: %d\n", ret);
    }

  ret = settings_get("v2", SETTING_INT, &testval);
  if (ret < 0)
    {
      printf("Deliberate fail: non-existent setting requested. Error:%d\n",
             ret);
    }

  printf("Trying to change setting (v1) from int to string: %s\n", teststr);
  ret = settings_set("v1", SETTING_STRING, &teststr);
  if (ret < 0)
    {
      printf("Deliberate fail: settings change invalid: %d\n", ret);
    }

  printf("Creating a string settings value (s1):%s\n", teststr);
  ret = settings_create("s1", SETTING_STRING, teststr);
  if (ret < 0)
    {
      printf("settings changed failed: %d\n", ret);
      goto end;
    }

  ret = settings_get("s1", SETTING_STRING, &readstr, sizeof(readstr));
  if (ret < 0)
    {
      printf("settings retrieve failed: %d\n", ret);
      goto end;
    }

  printf("Retrieved string settings value (s1) with value:%s\n",
          readstr);

  FAR struct in_addr save_ip;
  FAR struct in_addr load_ip;
  save_ip.s_addr = 0xc0a86401;

  printf("Changing setting to an IP value (s1) with value:%d.%d.%d.%d\n",
          (int)(save_ip.s_addr >> 24) & 0xff,
          (int)(save_ip.s_addr >> 16) & 0xff,
          (int)(save_ip.s_addr >>  8) & 0xff,
          (int)(save_ip.s_addr >>  0) & 0xff);
  ret = settings_set("s1", SETTING_IP_ADDR, &save_ip);
  if (ret < 0)
    {
      printf("IP address settings create failed: %d\n", ret);
      goto end;
    }

  ret = settings_get("s1", SETTING_IP_ADDR, &load_ip);
  if (ret < 0)
    {
      printf("IP address settings retrieve failed: %d\n", ret);
      goto end;
    }

    printf("Retrieved IP address settings value (s1) with value:0x%08lx\n",
          load_ip.s_addr);

  printf("syncing storages\n");
  ret = settings_sync(true); /* wait for cached saves to complete
                              * (will only be done if caching enabled).
                              */
  if (ret < 0)
    {
      printf("Failed to sync storages: %d\n", ret);
      goto end;
    }

end:
  printf("exiting settings example app\n");
  fflush(stdout);

  return ret;
}
