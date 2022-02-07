/****************************************************************************
 * apps/examples/mcuboot/swap_test/set_img_main.c
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
#include <stdlib.h>
#include <sys/boardctl.h>

#include <bootutil/bootutil_public.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mcuboot_set_img_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  boot_set_pending_multi(0, 0);

  printf("Requested update for next boot. Restarting...\n");

  fflush(stdout);
  fflush(stderr);

  usleep(1000);

  boardctl(BOARDIOC_RESET, 0);

  return 0;
}
