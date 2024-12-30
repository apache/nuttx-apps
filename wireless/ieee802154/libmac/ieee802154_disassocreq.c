/****************************************************************************
 * apps/wireless/ieee802154/libmac/ieee802154_disassocreq.c
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

#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee802154_disassoc_req(int fd,
                            FAR struct ieee802154_disassoc_req_s *req)
{
  int ret;

  ret = ioctl(fd, MAC802154IOC_MLME_DISASSOC_REQUEST,
             (unsigned long)((uintptr_t)req));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr,
              "MAC802154IOC_MLME_DISASSOC_REQUEST failed: %d\n", ret);
    }

  return ret;
}
