/****************************************************************************
 * apps/industry/foc/fixed16/foc_model_pmsm.c
 * This file implements PMSM model for fixed16
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

#include "industry/foc/fixed16/foc_model.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if CONFIG_MOTOR_FOC_PHASES != 3
#  error
#endif

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* PMSM model data */

struct foc_model_pmsm_b16_s
{
  b16_t                           one_by_iphadc;
  struct foc_model_pmsm_cfg_b16_s cfg;
  struct pmsm_model_b16_s         model;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_model_pmsm_init_b16(FAR foc_model_b16_t *h);
static void foc_model_pmsm_deinit_b16(FAR foc_model_b16_t *h);
static int foc_model_pmsm_cfg_b16(FAR foc_model_b16_t *h, FAR void *cfg);
static void foc_model_pmsm_elerun_b16(FAR foc_model_b16_t *h,
                                   FAR ab_frame_b16_t *v_ab);
static void foc_model_pmsm_mechrun_b16(FAR foc_model_b16_t *h, b16_t load);
static void foc_model_pmsm_state_b16(FAR foc_model_b16_t *h,
                                   FAR struct foc_model_state_b16_s *state);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC model fixed16 interface */

struct foc_model_ops_b16_s g_foc_model_pmsm_ops_b16 =
{
  .init      = foc_model_pmsm_init_b16,
  .deinit    = foc_model_pmsm_deinit_b16,
  .cfg       = foc_model_pmsm_cfg_b16,
  .ele_run   = foc_model_pmsm_elerun_b16,
  .mech_run  = foc_model_pmsm_mechrun_b16,
  .state     = foc_model_pmsm_state_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_model_pmsm_init_b16
 *
 * Description:
 *   Initialize PMSM model (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC model handler
 *
 ****************************************************************************/

static int foc_model_pmsm_init_b16(FAR foc_model_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect model data */

  h->model = zalloc(sizeof(struct foc_model_pmsm_b16_s));
  if (h->model == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_model_pmsm_deinit_b16
 *
 * Description:
 *   Deinitialize PMSM model (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC model handler
 *
 ****************************************************************************/

static void foc_model_pmsm_deinit_b16(FAR foc_model_b16_t *h)
{
  DEBUGASSERT(h);

  /* Free model data */

  if (h->model)
    {
      free (h->model);
    }
}

/****************************************************************************
 * Name: foc_model_pmsm_cfg_b16
 *
 * Description:
 *   Configure PMSM model (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC model handler
 *   cfg - pointer to FOC model configuration (struct foc_model_pmsm_b16_s)
 *
 ****************************************************************************/

static int foc_model_pmsm_cfg_b16(FAR foc_model_b16_t *h, FAR void *cfg)
{
  FAR struct foc_model_pmsm_b16_s *model = NULL;
  struct pmsm_phy_params_b16_s     phy;

  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  /* Get model data */

  DEBUGASSERT(h->model);
  model = h->model;

  /* Copy data */

  memcpy(&model->cfg, cfg, sizeof(struct foc_model_pmsm_cfg_b16_s));

  /* Initialize model */

  pmsm_phy_params_init_b16(&phy,
                           model->cfg.poles,
                           model->cfg.res,
                           model->cfg.ind,
                           model->cfg.iner,
                           model->cfg.flux_link,
                           model->cfg.ind_q,
                           model->cfg.ind_d);

  pmsm_model_initialize_b16(&model->model, &phy, model->cfg.per);

  /* Get one by iphase_adc */

  model->one_by_iphadc = b16divb16(b16ONE, model->cfg.iphase_adc);

  return OK;
}

/****************************************************************************
 * Name: foc_model_pmsm_elerun_b16
 *
 * Description:
 *   Run model electrical simulation (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *   v_ab - applied voltage in alpha-beta frame
 *
 ****************************************************************************/

static void foc_model_pmsm_elerun_b16(FAR foc_model_b16_t *h,
                                      FAR ab_frame_b16_t *v_ab)
{
  FAR struct foc_model_pmsm_b16_s *model = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(v_ab);

  /* Get model data */

  DEBUGASSERT(h->model);
  model = h->model;

  /* Run electrical model */

  pmsm_model_elec_b16(&model->model, v_ab);
}

/****************************************************************************
 * Name: foc_model_pmsm_mechrun_b16
 *
 * Description:
 *   Run model mechanical simulation (fixed16)
 *
 * Input Parameter:
 *   h    - pointer to FOC model handler
 *   load - applied load
 *
 ****************************************************************************/

static void foc_model_pmsm_mechrun_b16(FAR foc_model_b16_t *h, b16_t load)
{
  FAR struct foc_model_pmsm_b16_s *model = NULL;

  DEBUGASSERT(h);

  /* Get model data */

  DEBUGASSERT(h->model);
  model = h->model;

  /* Run mechanical model */

  pmsm_model_mech_b16(&model->model, load);
}

/****************************************************************************
 * Name: foc_model_pmsm_state_b16
 *
 * Description:
 *   Get the model state (fixed16)
 *
 * Input Parameter:
 *   h     - pointer to FOC model handler
 *   state - pointer to FOC model state
 *
 ****************************************************************************/

static void foc_model_pmsm_state_b16(FAR foc_model_b16_t *h,
                                     FAR struct foc_model_state_b16_s *state)
{
  FAR struct foc_model_pmsm_b16_s *model = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(state);

  /* Get model data */

  DEBUGASSERT(h->model);
  model = h->model;

  /* Get model state */

  state->volt[0] = model->model.state.v_abc.a;
  state->volt[1] = model->model.state.v_abc.b;
  state->volt[2] = model->model.state.v_abc.c;
  state->vab.a   = model->model.state.v_ab.a;
  state->vab.b   = model->model.state.v_ab.b;
  state->vdq.d   = model->model.state.v_dq.d;
  state->vdq.q   = model->model.state.v_dq.q;
  state->curr[0] = model->model.state.i_abc.a;
  state->curr[1] = model->model.state.i_abc.b;
  state->curr[2] = model->model.state.i_abc.c;
  state->iab.a   = model->model.state.i_ab.a;
  state->iab.b   = model->model.state.i_ab.b;
  state->idq.d   = model->model.state.i_dq.d;
  state->idq.q   = model->model.state.i_dq.q;
  state->omega_e = model->model.state.omega_e;
  state->omega_m = model->model.state.omega_m;

  /* Get RAW currents */

  state->curr_raw[0] = b16toi(b16mulb16(state->curr[0],
                                        model->one_by_iphadc));
  state->curr_raw[1] = b16toi(b16mulb16(state->curr[1],
                                        model->one_by_iphadc));
  state->curr_raw[2] = b16toi(b16mulb16(state->curr[2],
                                        model->one_by_iphadc));
}
