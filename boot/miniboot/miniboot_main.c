/****************************************************************************
 * apps/boot/miniboot/miniboot_main.c
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
#include <syslog.h>

#include <sys/boardctl.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: miniboot_main
 *
 * Description:
 *   Minimal bootlaoder entry point.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_boot_info_s info;

  syslog(LOG_INFO, "*** miniboot ***\n");

  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif

  /* Call board specific image boot */

  info.path        = CONFIG_MINIBOOT_SLOT_PATH;
  info.header_size = CONFIG_MINIBOOT_HEADER_SIZE;

  return boardctl(BOARDIOC_BOOT_IMAGE, (uintptr_t)&info);
}
