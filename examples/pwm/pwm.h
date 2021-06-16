/****************************************************************************
 * apps/examples/examples/pwm/pwm.h
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

#ifndef __APPS_EXAMPLES_PWM_PWM_H
#define __APPS_EXAMPLES_PWM_PWM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_NSH_BUILTIN_APPS - Build the PWM test as an NSH built-in function.
 * CONFIG_EXAMPLES_PWM_DEVPATH - The path to the PWM device. Default: /dev/pwm0
 * CONFIG_EXAMPLES_PWM_FREQUENCY - The initial PWM frequency.  Default: 100 Hz
 * CONFIG_EXAMPLES_PWM_DUTYPCT - The initial PWM duty as a percentage.  Default: 50%
 * CONFIG_EXAMPLES_PWM_DURATION - The initial PWM pulse train duration in seconds.
 *   Used only if the current pulse count is zero (pulse count is only supported
 *   if CONFIG_PWM_PULSECOUNT is defined). Default: 5 seconds
 * CONFIG_EXAMPLES_PWM_PULSECOUNT - The initial PWM pulse count.  This option is
 *   only available if CONFIG_PWM_PULSECOUNT is defined. Default: 0 (i.e., use
 *   the duration, not the count).
 */

#ifndef CONFIG_PWM
#  error "PWM device support is not enabled (CONFIG_PWM)"
#endif

#ifndef CONFIG_EXAMPLES_PWM_DEVPATH
#  define CONFIG_EXAMPLES_PWM_DEVPATH "/dev/pwm0"
#endif

#ifndef CONFIG_EXAMPLES_PWM_FREQUENCY
#  define CONFIG_EXAMPLES_PWM_FREQUENCY 100
#endif

#ifndef CONFIG_EXAMPLES_PWM_DUTYPCT
#  define CONFIG_EXAMPLES_PWM_DUTYPCT 50
#endif

#ifndef CONFIG_EXAMPLES_PWM_DURATION
#  define CONFIG_EXAMPLES_PWM_DURATION 5
#endif

#ifndef CONFIG_EXAMPLES_PWM_PULSECOUNT
#  define CONFIG_EXAMPLES_PWM_PULSECOUNT 0
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

#endif /* __APPS_EXAMPLES_PWM_PWM_H */
