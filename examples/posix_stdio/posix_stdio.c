/****************************************************************************
 * apps/examples/posix_stdio/posix_stdio.c
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
 * This is a single POSIX example that works for both Nuttx and Linux distros
 * (with minimal changes). By this example its possible to learn how to use
 * posix style and also works with onpen(), close() and write() functions.
 *
 * For linux, need to change the headers and the output device
 * A) Headers:
 *     #include <fcntl.h>
 *     #include <unistd.h>
 *     #include <stdio.h>
 *     #include <string.h>
 *
 * B) output:
 *     /dev/tty
 *
 * To compile it on Linux, you can simple use gcc:
 *  gcc posix_stdio.c -o posix_stdio
 *
 * To run, just send the following command: ./posix_stdio
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 * *************************************************************************/

/* define how many times the message will be printed in the console */

#ifndef PRINT_N_TIMES
#  define PRINT_N_TIMES 10
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void print_message(int fd)
{
  const char *message = "Hello, NuttX users, welcome!!!\n";

  write(fd, message, strlen(message));
}

/****************************************************************************
 * hello_nuttx_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int counter;
  int fd = open("/dev/console", O_WRONLY);

  if (fd < 0)
    {
      fprintf(stderr, "Failed to open console\n");
      return 1;
    }

  for (counter = 0; counter < PRINT_N_TIMES; counter++)
    {
      print_message(fd);
    }

  close(fd);
  return 0;
}

