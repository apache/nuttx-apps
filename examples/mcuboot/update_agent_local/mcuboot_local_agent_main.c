/****************************************************************************
 * apps/examples/mcuboot/update_agent_local/mcuboot_local_agent_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/boardctl.h>

#include <bootutil/bootutil_public.h>

#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define BUFFER_SIZE         4096
#define DEFAULT_FW_PATH     "/mnt/sdcard/firmware.bin"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct update_context_s
{
  FAR const struct flash_area *fa;
  uint32_t                     fa_offset;
  ssize_t                      image_size;
  int                          fd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_buffer[BUFFER_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: get_file_size
 *
 * Description:
 *   Retrieves the size of an open file descriptor.
 *
 * Input Parameters:
 *   fd - File descriptor of the open file.
 *
 * Returned Value:
 *   On success, returns the file size in bytes.
 *   On failure, returns -1.
 *
 ****************************************************************************/

static ssize_t get_file_size(int fd)
{
  struct stat st;

  if (fstat(fd, &st) < 0)
    {
      return -1;
    }

  return st.st_size;
}

/****************************************************************************
 * Name: copy_firmware_from_local
 *
 * Description:
 *   Copies a firmware binary from local storage to the secondary flash slot.
 *
 * Input Parameters:
 *   filepath - Path to the firmware binary file.
 *
 * Returned Value:
 *   On success, returns OK.
 *   On failure, returns an error code.
 *
 ****************************************************************************/

static int copy_firmware_from_local(FAR const char *filepath)
{
  int ret = OK;
  struct update_context_s ctx;
  ssize_t bytes_read;
  uint32_t total_copied = 0;
  uint32_t progress;

  /* Initialize context */

  memset(&ctx, 0, sizeof(ctx));
  ctx.fa = NULL;
  ctx.fa_offset = 0;
  ctx.image_size = -1;
  ctx.fd = -1;

  /* Open firmware file from local storage */

  ctx.fd = open(filepath, O_RDONLY);
  if (ctx.fd < 0)
    {
      fprintf(stderr, "Failed to open firmware file: %s\n", filepath);
      ret = -errno;
      goto exit;
    }

  /* Get file size */

  ctx.image_size = get_file_size(ctx.fd);
  if (ctx.image_size < 0)
    {
      fprintf(stderr, "Failed to get file size\n");
      ret = -errno;
      goto exit_close_file;
    }

  printf("Firmware file size: %zd bytes\n", ctx.image_size);

  /* Open secondary flash area for MCUBoot */

  ret = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(0), &ctx.fa);
  if (ret != OK)
    {
      fprintf(stderr, "Failed to open secondary flash area: %d\n", ret);
      goto exit_close_file;
    }

  /* Check if file fits in secondary slot */

  if (ctx.image_size > ctx.fa->fa_size)
    {
      fprintf(stderr, "Firmware file too large for secondary slot\n");
      fprintf(stderr, "File size: %zd, Slot size: %lu\n",
              ctx.image_size, ctx.fa->fa_size);
      ret = -EFBIG;
      goto exit_close_flash;
    }

  /* Erase secondary slot */

  printf("Erasing secondary flash slot...\n");
  ret = flash_area_erase(ctx.fa, 0, ctx.fa->fa_size);
  if (ret != OK)
    {
      fprintf(stderr, "Failed to erase secondary flash area: %d\n", ret);
      goto exit_close_flash;
    }

  /* Copy firmware from local storage to flash */

  printf("Copying firmware to secondary slot...\n");

  while (total_copied < ctx.image_size)
    {
      /* Read from local file */

      bytes_read = read(ctx.fd, g_buffer, BUFFER_SIZE);
      if (bytes_read < 0)
        {
          fprintf(stderr, "Failed to read from firmware file: %d\n", errno);
          ret = -errno;
          goto exit_close_flash;
        }

      if (bytes_read == 0)
        {
          break; /* EOF reached */
        }

      /* Adjust bytes to write if near end of file */

      if (total_copied + bytes_read > ctx.image_size)
        {
          bytes_read = ctx.image_size - total_copied;
        }

      /* Write to flash */

      ret = flash_area_write(ctx.fa, ctx.fa_offset, g_buffer, bytes_read);
      if (ret != OK)
        {
          fprintf(stderr, "Failed to write to flash: %d\n", ret);
          goto exit_close_flash;
        }

      ctx.fa_offset += bytes_read;
      total_copied += bytes_read;

      /* Show progress */

      progress = (total_copied * 100) / ctx.image_size;
      printf("Progress: %lu/%zd bytes [%lu%%]\n",
             total_copied, ctx.image_size, progress);
    }

  printf("Firmware copy completed successfully!\n");

exit_close_flash:
  flash_area_close(ctx.fa);

exit_close_file:
  if (ctx.fd >= 0)
    {
      close(ctx.fd);
    }

exit:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * mcuboot_local_agent_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  FAR const char *filepath;

  printf("MCUBoot Local Update Agent\n");

  if (argc > 1)
    {
      filepath = argv[1];
    }
  else
    {
      filepath = DEFAULT_FW_PATH;
    }

  printf("Firmware file: %s\n", filepath);

  /* Copy firmware from local storage to secondary flash slot */

  ret = copy_firmware_from_local(filepath);
  if (ret != OK)
    {
      fprintf(stderr, "Firmware update failed: %d\n", ret);
      return ERROR;
    }

  printf("Firmware successfully copied to secondary slot!\n");

  /* Mark image as pending for next boot */

  boot_set_pending_multi(0, 0);

  printf("Update scheduled for next boot. Restarting...\n");

  fflush(stdout);
  fflush(stderr);

  usleep(1000000); /* Wait 1 second */

  /* Restart system to apply update */

  boardctl(BOARDIOC_RESET, 0);

  return OK;
}
