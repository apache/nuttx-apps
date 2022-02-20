/****************************************************************************
 * apps/industry/foc/fixed16/foc_ang_onfo.c
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

#include "industry/foc/foc_log.h"
#include "industry/foc/fixed16/foc_angle.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SIGN(x)	((x > 0) ? b16ONE : -b16ONE)

#define LINEAR_MAP(x, in_min, in_max, out_min, out_max)     \
  (b16divb16(b16mulb16((x - in_min), (out_max - out_min)),  \
             (in_max - in_min)) + out_min)

#ifndef ABS
#  define ABS(a)   (a < 0 ? -a : a)
#endif

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* sensorless observer data */

struct foc_ang_onfo_b16_s
{
  struct foc_angle_onfo_cfg_b16_s  cfg;
  struct motor_aobserver_nfo_b16_s data;
  struct motor_aobserver_b16_s     o;
  b16_t                            sensor_dir;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_onfo_init_b16(FAR foc_angle_b16_t *h);
static void foc_angle_onfo_deinit_b16(FAR foc_angle_b16_t *h);
static int foc_angle_onfo_cfg_b16(FAR foc_angle_b16_t *h, FAR void *cfg);
static int foc_angle_onfo_zero_b16(FAR foc_angle_b16_t *h);
static int foc_angle_onfo_dir_b16(FAR foc_angle_b16_t *h, b16_t dir);
static int foc_angle_onfo_run_b16(FAR foc_angle_b16_t *h,
                                  FAR struct foc_angle_in_b16_s *in,
                                  FAR struct foc_angle_out_b16_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle b16_t interface */

struct foc_angle_ops_b16_s g_foc_angle_onfo_b16 =
{
  .init   = foc_angle_onfo_init_b16,
  .deinit = foc_angle_onfo_deinit_b16,
  .cfg    = foc_angle_onfo_cfg_b16,
  .zero   = foc_angle_onfo_zero_b16,
  .dir    = foc_angle_onfo_dir_b16,
  .run    = foc_angle_onfo_run_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_onfo_init_b16
 *
 * Description:
 *   Initialize the sensorless observer FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_onfo_init_b16(FAR foc_angle_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_ang_onfo_b16_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_onfo_deinit_b16
 *
 * Description:
 *   De-initialize the sensorless observer FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_onfo_deinit_b16(FAR foc_angle_b16_t *h)
{
  DEBUGASSERT(h);

  if (h->data)
    {
      /* Free angle data */

      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_angle_onfo_cfg_b16
 *
 * Description:
 *   Configure the sensorless observer FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_ang_onfo_b16_s)
 *
 ****************************************************************************/

static int foc_angle_onfo_cfg_b16(FAR foc_angle_b16_t *h, FAR void *cfg)
{
  FAR struct foc_ang_onfo_b16_s *ob  = NULL;
  int                            ret = OK;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Copy configuration */

  memcpy(&ob->cfg, cfg, sizeof(struct foc_angle_onfo_cfg_b16_s));

  /* Initialize sensorless observer controller data */

  DEBUGASSERT(ob->cfg.per > 0);

  /* Initialize nolinear fluxlink angle observer */

  motor_aobserver_nfo_init_b16(&ob->data);

  /* Initialize sensorless observer */

  motor_aobserver_init_b16(&ob->o, &ob->data, ob->cfg.per);

  /* Initialize with CW direction */

  ob->sensor_dir = DIR_CW_B16;

  return ret;
}

/****************************************************************************
 * Name: foc_angle_onfo_zero_b16
 *
 * Description:
 *   Zero the sensorless observer FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_onfo_zero_b16(FAR foc_angle_b16_t *h)
{
  FAR struct foc_ang_onfo_b16_s *ob = NULL;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Reinitialize observer data */

  motor_aobserver_nfo_init_b16(&ob->data);

  /* Reset angle */

  ob->o.angle = 0;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_onfo_dir_b16
 *
 * Description:
 *   Set the sensorless observer FOC angle handler direction (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_onfo_dir_b16(FAR foc_angle_b16_t *h, b16_t dir)
{
  FAR struct foc_ang_onfo_b16_s *ob = NULL;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Configure direction */

  ob->sensor_dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_onfo_run_b16
 *
 * Description:
 *   Process the FOC sensorless observer angle data (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_onfo_run_b16(FAR foc_angle_b16_t *h,
                                  FAR struct foc_angle_in_b16_s *in,
                                  FAR struct foc_angle_out_b16_s *out)
{
  FAR struct foc_ang_onfo_b16_s *ob = NULL;
  FAR dq_frame_b16_t v_dq_mod;
  b16_t duty_now;
  b16_t dyn_gain;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Normalize the d-q voltage to get the d-q modulation
   * voltage
   */

  v_dq_mod.d = b16mulb16(in->state->vdq.d, in->state->mod_scale);
  v_dq_mod.q = b16mulb16(in->state->vdq.q, in->state->mod_scale);

  /* Update duty cycle now */

  duty_now = b16mulb16(SIGN(in->state->vdq.q),
                       vector2d_mag_b16(v_dq_mod.d, v_dq_mod.q));

  /* Update and the observer gain. */

  dyn_gain = b16mulb16(LINEAR_MAP(ABS(duty_now),
                                  0,
                                  b16ONE,
                                  b16mulb16(ob->cfg.gain, ob->cfg.gain_slow),
                                  ob->cfg.gain),
                       b16HALF);

  /* Update observer */

  motor_aobserver_nfo_b16(&ob->o, &in->state->iab, &in->state->vab,
                          &ob->cfg.phy, dyn_gain);

  /* Copy data */

  out->type  = FOC_ANGLE_TYPE_ELE;
  out->angle = b16mulb16(ob->sensor_dir,
                         motor_aobserver_angle_get_b16(&ob->o));

  return OK;
}
