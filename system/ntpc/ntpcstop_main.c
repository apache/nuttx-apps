/****************************************************************************
 * apps/system/ntpc/ntpcstop_main.c
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
#include <stdio.h>

#include "netutils/ntpclient.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ntpcstop_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret = ntpc_stop();
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ntpc_stop() failed\n");
      return EXIT_FAILURE;
    }

  printf("Stopped the NTP daemon\n");
  return EXIT_SUCCESS;
}
