/****************************************************************************
 * apps/examples/foc/foc_cfg.h
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

#ifndef __APPS_EXAMPLES_FOC_FOC_CFG_H
#define __APPS_EXAMPLES_FOC_FOC_CFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_FOC_SENSORLESS) && \
  defined(CONFIG_EXAMPLES_FOC_SENSORED)
#  error Simultaneous support for sensorless and sensored mode not supported
#endif

/* For now only sensorless velocity control supported */

#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
#  ifndef CONFIG_EXAMPLES_FOC_HAVE_VEL
#    error
#  endif
#endif

/* Position controller needs velocity controller */

#if defined(CONFIG_EXAMPLES_FOC_HAVE_POS) &&    \
   !defined(CONFIG_EXAMPLES_FOC_HAVE_VEL)
#  error Position controller needs velocity controller
#endif

/* Velocity controller needs torque controller */

#if defined(CONFIG_EXAMPLES_FOC_HAVE_VEL) &&    \
   !defined(CONFIG_EXAMPLES_FOC_HAVE_TORQ)
#  error Velocity controller needs torque controller
#endif

/* Open-loop configuration */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
#  ifndef CONFIG_EXAMPLES_FOC_HAVE_VEL
#    error open-loop needs CONFIG_EXAMPLES_FOC_HAVE_VEL set
#  endif
#  ifndef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
#    error open-loop needs CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
#  endif
#endif

/* For now only the FOC PI current controller supported */

#ifndef CONFIG_EXAMPLES_FOC_CONTROL_PI
#  error For now only the FOC PI current controller supported
#endif

/* Velocity ramp must be configured */

#if (CONFIG_EXAMPLES_FOC_RAMP_THR == 0)
#  error CONFIG_EXAMPLES_FOC_RAMP_THR not configured
#endif
#if (CONFIG_EXAMPLES_FOC_RAMP_ACC == 0)
#  error CONFIG_EXAMPLES_FOC_RAMP_ACC not configured
#endif
#if (CONFIG_EXAMPLES_FOC_RAMP_DEC == 0)
#  error CONFIG_EXAMPLES_FOC_RAMP_DEC not configured
#endif

/* Motor identification support */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
#  if (CONFIG_EXAMPLES_FOC_IDENT_RES_CURRENT == 0)
#    error CONFIG_EXAMPLES_FOC_IDENT_RES_CURRENT not configured
#  endif
#  if (CONFIG_EXAMPLES_FOC_IDENT_RES_KI == 0)
#    error CONFIG_EXAMPLES_FOC_IDENT_RES_KI not configured
#  endif
#  if (CONFIG_EXAMPLES_FOC_IDENT_IND_VOLTAGE == 0)
#    error CONFIG_EXAMPLES_FOC_IDENT_IND_VOLTAGE not configured
#  endif
#  if (CONFIG_EXAMPLES_FOC_IDENT_RES_SEC == 0)
#    error CONFIG_EXAMPLES_FOC_IDENT_RES_SEC not configured
#  endif
#  if (CONFIG_EXAMPLES_FOC_IDENT_IND_SEC == 0)
#    error CONFIG_EXAMPLES_FOC_IDENT_IND_SEC not configured
#  endif
#endif

/* Printer prescaler */

#if defined(CONFIG_INDUSTRY_FOC_HANDLER_PRINT) && \
  (CONFIG_EXAMPLES_FOC_STATE_PRINT_FREQ > 0)
#  define FOC_STATE_PRINT_PRE (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ / \
                               CONFIG_EXAMPLES_FOC_STATE_PRINT_FREQ)
#else
#  undef FOC_STATE_PRINT_PRE
#endif

/* Velocity ramp configuration */

#define RAMP_CFG_THR (CONFIG_EXAMPLES_FOC_RAMP_THR / 1.0f)

#ifdef CONFIG_EXAMPLES_FOC_STATE_USE_MODEL_PMSM

/* PMSM model parameters */

#  define FOC_MODEL_POLES 7
#  define FOC_MODEL_LOAD  (1.0f)
#  define FOC_MODEL_RES   (0.11f)
#  define FOC_MODEL_IND   (0.0002f)
#  define FOC_MODEL_INER  (0.1f)
#  define FOC_MODEL_FLUX  (0.001f)
#  define FOC_MODEL_INDD  (0.0002f)
#  define FOC_MODEL_INDQ  (0.0002f)
#endif

/* Qenco configuration */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_QENCO
#  if CONFIG_EXAMPLES_FOC_MOTOR_POLES == 0
#    error CONFIG_EXAMPLES_FOC_MOTOR_POLES must be defined
#  endif
#  if CONFIG_EXAMPLES_FOC_QENCO_POSMAX == 0
#    error CONFIG_EXAMPLES_FOC_QENCO_POSMAX must be defined
#  endif
#endif

/* Setpoint source must be specified */

#if !defined(CONFIG_EXAMPLES_FOC_SETPOINT_CONST) &&  \
    !defined(CONFIG_EXAMPLES_FOC_SETPOINT_ADC) && \
    !defined(CONFIG_EXAMPLES_FOC_SETPOINT_CHAR)
#  error setpoint source not selected
#endif

/* Setpoint ADC scale factor */

