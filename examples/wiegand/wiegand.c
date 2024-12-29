/****************************************************************************
 * apps/examples/wiegand/wiegand.c
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
#include <inttypes.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <nuttx/wiegand/wiegand.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Wiegand
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  struct wiegand_data_s data;

  fd = open ("/dev/wiega0", O_RDONLY);

  if (fd < 0)
    {
      printf("Erro: Failed to open \n");
      return -1;
    }

  printf("Wiegand app is running\n");

  while (1)
    {
      ret = read(fd, &data, sizeof(data));
      if (ret > 0)
        {
          if (data.status == WIEGAND_SUCCESS)
            {
              printf("ABA Code: %d\n", data.aba_code);
              printf("FC: %04x \n", data.facility_code);
              printf("ID: %06x\n", data.id);
              break;
            }
        }

      sleep(1);
    }

  close (fd);

  return 0;
}
