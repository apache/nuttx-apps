/****************************************************************************
 * apps/system/microros/transport/microros_transport.c
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

#include <rmw_microros/rmw_microros.h>

#include <system/microros_transport.h>

#include "microros_transport.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int microros_transport_init(void)
{
#if defined(CONFIG_MICROROS_TRANSPORT_UDP)
  rmw_uros_set_custom_transport(false,
                                NULL,
                                microros_udp_open,
                                microros_udp_close,
                                microros_udp_write,
                                microros_udp_read);
  return 0;
#elif defined(CONFIG_MICROROS_TRANSPORT_SERIAL)
  rmw_uros_set_custom_transport(true,
                                NULL,
                                microros_serial_open,
                                microros_serial_close,
                                microros_serial_write,
                                microros_serial_read);
  return 0;
#else
  return -ENOSYS;
#endif
}
