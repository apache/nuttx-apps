/****************************************************************************
 * apps/examples/mcuboot/swap_test/version_main.c
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
#include <endian.h>

#include <errno.h>
#include <stdio.h>

#include <bootutil/bootutil_public.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOOT_HEADER_MAGIC_V1 0x96f3b83d
#define BOOT_HEADER_SIZE_V1  32

/****************************************************************************
 * Private Types
 ****************************************************************************/

begin_packed_struct struct mcuboot_raw_version_v1_s
{
    uint8_t  major;
    uint8_t  minor;
    uint16_t revision;
    uint32_t build_num;
} end_packed_struct;

begin_packed_struct struct mcuboot_raw_header_v1_s
{
  uint32_t header_magic;
  uint32_t image_load_address;
  uint16_t header_size;
  uint16_t pad;
  uint32_t image_size;
  uint32_t image_flags;
  struct mcuboot_raw_version_v1_s version;
  uint32_t pad2;
} end_packed_struct;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int boot_read_v1_header(uint8_t area_id,
                               struct mcuboot_raw_header_v1_s *raw_header)
{
  const struct flash_area *fa;
  int rc;

  rc = flash_area_open(area_id, &fa);
  if (rc)
    {
      return rc;
    }

  rc = flash_area_read(fa, 0, raw_header,
                       sizeof(struct mcuboot_raw_header_v1_s));
  flash_area_close(fa);
  if (rc)
    {
      return rc;
    }

  raw_header->header_magic       = le32toh(raw_header->header_magic);
  raw_header->image_load_address = le32toh(raw_header->image_load_address);
  raw_header->header_size        = le16toh(raw_header->header_size);
  raw_header->image_size         = le32toh(raw_header->image_size);
  raw_header->image_flags        = le32toh(raw_header->image_flags);
  raw_header->version.revision   = le16toh(raw_header->version.revision);
  raw_header->version.build_num  = le32toh(raw_header->version.build_num);

  if ((raw_header->header_magic != BOOT_HEADER_MAGIC_V1)
  ||  (raw_header->header_size   < BOOT_HEADER_SIZE_V1))
    {
      return -EIO;
    }

  return 0;
}

/****************************************************************************
 * mcuboot_version_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int rc;
  struct mcuboot_raw_header_v1_s raw_header;

  rc = boot_read_v1_header(0, &raw_header);
  if (rc)
    {
      printf("Only header version 1 can be read. Abort!\n");
      return rc;
    }

  printf("Image version %d.%d.%d.%ld\n", raw_header.version.major,
                                         raw_header.version.minor,
                                         raw_header.version.revision,
                                         raw_header.version.build_num);

  return 0;
}
