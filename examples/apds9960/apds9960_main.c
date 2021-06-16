/****************************************************************************
 * apps/examples/apds9960/apds9960_main.c
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
#include <errno.h>
#include <fcntl.h>

#include <nuttx/sensors/apds9960.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_APDS9960_DEVNAME
#  define CONFIG_EXAMPLES_APDS9960_DEVNAME "/dev/gest0"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * apds9960_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int nbytes;
  char gest;

  fd = open(CONFIG_EXAMPLES_APDS9960_DEVNAME, O_RDONLY|O_NONBLOCK);
  if (fd < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_APDS9960_DEVNAME, errcode);
      goto errout;
    }

  while (1)
    {
      nbytes = read(fd, (void *)&gest, sizeof(gest));
      if (nbytes == 1)
        {
          switch (gest)
            {
              case DIR_LEFT:
                printf("LEFT\n");
                break;

              case DIR_RIGHT:
                printf("RIGHT\n");
                break;

              case DIR_UP:
                printf("UP\n");
                break;

              case DIR_DOWN:
                printf("DOWN\n");
                break;
            }
        }

      fflush(stdout);

      /* Wait 10ms */

      usleep(10000);
    }

  return 0;

errout:
  return EXIT_FAILURE;
}
