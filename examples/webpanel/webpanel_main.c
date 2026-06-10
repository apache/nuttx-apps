/****************************************************************************
 * apps/examples/webpanel/webpanel_main.c
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

#include <sys/mount.h>
#include <sys/boardctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <nuttx/drivers/ramdisk.h>

#include "netutils/thttpd.h"
#include "fsutils/mksmartfs.h"
#include "ws_terminal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SECTORSIZE   64
#define NSECTORS(b)  (((b) + SECTORSIZE - 1) / SECTORSIZE)
#define WEBPANEL_RAMDISK_MINOR 2
#define ROMFSDEV     "/dev/ram2"

#define ROMFS_MOUNTPT  "/data/tmp_romfs"
#define BINFS_MOUNTPT  "/data/tmp_binfs"
#define BINFS_PREFIX   "cgi-bin"
#define UNIONFS_MOUNTPT CONFIG_THTTPD_PATH

#define SMARTFS_DEV    "/dev/smart0"
#define SMARTFS_MNT    "/mnt"

/****************************************************************************
 * External References
 ****************************************************************************/

extern const unsigned char webpanel_romfs_img[];
extern const unsigned int  webpanel_romfs_img_len;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Initialize WebPanel filesystem mounts, start the websocket daemon,
 *   then launch the THTTPD server.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Argument vector.
 *
 * Returned Value:
 *   EXIT_SUCCESS on success; EXIT_FAILURE on setup error.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_romdisk_s desc;
  char *thttpd_argv = "thttpd";
  struct statfs check_fs;
  struct statfs sfs;
  int ret;

  /* Prevent duplicate startup (e.g. rcS runs again in child NSH) */

  if (statfs(UNIONFS_MOUNTPT, &check_fs) == 0)
    {
      return 0;
    }

  /* Register the ROMFS image as a ramdisk */

  printf("WebPanel: Registering ROMFS ramdisk\n");

  desc.minor    = WEBPANEL_RAMDISK_MINOR;
  desc.nsectors = NSECTORS(webpanel_romfs_img_len);
  desc.sectsize = SECTORSIZE;
  desc.image    = (FAR uint8_t *)webpanel_romfs_img;

  ret = boardctl(BOARDIOC_ROMDISK, (uintptr_t)&desc);
  if (ret < 0 && errno != EEXIST)
    {
      printf("WebPanel: ERROR romdisk_register failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Create mount point directories */

  mkdir("/data", 0777);
  mkdir(ROMFS_MOUNTPT, 0777);
  mkdir(BINFS_MOUNTPT, 0777);

  /* Mount ROMFS at temp location */

  printf("WebPanel: Mounting ROMFS at %s\n", ROMFS_MOUNTPT);

  ret = mount(ROMFSDEV, ROMFS_MOUNTPT, "romfs", MS_RDONLY, NULL);
  if (ret < 0 && errno != EBUSY)
    {
      printf("WebPanel: ERROR mount ROMFS failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Mount BINFS at temp location (exposes built-in apps as CGI) */

  printf("WebPanel: Mounting BINFS at %s\n", BINFS_MOUNTPT);

  ret = mount(NULL, BINFS_MOUNTPT, "binfs", MS_RDONLY, NULL);
  if (ret < 0 && errno != EBUSY)
    {
      printf("WebPanel: ERROR mount BINFS failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Create UNIONFS: ROMFS content + BINFS under cgi-bin/ prefix */

  printf("WebPanel: Creating UNIONFS at %s\n", UNIONFS_MOUNTPT);

  ret = mount(NULL, UNIONFS_MOUNTPT, "unionfs", 0,
              "fspath1=" ROMFS_MOUNTPT ",prefix1="
              ",fspath2=" BINFS_MOUNTPT ",prefix2=" BINFS_PREFIX);
  if (ret < 0 && errno != EBUSY)
    {
      printf("WebPanel: ERROR mount UNIONFS failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  /* Ensure SmartFS is formatted and mounted at /mnt for user files */

  if (statfs(SMARTFS_MNT, &sfs) != 0)
    {
      printf("WebPanel: SmartFS not mounted, formatting %s\n",
             SMARTFS_DEV);
      ret = mksmartfs(SMARTFS_DEV, 0);
      if (ret < 0)
        {
          printf("WebPanel: WARNING mksmartfs failed: %d\n", errno);
        }
      else
        {
          mkdir(SMARTFS_MNT, 0777);
          ret = mount(SMARTFS_DEV, SMARTFS_MNT, "smartfs", 0, NULL);
          if (ret < 0)
            {
              printf("WebPanel: WARNING mount SmartFS failed: %d\n",
                     errno);
            }
          else
            {
              printf("WebPanel: SmartFS mounted at %s\n", SMARTFS_MNT);
            }
        }
    }
  else
    {
      printf("WebPanel: SmartFS already mounted at %s\n", SMARTFS_MNT);
    }

  /* Start WebSocket terminal server */

  ret = ws_terminal_start();
  if (ret < 0)
    {
      printf("WebPanel: WARNING WebSocket server failed to start\n");
    }

  /* Start THTTPD */

  printf("WebPanel: Starting THTTPD (serving from %s)\n", UNIONFS_MOUNTPT);
  fflush(stdout);

  thttpd_main(1, &thttpd_argv);

  printf("WebPanel: THTTPD terminated\n");
  return 0;
}
