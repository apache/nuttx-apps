/****************************************************************************
 * apps/system/adb/adb_banner.c
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

#include "adb.h"
#ifdef CONFIG_BOARDCTL_UNIQUEID
#include <sys/boardctl.h>
#include <string.h>
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int adb_fill_connect_data(char *buf, size_t bufsize)
{
  size_t len;
  size_t remaining = bufsize;

#ifdef CONFIG_BOARDCTL_UNIQUEID
  /* Get board id */

  int ret;
  uint8_t board_id[CONFIG_BOARDCTL_UNIQUEID_SIZE];

  memset(board_id, 0, CONFIG_BOARDCTL_UNIQUEID_SIZE);
  ret = boardctl(BOARDIOC_UNIQUEID, (uintptr_t)board_id);

  if (ret)
    {
      /* Failed to get board id */

      adb_log("failed to get board id\n");
      len = snprintf(buf, remaining, "device::");
    }
  else
    {
      /* FIXME only keep first 4 bytes */

      len = snprintf(buf, remaining, "device:%x:", *(uint32_t *)board_id);
    }
#else
  len = snprintf(buf, remaining, "device:" CONFIG_ADBD_DEVICE_ID ":");
#endif

  if (len >= remaining)
    {
      return -1;
    }

#ifdef CONFIG_ADBD_PRODUCT_NAME
  remaining -= len;
  buf += len;
  len = snprintf(buf, remaining,
                 "ro.product.name=" CONFIG_ADBD_PRODUCT_NAME ";");

  if (len >= remaining)
    {
      return bufsize;
    }
#endif

#ifdef CONFIG_ADBD_PRODUCT_MODEL
  remaining -= len;
  buf += len;
  len = snprintf(buf, remaining,
                 "ro.product.model=" CONFIG_ADBD_PRODUCT_MODEL ";");

  if (len >= remaining)
    {
      return bufsize;
    }
#endif

#ifdef CONFIG_ADBD_PRODUCT_DEVICE
  remaining -= len;
  buf += len;
  len = snprintf(buf, remaining,
                 "ro.product.device=" CONFIG_ADBD_PRODUCT_DEVICE ";");

  if (len >= remaining)
    {
      return bufsize;
    }
#endif

  remaining -= len;
  buf += len;
  len = snprintf(buf, remaining, "features=" CONFIG_ADBD_FEATURES);

  if (len >= remaining)
    {
      return bufsize;
    }

  return bufsize - remaining + len;
}
