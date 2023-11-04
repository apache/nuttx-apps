/****************************************************************************
 * apps/examples/settings/settings_main.c
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

int main(int argc, FAR char *argv[])
{
  int                  ret;
  int                  fd;
  enum settings_type_e stype;
  char                 path[50];
  char                 *teststr = "";
  int                  testval = 0x5aa5;
  char                 readstr[CONFIG_SETTINGS_VALUE_SIZE];
  enum storage_type_e  storage_type = STORAGE_BINARY;
  bool                 flag_present = false;
  struct stat          sbuf;

  if ((argc < 1) || *argv[1] == 0 || *(argv[1] + 1) == 0)
    {
      goto print_help;
    }

  if (*argv[1] == '-')
    {
      flag_present = true;
      if (*(argv[1] + 1) == 'b')
        {
          storage_type = STORAGE_BINARY;
        }
      else if (*(argv[1] + 1) == 't')
        {
          storage_type = STORAGE_TEXT;
        }
      else
        {
          goto print_help;
        }
    }

  if (flag_present && argc < 2)
    {
      goto print_help;
    }

#ifdef CONFIG_EXAMPLES_SETTINGS_USE_TMPFS
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

  strcpy(path, CONFIG_LIBC_TMPDIR);
  strcat(path, "/settings");
#else
  strcpy(path, CONFIG_EXAMPLES_SETTINGS_EXISTING_STORAGE);
  if (path == NULL)
    {
      printf("Settings filepath is empty!");
      goto end;
    }

  strcat(path, "/settings");
#endif

  printf("Example of settings usage: %s. Path: %s\n",
         ((storage_type == STORAGE_TEXT) ? "text" : "binary"), path);
  printf("-------------------------------------------------------\n");

  ret = settings_init();
  if (ret < 0)
    {
      printf("settings init failed: %d", ret);
      goto end;
    }

  ret = settings_setstorage(path, storage_type);
  if (ret == -ENOENT)
    {
      printf("INFO: no existing binary settings file found. Creating it.\n");
      fd = open(path, O_CREAT);
      if (fd < 0)
        {
          printf("failed to create settings file\n");
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
      printf("INFO: a settings file was found and loaded\n");
      ret = settings_get("v1", SETTING_STRING, readstr,
                         CONFIG_SETTINGS_VALUE_SIZE);
      if (ret < 0)
        {
          printf("settings retrieve failed: %d\n", ret);
          goto end;
        }

      printf("Retrieved settings value (v1) with value:%s\n",
              readstr);
    }

  printf("Creating an integer settings value (v1) with value:0x%x\n",
          testval);
  ret = settings_create("v1", SETTING_INT, testval);
  if (ret < 0)
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

  printf("Retrieved setting type is:%d\n", stype);

  ret = settings_get("v1", SETTING_INT, &testval);
  if (ret < 0)
    {
      printf("settings retrieve failed: %d\n", ret);
      goto end;
    }

  printf("Retrieved integer settings value (v1) with value:0x%x\n",
          testval);

  ret = settings_get("v1", SETTING_BOOL, &testval);
  if (ret < 0)
    {
      printf("Deliberate fail: BOOL type requested not INT. Error:%d\n",
              ret);
    }

  ret = settings_get("v2", SETTING_INT, &testval);
  if (ret < 0)
    {
      printf("Deliberate fail: non-existent setting requested. Error:%d\n",
             ret);
    }

  teststr = "I'm a string!";
  printf("Changing an integer settings value (v1) to string:%s\n",
          teststr);
  ret = settings_create("v1", SETTING_STRING, teststr);
  if (ret < 0)
    {
      printf("settings changed failed: %d\n", ret);
      goto end;
    }

  ret = settings_get("v1", SETTING_STRING, &readstr, sizeof(readstr));
  if (ret < 0)
    {
      printf("settings retrieve failed: %d\n", ret);
      goto end;
    }

  printf("Retrieved string settings value (v1) with value:%s\n",
          readstr);

  FAR struct in_addr save_ip;
  FAR struct in_addr load_ip;
  save_ip.s_addr = HTONL(0xc0a86401);

  printf("Creating an IP settings value (IP0) with value:0x%08lx\n",
          save_ip.s_addr);
  ret = settings_create("IP0", SETTING_IP_ADDR, &save_ip);
  if (ret < 0)
    {
      printf("IP address settings create failed: %d\n", ret);
      goto end;
    }

  ret = settings_get("IP0", SETTING_IP_ADDR, &load_ip);
  if (ret < 0)
    {
      printf("IP address settings retrieve failed: %d\n", ret);
      goto end;
    }

  printf("Retrieved IP address settings value (IP0) with value:0x%08lx\n",
          NTOHL(load_ip.s_addr));
end:

#ifdef CONFIG_SETTINGS_CACHED_SAVES
  /* saves may not have been written out before we exit */

  sleep(1);
#endif
  return ret;

print_help:
  printf("Usage...\n");
  printf("settings [-b | -t] \n");
  printf("    -i = use a binary storage file (default)\n");
  printf("    -t = use a text   storage file\n");
  printf(" Example:\n");
  printf("   settings -b\n");
  return -EINVAL;
}
