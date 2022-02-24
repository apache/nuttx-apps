/****************************************************************************
 * apps/industry/foc/float/foc_ang_osmo.c
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
#include "industry/foc/float/foc_angle.h"

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* sensorless observer data */

struct foc_ang_osmo_f32_s
{
  struct foc_angle_osmo_cfg_f32_s  cfg;
  struct motor_aobserver_smo_f32_s data;
  struct motor_aobserver_f32_s     o;
  float                            sensor_dir;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_osmo_init_f32(FAR foc_angle_f32_t *h);
static void foc_angle_osmo_deinit_f32(FAR foc_angle_f32_t *h);
static int foc_angle_osmo_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg);
static int foc_angle_osmo_zero_f32(FAR foc_angle_f32_t *h);
static int foc_angle_osmo_dir_f32(FAR foc_angle_f32_t *h, float dir);
static int foc_angle_osmo_run_f32(FAR foc_angle_f32_t *h,
                                  FAR struct foc_angle_in_f32_s *in,
                                  FAR struct foc_angle_out_f32_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle float interface */

struct foc_angle_ops_f32_s g_foc_angle_osmo_f32 =
{
  .init   = foc_angle_osmo_init_f32,
  .deinit = foc_angle_osmo_deinit_f32,
  .cfg    = foc_angle_osmo_cfg_f32,
  .zero   = foc_angle_osmo_zero_f32,
  .dir    = foc_angle_osmo_dir_f32,
  .run    = foc_angle_osmo_run_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_osmo_init_f32
 *
 * Description:
 *   Initialize the sensorless observer FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_osmo_init_f32(FAR foc_angle_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_ang_osmo_f32_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_osmo_deinit_f32
 *
 * Description:
 *   De-initialize the sensorless observer FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_osmo_deinit_f32(FAR foc_angle_f32_t *h)
{
  DEBUGASSERT(h);

  if (h->data)
    {
      /* Free angle data */

      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_angle_osmo_cfg_f32
 *
 * Description:
 *   Configure the sensorless observer FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_ang_osmo_f32_s)
 *
 ****************************************************************************/

static int foc_angle_osmo_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg)
{
  FAR struct foc_ang_osmo_f32_s *ob  = NULL;
  int                            ret = OK;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Copy configuration */

  memcpy(&ob->cfg, cfg, sizeof(struct foc_angle_osmo_cfg_f32_s));

  /* Initialize sensorless observer controller data */

  DEBUGASSERT(ob->cfg.per > 0.0f);

  /* Initialize SMO angle observer */

  motor_aobserver_smo_init(&ob->data, ob->cfg.k_slide, ob->cfg.err_max);

  /* Initialize sensorless observer */

  motor_aobserver_init(&ob->o, &ob->data, ob->cfg.per);

  /* Initialize with CW direction */

  ob->sensor_dir = DIR_CW;

  return ret;
}

/****************************************************************************
 * Name: foc_angle_osmo_zero_f32
 *
 * Description:
 *   Zero the sensorless observer FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_osmo_zero_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_ang_osmo_f32_s *ob = NULL;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Reinitialize observer data */

  motor_aobserver_smo_init(&ob->data, ob->cfg.k_slide, ob->cfg.err_max);

  /* Reset angle */

  ob->o.angle = 0.0f;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_osmo_dir_f32
 *
 * Description:
 *   Set the sensorless observer FOC angle handler direction (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_osmo_dir_f32(FAR foc_angle_f32_t *h, float dir)
{
  FAR struct foc_ang_osmo_f32_s *ob = NULL;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Configure direction */

  ob->sensor_dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_osmo_run_f32
 *
 * Description:
 *   Process the FOC sensorless observer angle data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_osmo_run_f32(FAR foc_angle_f32_t *h,
                                  FAR struct foc_angle_in_f32_s *in,
                                  FAR struct foc_angle_out_f32_s *out)
{
  FAR struct foc_ang_osmo_f32_s *ob = NULL;

  DEBUGASSERT(h);

  /* Get sensorless observer data */

  DEBUGASSERT(h->data);
  ob = h->data;

  /* Update observer */

  motor_aobserver_smo(&ob->o, &in->state->iab, &in->state->vab,
                      &ob->cfg.phy, in->dir, in->vel);

  /* Copy data */

  out->type  = FOC_ANGLE_TYPE_ELE;
  out->angle = ob->sensor_dir * motor_aobserver_angle_get(&ob->o);

  return OK;
}
