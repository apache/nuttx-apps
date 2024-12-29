/****************************************************************************
 * apps/system/usbmsc/usbmsc.h
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

#ifndef __SYSTEM_USBMSC_USBMSC_H
#define __SYSTEM_USBMSC_USBMSC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <malloc.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* All global variables used by this add-on are packed into a structure in
 * order to avoid name collisions.
 */

struct usbmsc_state_s
{
  /* This is the handle that references to this particular USB storage driver
   * instance. The value of the driver handle must be remembered between the
   * 'msconn' and 'msdis' commands.
   */

  FAR void *mshandle;

  /* Heap usage samples.  These are useful for checking USB storage memory
   * usage and for tracking down memoryh leaks.
   */

#ifdef CONFIG_SYSTEM_USBMSC_DEBUGMM
  struct mallinfo mmstart;    /* Memory usage before the connection */
  struct mallinfo mmprevious; /* The last memory usage sample */
  struct mallinfo mmcurrent;  /* The current memory usage sample */
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* All global variables used by this add-on are packed into a structure in
 * order to avoid name collisions.
 */

extern struct usbmsc_state_s g_usbmsc;

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

#endif /* __SYSTEM_USBMSC_USBMSC_H */
