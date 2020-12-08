/****************************************************************************
 * apps/examples/esp32_efuse/esp32_efuse_main.c
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <nuttx/efuse/esp_efuse.h>
#include <nuttx/efuse/esp_efuse_table.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * esp32_efuse_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct esp_efuse_par param;
  struct esp_efuse_par_id param2;
  int ret;
  int fd;

  fd = open("/dev/efuse", O_RDONLY);
  if (fd < 0)
    {
      printf("Failed to open /dev/efuse, error = %d!\n", errno);
      return -ENODEV;
    }

  /* Reading a Register: Block 0 | Register 1 */

  param.block = 0;
  param.reg   = 1;
  param.data  = malloc(sizeof(uint32_t));

  ret = ioctl(fd, EFUSEIOC_READ_REG, &param);
  if (ret < 0)
    {
      printf("Failed to run ioctl EFUSEIOC_READ_REG, error = %d!\n",
             errno);
      close(fd);
      return -EINVAL;
    }

  printf("Block %d Reg %d = 0x%08x\n", (int) param.block,
         param.reg, (uint32_t) *param.data);

  /* Reading a bit-field: ESP_EFUSE_CONSOLE_DEBUG_DISABLE */

  param2.field = ESP_EFUSE_CONSOLE_DEBUG_DISABLE;
  param2.data = malloc(sizeof(uint8_t));
  *param2.data = 0;

  ret = ioctl(fd, EFUSEIOC_READ_FIELD_BIT, &param2);
  if (ret < 0)
    {
      printf("Failed to run ioctl EFUSEIOC_READ_FIELD_BIT, error = %d!\n",
             errno);
      close(fd);
      return -EINVAL;
    }

  printf("CONSOLE_DISABLE = %d\n", *param2.data);

#if 0
  /* Writing block */

  param.block = 3;
  param.bit_offset = 0;
  param.bit_size = 128;
  param.data[0] = 0x00000003;
  param.data[1] = 0x00000000;
  param.data[2] = 0x00000000;
  param.data[3] = 0x00000001;

  /* Call the write ioctl */

  ret = ioctl(fd, EFUSEIOC_WRITE_REG, &param);
  if (ret < 0)
    {
      printf("Failed to run ioctl EFUSEIOC_WRITE_REG, error = %d!\n",
             errno);
      close(fd);
      return -EINVAL;
    }

  /* Commit the operation */

  ret = ioctl(fd, EFUSEIOC_BATCH_WRITE_COMMIT, NULL);
  if (ret < 0)
    {
      printf("Failed to run ioctl EFUSEIOC_WRITE_COMMIT, error = %d!\n",
             errno);
      close(fd);
      return -EINVAL;
    }
#endif

  /* Read a Block: Block 3 */

  param.block = 3;
  param.bit_offset = 0;
  param.bit_size = 128;

  /* Free previous allocate uint32_t and allocate 128-bits */

  free(param.data);
  param.data = malloc(sizeof(uint32_t) * 4);

  ret = ioctl(fd, EFUSEIOC_READ_BLOCK, &param);
  if (ret < 0)
    {
      printf("Failed to run ioctl EFUSEIOC_WRITE_REG, error = %d!\n",
             errno);
      close(fd);
      return -EINVAL;
    }

  printf("READ BLOCK Data: %08x %08x %08x %08x\n", param.data[0],
         param.data[1], param.data[2], param.data[3]);

  printf("Done!\n");
}

