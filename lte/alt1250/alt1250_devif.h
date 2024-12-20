/****************************************************************************
 * apps/lte/alt1250/alt1250_devif.h
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

#ifndef __APSS_LTE_ALT1250_ALT1250_DEVIF_H
#define __APPS_LTE_ALT1250_ALT1250_DEVIF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include <nuttx/modem/alt1250.h>
#include "alt1250_daemon.h"

#define DEV_ALT1250  "/dev/alt1250"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int init_alt1250_device(void);
void finalize_alt1250_device(int fd);
FAR struct alt_container_s *altdevice_exchange_selcontainer(int fd,
    FAR struct alt_container_s *container);
int altdevice_send_command(FAR struct alt1250_s *dev, int fd,
                           FAR struct alt_container_s *container,
                           FAR int32_t *usock_res);
int altdevice_powercontrol(int fd, uint32_t cmd);
int altdevice_seteventbuff(int fd, FAR struct alt_evtbuffer_s *buffers);
int altdevice_getevent(int fd, FAR uint64_t *evtbitmap,
                       FAR struct alt_container_s **replys);
void altdevice_reset(int fd);
#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
int altdevice_powerresponse(int fd, uint32_t cmd, int resp);
#endif

#endif  /* __APPS_LTE_ALT1250_ALT1250_DEVIF_H */
