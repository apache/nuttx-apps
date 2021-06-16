/****************************************************************************
 * apps/examples/dhcpd/host.c
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

#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern int dhcpd_run(const char *interface);

/****************************************************************************
 * main
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  /* One and only one argument is expected:  The network device name. */

  if (argc != 2)
    {
      fprintf(stderr, "ERROR: Invalid number of arguments\n");
      fprintf(stderr, "Usage: %s <device-name>\n", argv[0]);
      exit(EXIT_FAILURE);
    }

  dhcpd_run(argv[1]);

  return 0;
}
