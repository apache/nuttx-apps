/****************************************************************************
 * apps/system/ptpd/ptpd_main.c
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

#include "netutils/ptpd.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ptpd_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int pid;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: ptpd <interface>\n");
      return 1;
    }

  pid = ptpd_start(argv[1]);
  if (pid < 0)
    {
      fprintf(stderr, "ERROR: ptpd_start() failed\n");
      return EXIT_FAILURE;
    }

  printf("Started the PTP daemon as PID=%d\n", pid);
  return EXIT_SUCCESS;
}
