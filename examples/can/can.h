/****************************************************************************
 * apps/examples/examples/can/can.h
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

#ifndef __APPS_EXAMPLES_CAN_CAN_H
#define __APPS_EXAMPLES_CAN_CAN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* This test depends on these specific CAN configurations settings (your
 * specific CAN settings might require additional settings).
 *
 * CONFIG_CAN - Enables CAN support.
 * CONFIG_CAN_LOOPBACK - A CAN driver may or may not support a loopback
 *   mode for testing. The STM32 CAN driver does support loopback mode.
 *
 * Specific configuration options for this example include:
 *
 * CONFIG_CAN_LOOPBACK
 * CONFIG_EXAMPLES_CAN_DEVPATH - The path to the CAN device. Default: /dev/can0
 * CONFIG_EXAMPLES_CAN_NMSGS - This number of CAN message is collected and
 *   the program terminates. Default: 32.
 * CONFIG_EXAMPLES_CAN_READ - Only receive messages
 * CONFIG_EXAMPLES_CAN_WRITE - Only send messages
 * CONFIG_EXAMPLES_CAN_READWRITE - Receive and send messages
 */

#ifndef CONFIG_CAN
#  error "CAN device support is not enabled (CONFIG_CAN)"
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_EXAMPLES_CAN_CAN_H */