#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_ADC
#  define SETPOINT_INTF_SCALE (1.0f / CONFIG_EXAMPLES_FOC_ADC_MAX)
#endif

/* If constant setpoint is selected, setpoint value must be provided */

#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CONST
#  define SETPOINT_INTF_SCALE   (1)
#  if CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE == 0
#    error CONFIG_EXAMPLES_FOC_SETPOINT_CONST_VALUE not configured
#  endif
#endif

/* CHARCTRL setpoint control */

#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CHAR
#  define SETPOINT_INTF_SCALE  (1.0f / (CONFIG_EXAMPLES_FOC_SETPOINT_MAX / 1000.0f))
#endif

/* VBUS source must be specified */

#if !defined(CONFIG_EXAMPLES_FOC_VBUS_CONST) &&  \
    !defined(CONFIG_EXAMPLES_FOC_VBUS_ADC)
#  error no VBUS source selected !
#endif

/* VBUS ADC scale factor */

#ifdef CONFIG_EXAMPLES_FOC_VBUS_ADC
#  define VBUS_ADC_SCALE (CONFIG_EXAMPLES_FOC_ADC_VREF *    \
                          CONFIG_EXAMPLES_FOC_VBUS_SCALE /  \
                          CONFIG_EXAMPLES_FOC_ADC_MAX /     \
                          1000.0f /                         \
                          1000.0f)
#endif

/* If constant VBUS is selected, VBUS value must be provided */

#ifdef CONFIG_EXAMPLES_FOC_VBUS_CONST
#  define VBUS_ADC_SCALE   (1)
#  define VBUS_CONST_VALUE (CONFIG_EXAMPLES_FOC_VBUS_CONST_VALUE / 1000.0f)
#  if CONFIG_EXAMPLES_FOC_VBUS_CONST_VALUE == 0
#    error CONFIG_EXAMPLES_FOC_VBUS_CONST_VALUE not configured
#  endif
#endif

/* Velocity controller prescaler */

#define VEL_CONTROL_PRESCALER (CONFIG_EXAMPLES_FOC_NOTIFIER_FREQ /  \
                               CONFIG_EXAMPLES_FOC_VELCTRL_FREQ)

/* Open-loop to observer angle merge factor */

#ifdef CONFIG_EXAMPLES_FOC_SENSORLESS
#  if CONFIG_EXAMPLES_FOC_ANGOBS_MERGE_RATIO > 0
#    define ANGLE_MERGE_FACTOR (CONFIG_EXAMPLES_FOC_ANGOBS_MERGE_RATIO / 100.0f)
#  endif
#endif

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

struct foc_thr_cfg_s
{
  int      fmode;               /* FOC control mode */
  int      mmode;               /* Motor control mode */
#ifdef CONFIG_EXAMPLES_FOC_HAVE_OPENLOOP
  uint32_t qparam;              /* Open-loop Q setting (x1000) */
  bool     ol_force;            /* Force open-loop */
#  ifdef CONFIG_EXAMPLES_FOC_ANGOBS
  uint32_t ol_thr;             /* Observer vel threshold [x1] */
  uint32_t ol_hys;             /* Observer vel hysteresys [x1] */
#  endif
#endif

#ifdef CONFIG_EXAMPLES_FOC_CONTROL_PI
  uint32_t foc_pi_kp;           /* FOC PI Kp (x1000) */
  uint32_t foc_pi_ki;           /* FOC PI Ki (x1000) */
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_TORQ
  uint32_t torqmax;             /* Torque max (x1000) */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  uint32_t velmax;              /* Velocity max (x1000) */
  uint32_t acc;                 /* Acceleration (x1) */
  uint32_t dec;                 /* Deceleration (x1) */
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_POS
  uint32_t posmax;              /* Position max (x1000) */
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_IDENT
  uint32_t ident_res_ki;        /* Ident res Ki (x1000) */
  uint32_t ident_res_curr;      /* Ident res current (x1000) */
  uint32_t ident_res_sec;       /* Ident res sec */
  uint32_t ident_ind_volt;      /* Ident res voltage (x1000) */
  uint32_t ident_ind_sec;       /* Ident ind sec */
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_VEL
  uint32_t vel_filter;          /* Velocity filter (x1000) */
#endif

#ifdef CONFIG_EXAMPLES_FOC_VELOBS_PLL
  uint32_t vel_pll_kp;          /* Vel PLL observer Kp (x1000) */
  uint32_t vel_pll_ki;          /* Vel PLL observer Ki (x1000) */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELOBS_DIV
  uint32_t vel_div_samples;     /* Vel DIV observer samples */
  uint32_t vel_div_filter;      /* Vel DIV observer filter (x1000) */
#endif
#ifdef CONFIG_EXAMPLES_FOC_VELCTRL_PI
  uint32_t vel_pi_kp;           /* Vel controller PI Kp (x1000000) */
  uint32_t vel_pi_ki;           /* Vel controller PI Ki (x1000000) */
#endif

#ifdef CONFIG_EXAMPLES_FOC_ANGOBS_NFO
  uint32_t ang_nfo_slow;        /* Ang NFO slow gain (x1) */
  uint32_t ang_nfo_gain;        /* Ang NFO gain (x1) */
#endif
};

#endif /* __APPS_EXAMPLES_FOC_FOC_CFG_H */
