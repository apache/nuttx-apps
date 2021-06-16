/****************************************************************************
 * apps/examples/rfid_readuid/rfid_readuid.c
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <nuttx/contactless/mfrc522.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_CL_MFRC522
#  error "CONFIG_CL_MFRC522 is not defined in the configuration"
#endif

#ifndef CONFIG_EXAMPLES_RFID_READUID_DEVNAME
#  define CONFIG_EXAMPLES_RFID_READUID_DEVNAME "/dev/rfid0"
#endif


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * rfid_readuid_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  int count = 0;
  char buffer[10];

  fd = open(CONFIG_EXAMPLES_RFID_READUID_DEVNAME, O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open %s\n", CONFIG_EXAMPLES_RFID_READUID_DEVNAME);
      return -1;
    }

  /* Try to read a card up to 3 times */

  while (count < 3)
    {
      printf("Trying to READ: ");

      /* 11 bytes = 0x12345678\0 */

      ret = read(fd, buffer, 11);
      if (ret == 11)
        {
          printf("RFID CARD UID = %s\n", buffer);
          break;
        }

      if (ret == -EAGAIN || ret == -EPERM)
        {
          printf("Card is not present!\n");
        }
      else
        {
          printf("Unknown error!\n");
        }

      /* Wait 500ms before trying again */

      usleep(500000);
      count++;
    }

  return 0;
}
