/****************************************************************************
 * apps/industry/foc/float/foc_svm3.c
 * This file implements 3-phase space vector modulation for float32
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "industry/foc/float/foc_handler.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if CONFIG_MOTOR_FOC_PHASES != 3
#  error
#endif

/* Enable current samples correction if 3-shunts */

#if CONFIG_MOTOR_FOC_SHUNTS == 3
#  define FOC_CORRECT_CURRENT_SAMPLES 1
#endif

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* SVM3 data */

struct foc_svm3mod_f32_s
{
  struct foc_mod_cfg_f32_s cfg;
  struct svm3_state_f32_s  state;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_modulation_init_f32(FAR foc_handler_f32_t *h);
static void foc_modulation_deinit_f32(FAR foc_handler_f32_t *h);
static void foc_modulation_cfg_f32(FAR foc_handler_f32_t *h, FAR void *cfg);
static void foc_modulation_current_f32(FAR foc_handler_f32_t *h,
                                       FAR float *curr);
static void foc_modulation_vbase_get_f32(FAR foc_handler_f32_t *h,
                                         float vbus,
                                         FAR float *vbase);
static void foc_modulation_run_f32(FAR foc_handler_f32_t *h,
                                   FAR ab_frame_f32_t *v_ab_mod,
                                   FAR float *duty);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC modulation float32 interface */

struct foc_modulation_ops_f32_s g_foc_mod_svm3_f32 =
{
  .init      = foc_modulation_init_f32,
  .deinit    = foc_modulation_deinit_f32,
  .cfg       = foc_modulation_cfg_f32,
  .current   = foc_modulation_current_f32,
  .vbase_get = foc_modulation_vbase_get_f32,
  .run       = foc_modulation_run_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_modulation_init_f32
 *
 * Description:
 *   Initialize the SVM3 modulation (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static int foc_modulation_init_f32(FAR foc_handler_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect modulation data */

  h->modulation = zalloc(sizeof(struct foc_svm3mod_f32_s));
  if (h->modulation == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_modulation_deinit_f32
 *
 * Description:
 *   Deinitialize the SVM3 modulation (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static void foc_modulation_deinit_f32(FAR foc_handler_f32_t *h)
{
  DEBUGASSERT(h);

  /* Free modulation data */

  if (h->modulation)
    {
      free(h->modulation);
    }
}

/****************************************************************************
 * Name: foc_modulation_cfg_f32
 *
 * Description:
 *   Configure the SVM3 modulation (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC handler
 *   cfg - pointer to modulation configuration data
 *         (struct foc_svm3mod_f32_s)
 *
 ****************************************************************************/

static void foc_modulation_cfg_f32(FAR foc_handler_f32_t *h, FAR void *cfg)
{
  FAR struct foc_svm3mod_f32_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Copy data */

  memcpy(&svm->cfg, cfg, sizeof(struct foc_mod_cfg_f32_s));

  /* Initialize SVM3 data */

  svm3_init(&svm->state);
}

/****************************************************************************
 * Name: foc_modulation_vbase_get_f32
 *
 * Description:
 *   Get the modulation base voltage (float32)
 *
 * Input Parameter:
 *   h     - pointer to FOC handler
 *   vbus  - bus voltage
 *   vbase - (out) pointer to base voltage from modulation
 *
 ****************************************************************************/

static void foc_modulation_vbase_get_f32(FAR foc_handler_f32_t *h,
                                         float vbus,
                                         FAR float *vbase)
{
  DEBUGASSERT(h);
  DEBUGASSERT(vbus);
  DEBUGASSERT(vbase);

  /* Get maximum possible base voltage */

  *vbase = SVM3_BASE_VOLTAGE_GET(vbus);
}

/****************************************************************************
 * Name: foc_modulation_current_f32
 *
 * Description:
 *   Correct current samples according to the SVM3 modulation state (float32)
 *
 * Input Parameter:
 *   h    - pointer to FOC handler
 *   curr - (in/out) phase currents
 *
 ****************************************************************************/

static void foc_modulation_current_f32(FAR foc_handler_f32_t *h,
                                       FAR float *curr)
{
#ifdef FOC_CORRECT_CURRENT_SAMPLES
  FAR struct foc_svm3mod_f32_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(curr);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Correct ADC current samples */

  svm3_current_correct(&svm->state,
                       &curr[0],
                       &curr[1],
                       &curr[2]);
#endif
}

/****************************************************************************
 * Name: foc_modulation_f32
 *
 * Description:
 *   Handle the SVM3 modulation (float32)
 *
 * Input Parameter:
 *   h        - pointer to FOC handler
 *   v_ab_mod - requested modulation voltage scaled to 0.0-1.0 range
 *   duty     - (out) duty cycle for switches
 *
 ****************************************************************************/

static void foc_modulation_run_f32(FAR foc_handler_f32_t *h,
                                   FAR ab_frame_f32_t *v_ab_mod,
                                   FAR float *duty)
{
  FAR struct foc_svm3mod_f32_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(v_ab_mod);
  DEBUGASSERT(duty);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Call 3-phase space vector modulation */

  svm3(&svm->state, v_ab_mod);

  /* Copy duty cycle */

  duty[0] = svm->state.d_u;
  duty[1] = svm->state.d_v;
  duty[2] = svm->state.d_w;

  /* Saturate duty cycle */

  f_saturate(&duty[0], 0.0f, svm->cfg.pwm_duty_max);
  f_saturate(&duty[1], 0.0f, svm->cfg.pwm_duty_max);
  f_saturate(&duty[2], 0.0f, svm->cfg.pwm_duty_max);
}
