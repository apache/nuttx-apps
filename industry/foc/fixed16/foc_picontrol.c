/****************************************************************************
 * apps/industry/foc/fixed16/foc_picontrol.c
 * This file implements classical FOC PI current controller for fixed16
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

#include "industry/foc/fixed16/foc_handler.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if CONFIG_MOTOR_FOC_PHASES != 3
#  error
#endif

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* FOC PI fixed16 controller data */

struct foc_picontrol_b16_s
{
  b16_t                     vbase_last; /* Last VBASE sample */
  phase_angle_b16_t         angle;      /* Phase angle */
  struct foc_initdata_b16_s cfg;        /* Controller configuration */
  struct foc_data_b16_s     data;       /* Controller private data */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_control_init_b16(FAR foc_handler_b16_t *h);
static void foc_control_deinit_b16(FAR foc_handler_b16_t *h);
static void foc_control_cfg_b16(FAR foc_handler_b16_t *h, FAR void *cfg);
static void foc_control_input_set_b16(FAR foc_handler_b16_t *h,
                                      FAR b16_t *current,
                                      b16_t vbase,
                                      b16_t angle);
static void foc_control_voltage_run_b16(FAR foc_handler_b16_t *h,
                                        FAR dq_frame_b16_t *dq_ref,
                                        FAR ab_frame_b16_t *v_ab_mod);
static void foc_control_current_run_b16(FAR foc_handler_b16_t *h,
                                        FAR dq_frame_b16_t *dq_ref,
                                        FAR dq_frame_b16_t *vdq_comp,
                                        FAR ab_frame_b16_t *v_ab_mod);
static void foc_control_state_get_b16(FAR foc_handler_b16_t *h,
                                      FAR struct foc_state_b16_s *state);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC control fixed16 interface */

struct foc_control_ops_b16_s g_foc_control_pi_b16 =
{
  .init        = foc_control_init_b16,
  .deinit      = foc_control_deinit_b16,
  .cfg         = foc_control_cfg_b16,
  .input_set   = foc_control_input_set_b16,
  .voltage_run = foc_control_voltage_run_b16,
  .current_run = foc_control_current_run_b16,
  .state_get   = foc_control_state_get_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_control_init_b16
 *
 * Description:
 *   Initialize the FOC PI controller (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static int foc_control_init_b16(FAR foc_handler_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect controller data */

  h->control = zalloc(sizeof(struct foc_picontrol_b16_s));
  if (h->control == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_control_deinit_b16
 *
 * Description:
 *   Deinitialize the FOC PI controller (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC handler
 *
 ****************************************************************************/

static void foc_control_deinit_b16(FAR foc_handler_b16_t *h)
{
  DEBUGASSERT(h);

  /* Free controller data */

  if (h->control)
    {
      free(h->control);
    }
}

/****************************************************************************
 * Name: foc_control_cfg_set_b16
 *
 * Description:
 *   Configure the FOC controller (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC handler
 *   cfg - pointer to controller configuration data
 *         (struct foc_picontrol_b16_s)
 *
 ****************************************************************************/

static void foc_control_cfg_b16(FAR foc_handler_b16_t *h, FAR void *cfg)
{
  FAR struct foc_picontrol_b16_s *foc = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(cfg);

  /* Get controller data */

  DEBUGASSERT(h->control);
  foc = h->control;

  /* Copy data */

  memcpy(&foc->cfg, cfg, sizeof(struct foc_initdata_b16_s));

  /* Initialize FOC controller data */

  foc_init_b16(&foc->data, &foc->cfg);
}

/****************************************************************************
 * Name: foc_control_input_set_b16
 *
 * Description:
 *   Update input for controller (fixed16)
 *
 * Input Parameter:
 *   h       - pointer to FOC handler
 *   current - phase currents in amps
 *   vbase   - base voltage for controller
 *   angle   - phase angle in rad
 *
 ****************************************************************************/

static void foc_control_input_set_b16(FAR foc_handler_b16_t *h,
                                      FAR b16_t *current,
                                      b16_t vbase,
                                      b16_t angle)
{
  FAR struct foc_picontrol_b16_s *foc = NULL;
  abc_frame_b16_t                 i_abc;

  DEBUGASSERT(h);
  DEBUGASSERT(current);

  /* Get controller data */

  DEBUGASSERT(h->control);
  foc = h->control;

  /* Get current in abc frame */

  i_abc.a = current[0];
  i_abc.b = current[1];
  i_abc.c = current[2];

  foc_iabc_update_b16(&foc->data, &i_abc);

  /* Update base voltage only if changed */

  if (foc->vbase_last != vbase)
    {
      /* Update FOC base voltage */

      foc_vbase_update_b16(&foc->data, vbase);

      /* Update last FOC base voltage  */

      foc->vbase_last = vbase;
    }

  /* Update phase angle */

#ifndef CONFIG_INDUSTRY_FOC_CORDIC_ANGLE
  phase_angle_update_b16(&foc->angle, angle);
#else
  foc_cordic_angle_b16(h->fd, &foc->angle, angle);
#endif

  /* Feed the controller with phase angle */

  foc_angle_update_b16(&foc->data, &foc->angle);
}

/****************************************************************************
 * Name: foc_control_voltage_b16
 *
 * Description:
 *   Handle the FOC voltage control (fixed16)
 *
 * Input Parameter:
 *   h        - pointer to FOC handler
 *   dq_ref   - DQ voltage reference frame
 *   v_ab_mod - (out) modulation alpha-veta voltage
 *
 ****************************************************************************/

static void foc_control_voltage_run_b16(FAR foc_handler_b16_t *h,
                                        FAR dq_frame_b16_t *dq_ref,
                                        FAR ab_frame_b16_t *v_ab_mod)
{
  FAR struct foc_picontrol_b16_s *foc     = NULL;
  b16_t                           mag_max = 0;

  DEBUGASSERT(h);
  DEBUGASSERT(dq_ref);
  DEBUGASSERT(v_ab_mod);

  /* Get controller data */

  DEBUGASSERT(h->control);
  foc = h->control;

  /* Get maximum possible voltage DQ vetor magnitude */

  foc_vdq_mag_max_get_b16(&foc->data, &mag_max);

  /* Saturate voltage DQ vector */

#ifndef CONFIG_INDUSTRY_FOC_CORDIC_DQSAT
  dq_saturate_b16(dq_ref, mag_max);
#else
  foc_cordic_dqsat_b16(h->fd, dq_ref, mag_max);
#endif

  /* Call FOC voltage controller */

  foc_voltage_control_b16(&foc->data, dq_ref);

  /* Get output v_ab_mod frame */

  foc_vabmod_get_b16(&foc->data, v_ab_mod);
}

/****************************************************************************
 * Name: foc_control_current_b16
 *
 * Description:
 *   Handle the FOC current control (fixed16)
 *
 *   h        - pointer to FOC handler
 *   dq_ref   - DQ current reference frame
 *   vdq_comp - DQ voltage compensation
 *   v_ab_mod - (out) modulation alpha-veta voltage
 *
 ****************************************************************************/

static void foc_control_current_run_b16(FAR foc_handler_b16_t *h,
                                        FAR dq_frame_b16_t *dq_ref,
                                        FAR dq_frame_b16_t *vdq_comp,
                                        FAR ab_frame_b16_t *v_ab_mod)
{
  FAR struct foc_picontrol_b16_s *foc = NULL;
  dq_frame_b16_t                  v_dq_ref;
  b16_t                           mag_max = 0;

  DEBUGASSERT(h);
  DEBUGASSERT(dq_ref);
  DEBUGASSERT(vdq_comp);
  DEBUGASSERT(v_ab_mod);

  /* Get controller data */

  DEBUGASSERT(h->control);
  foc = h->control;

  /* Reset voltage reference */

  v_dq_ref.d = 0;
  v_dq_ref.q = 0;

  /* Call FOC current controller */

  foc_current_control_b16(&foc->data, dq_ref, vdq_comp, &v_dq_ref);

  /* Get maximum possible voltage DQ vetor magnitude */

  foc_vdq_mag_max_get_b16(&foc->data, &mag_max);

  /* Saturate voltage DQ vector */

#ifndef CONFIG_INDUSTRY_FOC_CORDIC_DQSAT
  dq_saturate_b16(dq_ref, mag_max);
#else
  foc_cordic_dqsat_b16(h->fd, dq_ref, mag_max);
#endif

  /* Call FOC voltage control */

  foc_voltage_control_b16(&foc->data, &v_dq_ref);

  /* Get output v_ab_mod frame */

  foc_vabmod_get_b16(&foc->data, v_ab_mod);
}

/****************************************************************************
 * Name: foc_control_state_get_b16
 *
 * Description:
 *   Get the FOC controller state (fixed16)
 *
 * Input Parameter:
 *   h     - pointer to FOC handler
 *   state - (out) pointer to FOC state data
 *
 ****************************************************************************/

static void foc_control_state_get_b16(FAR foc_handler_b16_t *h,
                                      FAR struct foc_state_b16_s *state)
{
  FAR struct foc_picontrol_b16_s *foc = NULL;

  DEBUGASSERT(h);
  DEBUGASSERT(state);

  /* Get controller data */

  DEBUGASSERT(h->control);
  foc = h->control;

  /* Copy DQ voltage */

  state->vdq.q = foc->data.v_dq.q;
  state->vdq.d = foc->data.v_dq.d;

  /* Copy DQ current */

  state->idq.q = foc->data.i_dq.q;
  state->idq.d = foc->data.i_dq.d;

  /* Copy alpha-beta current */

  state->iab.a = foc->data.i_ab.a;
  state->iab.b = foc->data.i_ab.b;

  /* Copy alpha-beta voltage */

  state->vab.a = foc->data.v_ab.a;
  state->vab.b = foc->data.v_ab.b;

  /* Copy phase current */

  state->curr[0] = foc->data.i_abc.a;
  state->curr[1] = foc->data.i_abc.b;
  state->curr[2] = foc->data.i_abc.c;

  /* Copy phase voltage */

  state->volt[0] = foc->data.v_abc.a;
  state->volt[1] = foc->data.v_abc.b;
  state->volt[2] = foc->data.v_abc.c;
}
