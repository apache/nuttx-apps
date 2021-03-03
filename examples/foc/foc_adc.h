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

#ifndef __EXAMPLES_FOC_FOC_ADC_H
#define __EXAMPLES_FOC_FOC_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* VBUS source must be specified */

#if defined(CONFIG_EXAMPLES_FOC_VBUS_CONST) &&  \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#  error
#endif

/* Velocity source must be specified */

#if defined(CONFIG_EXAMPLES_FOC_VEL_CONST) &&   \
  defined(CONFIG_EXAMPLES_FOC_VEL_ADC)
#  error
#endif

/* VBUS ADC scale factor */

#ifdef CONFIG_EXAMPLES_FOC_VBUS_ADC
#  define VBUS_ADC_SCALE (CONFIG_EXAMPLES_FOC_ADC_VREF *    \
                          CONFIG_EXAMPLES_FOC_VBUS_SCALE /  \
                          CONFIG_EXAMPLES_FOC_ADC_MAX /     \
                          1000.0f /                         \
                          1000.0f)
#endif

/* Velocity ADC scale factor */

#ifdef CONFIG_EXAMPLES_FOC_VEL_ADC
#  define VEL_ADC_SCALE (1.0f / CONFIG_EXAMPLES_FOC_ADC_MAX)
#endif

/* If constant velocity is selected, velocity value must be provided */

#ifdef CONFIG_EXAMPLES_FOC_VEL_CONST
#  define VEL_ADC_SCALE   (1)
#  if CONFIG_EXAMPLES_FOC_VEL_CONST_VALUE == 0
#    error
#  endif
#endif

/* If constant VBUS is selected, VBUS value must be provided */

#ifdef CONFIG_EXAMPLES_FOC_VBUS_CONST
#  define VBUS_ADC_SCALE   (1)
#  define VBUS_CONST_VALUE (CONFIG_EXAMPLES_FOC_VBUS_CONST_VALUE / 1000.0f)
#  if CONFIG_EXAMPLES_FOC_VBUS_CONST_VALUE == 0
#    error
#  endif
#endif

/* Additional configuration if ADC support is required */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC

/* AUX ADC samples support */

#  if defined(CONFIG_EXAMPLES_FOC_VEL_ADC) &&   \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define ADC_SAMPLES (2)
#  elif defined(CONFIG_EXAMPLES_FOC_VEL_ADC) || \
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

#  if defined(CONFIG_EXAMPLES_FOC_VEL_ADC) &&   \
  defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define VBUS_ADC_SAMPLE (0)
#    define VEL_ADC_SAMPLE  (1)
#  elif defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#    define VBUS_ADC_SAMPLE (0)
#  elif defined(CONFIG_EXAMPLES_FOC_VEL_ADC)
#    define VEL_ADC_SAMPLE  (0)
#  endif

#endif  /* CONFIG_EXAMPLES_FOC_HAVE_ADC */

#endif /* __EXAMPLES_FOC_FOC_ADC_H */
