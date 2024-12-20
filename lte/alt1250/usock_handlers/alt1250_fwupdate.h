/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_fwupdate.h
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

#ifndef __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_FWUPDATE_H
#define __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_FWUPDATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/modem/alt1250.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LTE_IMAGE_PERT_SIZE (256)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct delta_header_s
{
  uint32_t chunk_code;
  uint32_t reserved;
  char np_package[LTE_VER_NP_PACKAGE_LEN];
  uint32_t pert_crc;
  uint32_t hdr_crc;
};

struct update_info_s
{
  int hdr_injected;
  int act_injected;

  struct delta_header_s hdr;
  char img_pert[LTE_IMAGE_PERT_SIZE];
};

#endif  /* __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_FWUPDATE_H */
