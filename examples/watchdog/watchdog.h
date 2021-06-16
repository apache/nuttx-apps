/****************************************************************************
 * apps/examples/examples/watchdog/watchdog.h
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

#ifndef __APPS_EXAMPLES_WATCHDOG_WATCHDOG_H
#define __APPS_EXAMPLES_WATCHDOG_WATCHDOG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_NSH_BUILTIN_APPS - Build the WATCHDOG test as an NSH built-in
 *   function.
 * CONFIG_EXAMPLES_WATCHDOG_DEVPATH - The path to the Watchdog device.
 *   Default: /dev/watchdog0
 * CONFIG_EXAMPLES_WATCHDOG_PINGTIME - Time in milliseconds that the example
 *   will ping the watchdog before letting the watchdog expire. Default: 5000
 *   milliseconds
 * CONFIG_EXAMPLES_WATCHDOG_PINGDELAY - Time delay between pings in
 *   milliseconds. Default: 500 milliseconds.
 * CONFIG_EXAMPLES_WATCHDOG_TIMEOUT - The watchdog timeout value in
 *   milliseconds before the watchdog timer expires.  Default:  2000
 *   milliseconds.
 */

#ifndef CONFIG_WATCHDOG
#  error "WATCHDOG device support is not enabled (CONFIG_WATCHDOG)"
#endif

#ifndef CONFIG_EXAMPLES_WATCHDOG_DEVPATH
#  define CONFIG_EXAMPLES_WATCHDOG_DEVPATH "/dev/watchdog0"
#endif

#ifndef CONFIG_EXAMPLES_WATCHDOG_PINGTIME
#  define CONFIG_EXAMPLES_WATCHDOG_PINGTIME 5000
#endif

#ifndef CONFIG_EXAMPLES_WATCHDOG_PINGDELAY
#  define CONFIG_EXAMPLES_WATCHDOG_PINGDELAY 500
#endif

#ifndef CONFIG_EXAMPLES_WATCHDOG_TIMEOUT
#  define CONFIG_EXAMPLES_WATCHDOG_TIMEOUT 2000
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

#endif /* __APPS_EXAMPLES_WATCHDOG_WATCHDOG_H */
