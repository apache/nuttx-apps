/****************************************************************************
 * apps/examples/foc/foc_adc.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_ADC_H
#define __APPS_EXAMPLES_FOC_FOC_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Additional configuration if ADC support is required */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC

/* AUX ADC samples support */

#  if defined(CONFIG_EXAMPLES_FOC_SETPOINT_ADC) &&   \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define ADC_SAMPLES (2)
#  elif defined(CONFIG_EXAMPLES_FOC_SETPOINT_ADC) || \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define ADC_SAMPLES (1)
#  else
#    define ADC_SAMPLES (0)
#endif

/* Verify ADC FIFO size */

#  if CONFIG_ADC_FIFOSIZE != (ADC_SAMPLES + 1)
#    error Invalid ADC FIFO size
#  endif

/* Numerate ADC samples */

#  if defined(CONFIG_EXAMPLES_FOC_SETPOINT_ADC) &&   \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define VBUS_ADC_SAMPLE      (0)
#    define SETPOINT_ADC_SAMPLE  (1)
#  elif defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define VBUS_ADC_SAMPLE      (0)
#  elif defined(CONFIG_EXAMPLES_FOC_SETPOINT_ADC)
#    define SETPOINT_ADC_SAMPLE  (0)
#  endif

#endif  /* CONFIG_EXAMPLES_FOC_HAVE_ADC */

#endif /* __APPS_EXAMPLES_FOC_FOC_ADC_H */
