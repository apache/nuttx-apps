/****************************************************************************
 * apps/industry/foc/fixed16/foc_svm3.c
 * This file implements 3-phase space vector modulation for fixed16
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

#include "industry/foc/fixed16/foc_handler.h"

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
 * Private Function Prototypes
 ****************************************************************************/

/* SVM3 data */

struct foc_svm3mod_b16_s
{
  struct foc_mod_cfg_b16_s cfg;
  struct svm3_state_b16_s state;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_modulation_init_b16(FAR foc_handler_b16_t *h);
static void foc_modulation_deinit_b16(FAR foc_handler_b16_t *h);
static void foc_modulation_cfg_b16(FAR foc_handler_b16_t *h, FAR void *cfg);
static void foc_modulation_current_b16(FAR foc_handler_b16_t *h,
                                       FAR b16_t *curr);
static void foc_modulation_vbase_get_b16(FAR foc_handler_b16_t *h,
                                         b16_t vbus,
                                         FAR b16_t *vbase);
static void foc_modulation_run_b16(FAR foc_handler_b16_t *h,
                                   FAR ab_frame_b16_t *v_ab_mod,
                                   FAR b16_t *duty);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC modulation fixed16 interface */

struct foc_modulation_ops_b16_s g_foc_mod_svm3_b16 =
{
  .init      = foc_modulation_init_b16,
  .deinit    = foc_modulation_deinit_b16,
  .cfg       = foc_modulation_cfg_b16,
  .current   = foc_modulation_current_b16,
  .vbase_get = foc_modulation_vbase_get_b16,
  .run       = foc_modulation_run_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_modulation_init_b16
 *
 * Description:
 *   Initialize the SVM3 modulation (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static int foc_modulation_init_b16(FAR foc_handler_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect modulation data */

  h->modulation = zalloc(sizeof(struct foc_svm3mod_b16_s));
  if (h->modulation == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

  errout:
  return ret;
}

/****************************************************************************
 * Name: foc_modulation_deinit_b16
 *
 * Description:
 *   Deinitialize the SVM3 modulation (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static void foc_modulation_deinit_b16(FAR foc_handler_b16_t *h)
{
  DEBUGASSERT(h);

  /* Free modulation data */

  if (h->modulation)
    {
      free(h->modulation);
    }
}

/****************************************************************************
 * Name: foc_modulation_cfg_b16
 *
 * Description:
 *   Configure the SVM3 modulation (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC handler
 *   cfg - pointer to modulation configuration data
 *         (struct foc_svm3mod_b16_s)
 *
 ****************************************************************************/

static void foc_modulation_cfg_b16(FAR foc_handler_b16_t *h, FAR void *cfg)
{
  FAR struct foc_svm3mod_b16_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Copy data */

  memcpy(&svm->cfg, cfg, sizeof(struct foc_mod_cfg_b16_s));

  /* Initialize SVM3 data */

  svm3_init_b16(&svm->state);
}

/****************************************************************************
 * Name: foc_modulation_vbase_get_b16
 *
 * Description:
 *   Get the modulation base voltage (fixed16)
 *
 * Input Parameter:
 *   h     - pointer to FOC handler
 *   vbus  - bus voltage
 *   vbase - (out) pointer to base voltage from modulation
 *
 ****************************************************************************/

static void foc_modulation_vbase_get_b16(FAR foc_handler_b16_t *h,
                                         b16_t vbus,
                                         FAR b16_t *vbase)
{
  DEBUGASSERT(h);
  DEBUGASSERT(vbus);
  DEBUGASSERT(vbase);

  /* Get maximum possible base voltage */

  *vbase = SVM3_BASE_VOLTAGE_GET_B16(vbus);
}

/****************************************************************************
 * Name: foc_modulation_current_b16
 *
 * Description:
 *   Correct current samples according to the SVM3 modulation state (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC handler
 *   curr - (in/out) phase currents
 *
 ****************************************************************************/

static void foc_modulation_current_b16(FAR foc_handler_b16_t *h,
                                       FAR b16_t *curr)
{
#ifdef FOC_CORRECT_CURRENT_SAMPLES
  FAR struct foc_svm3mod_b16_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(curr);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Correct ADC current samples */

  svm3_current_correct_b16(&svm->state,
                           &curr[0],
                           &curr[1],
                           &curr[2]);
#endif
}

/****************************************************************************
 * Name: foc_modulation_b16
 *
 * Description:
 *   Handle the SVM3 modulation (fixed16)
 *
 * Input Parameter:
 *   h        - pointer to FOC handler
 *   v_ab_mod - requested modulation voltage scaled to 0.0-1.0 range
 *   duty     - (out) duty cycle for switches
 *
 ****************************************************************************/

static void foc_modulation_run_b16(FAR foc_handler_b16_t *h,
                                   FAR ab_frame_b16_t *v_ab_mod,
                                   FAR b16_t *duty)
{
  FAR struct foc_svm3mod_b16_s *svm = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(v_ab_mod);
  DEBUGASSERT(duty);

  /* Get modulation data */

  DEBUGASSERT(h->modulation);
  svm = h->modulation;

  /* Call 3-phase space vector modulation */

  svm3_b16(&svm->state, v_ab_mod);

  /* Copy duty cycle */

  duty[0] = svm->state.d_u;
  duty[1] = svm->state.d_v;
  duty[2] = svm->state.d_w;

  /* Saturate duty cycle */

  f_saturate_b16(&duty[0], 0, svm->cfg.pwm_duty_max);
  f_saturate_b16(&duty[1], 0, svm->cfg.pwm_duty_max);
  f_saturate_b16(&duty[2], 0, svm->cfg.pwm_duty_max);
}
