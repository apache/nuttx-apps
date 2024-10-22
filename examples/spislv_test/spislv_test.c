/****************************************************************************
 * apps/examples/spislv_test/spislv_test.c
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SOURCE_FILE "/dev/spislv2"
#define BUFFER_SIZE 256

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * spislv_test
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  ssize_t i;

  printf("Slave started!!\n");
  fd = open(SOURCE_FILE, O_RDWR);

  if (fd < 0)
    {
      printf("Failed to open %s: %s\n", SOURCE_FILE, strerror(errno));
      return 0;
    }

  while (1)
    {
      /* Read the number from the source file */

      printf("Slave: Reading from %s\n", SOURCE_FILE);
      bytes_read = read(fd, buffer, BUFFER_SIZE - 1);

      if (bytes_read < 0)
        {
          printf("Failed to read from %s: %s\n",
                    SOURCE_FILE, strerror(errno));
          close(fd);
          return 0;
        }
      else if (bytes_read > 0)
        {
          buffer[bytes_read] = '\0';

          /* Print buffer in hexadecimal format */

          printf("Slave: Read value in hex: ");
          for (i = 0; i < bytes_read; ++i)
            {
              printf("%02x ", (unsigned char)buffer[i]);
            }

          printf("\n");

          /* Write the same value back */

          printf("Slave: Writing %d bytes back to %s\n",
                    bytes_read, SOURCE_FILE);
          ssize_t bytes_written = write(fd, buffer, bytes_read);
          if (bytes_written < 0)
            {
              printf("Failed to write to %s: %s\n",
                        SOURCE_FILE, strerror(errno));
              close(fd);
              return 0;
            }
        }
    }
}
