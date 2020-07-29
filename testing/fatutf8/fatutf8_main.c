/****************************************************************************
 * apps/testing/fatutf8/fatutf8_main.c
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname,
                       int exitcode) noreturn_function;
static void show_usage(FAR const char *progname, int exitcode)
{
  printf("USAGE: %s [<basepath>]\n", progname);
  printf("       %s --help\n", progname);
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#define BASEPATH "/mnt/sdcard"
#define FOLDER_NAME "/ÄÖÜ 测试文件夹 Test Folder"
#define FILE_NAME "/ÄÖÜ 测试文件 Test File"

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  int len;

  char buf[128];
  char path[256];

  char *basepath = NULL;

  if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
      show_usage("fatutf8", EXIT_SUCCESS);
    }
  else if (argc >= 2)
    {
      basepath = argv[1];

      len = strlen(basepath);
      if (basepath[len - 1] == '/')
        {
          basepath[--len] = '\0';
        }
    }

  if (basepath == NULL)
    {
      len = snprintf(path, sizeof(path), BASEPATH FOLDER_NAME);
    }
  else
    {
      len = snprintf(path, sizeof(path), "%s"FOLDER_NAME, basepath);
    }

  if (len + sizeof(FILE_NAME) >= sizeof(path))
    {
      printf("Error: Resulting Path too long.");
      return EXIT_FAILURE;
    }

  printf("mkdir(%s)\n", path);
  ret = mkdir(path, 0777);
  if (ret != 0)
    {
      printf("mkdir failed: %d\n", errno);
    }
  else
    {
      printf("mkdir successful\n");
    }

  printf("\n");

  strcat(path, FILE_NAME);

  printf("open(%s)\n", path);
  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (fd >= 0)
    {
      printf("open successful\n");
      printf("write(\"This is a test file in a test folder.\")\n");

      len = write(fd, "This is a test file in a test folder.", 37);
      if (len == 37)
        {
          printf("write successful\n");
        }
      else
        {
          printf("write failed: %d\n", errno);
        }

      fsync(fd);
      close(fd);
    }
  else
    {
      printf("open failed: %d\n", errno);
    }

  printf("\n");

  printf("open(%s)\n", path);
  fd = open(path, O_RDONLY);
  if (fd >= 0)
    {
      printf("open successful\n");

      len = read(fd, buf, sizeof(buf));
      buf[len] = '\0';

      if (len)
        {
          printf("read(\"%s\") successful\n", buf);
        }
      else
        {
          printf("read failed: %d\n", errno);
        }

      close(fd);
    }
  else
    {
      printf("open failed: %d\n", errno);
    }

  return EXIT_SUCCESS;
}
