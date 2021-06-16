/****************************************************************************
 * apps/examples/touchscreen/tc.h
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

#ifndef __APPS_EXAMPLES_TOUCHSCREEN_TC_H
#define __APPS_EXAMPLES_TOUCHSCREEN_TC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_EXAMPLES_TOUCHSCREEN_MINOR - The minor device number.  Minor=N
 *   corresponds to touchscreen device /dev/input0.  Note this value must
 *   with CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH.  Default 0.
 * CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH - The path to the touchscreen
 *   device.  This must be consistent with CONFIG_EXAMPLES_TOUCHSCREEN_MINOR.
 *   Default: "/dev/input0"
 * CONFIG_EXAMPLES_TOUCHSCREEN_NSAMPLES - This number of samples is collected
 *   and the program terminates.  Default:  Zero (Samples are collected
 *   indefinitely).
 * CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE - The touchscreen test can also be
 *   configured to work with a mouse driver by setting this option.
 */

#ifndef CONFIG_INPUT
#  error "Input device support is not enabled (CONFIG_INPUT)"
#endif

#ifndef CONFIG_EXAMPLES_TOUCHSCREEN_MINOR
#  undef  CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH
#  define CONFIG_EXAMPLES_TOUCHSCREEN_MINOR 0
#  ifdef CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE
#    define CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH "/dev/input0"
#  endif
#endif

#ifndef CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH
#  undef  CONFIG_EXAMPLES_TOUCHSCREEN_MINOR
#  define CONFIG_EXAMPLES_TOUCHSCREEN_MINOR 0
#  ifdef CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE
#    define CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH "/dev/mouse0"
#  else
#    define CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH "/dev/input0"
#  endif
#endif

#ifndef CONFIG_EXAMPLES_TOUCHSCREEN_NSAMPLES
#  define CONFIG_EXAMPLES_TOUCHSCREEN_NSAMPLES 0
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

#endif /* __APPS_EXAMPLES_TOUCHSCREEN_TC_H */
