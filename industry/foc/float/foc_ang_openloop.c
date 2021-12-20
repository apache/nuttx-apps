/****************************************************************************
 * apps/industry/foc/float/foc_ang_openloop.c
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

/* Open-loop data */

struct foc_openloop_f32_s
{
  struct foc_openloop_cfg_f32_s cfg;
  struct openloop_data_f32_s    data;
  float                         dir;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_ol_init_f32(FAR foc_angle_f32_t *h);
static void foc_angle_ol_deinit_f32(FAR foc_angle_f32_t *h);
static int foc_angle_ol_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg);
static int foc_angle_ol_zero_f32(FAR foc_angle_f32_t *h);
static int foc_angle_ol_dir_f32(FAR foc_angle_f32_t *h, float dir);
static int foc_angle_ol_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle float interface */

struct foc_angle_ops_f32_s g_foc_angle_ol_f32 =
{
  .init   = foc_angle_ol_init_f32,
  .deinit = foc_angle_ol_deinit_f32,
  .cfg    = foc_angle_ol_cfg_f32,
  .zero   = foc_angle_ol_zero_f32,
  .dir    = foc_angle_ol_dir_f32,
  .run    = foc_angle_ol_run_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_ol_init_f32
 *
 * Description:
 *   Initialize the open-loop FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_ol_init_f32(FAR foc_angle_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_openloop_f32_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_ol_deinit_f32
 *
 * Description:
 *   De-initialize the open-loop FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_ol_deinit_f32(FAR foc_angle_f32_t *h)
{
  DEBUGASSERT(h);

  /* Free angle data */

  if (h->data)
    {
      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_angle_ol_cfg_f32
 *
 * Description:
 *   Configure the open-loop FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_openloop_f32_s)
 *
 ****************************************************************************/

static int foc_angle_ol_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg)
{
  FAR struct foc_openloop_f32_s *ol = NULL;

  DEBUGASSERT(h);

  /* Get open-loop data */

  DEBUGASSERT(h->data);
  ol = h->data;

  /* Copy configuration */

  memcpy(&ol->cfg, cfg, sizeof(struct foc_openloop_cfg_f32_s));

  /* Initialize open-loop controller data */

  DEBUGASSERT(ol->cfg.per > 0.0f);
  motor_openloop_init(&ol->data, ol->cfg.per);

  /* Initialize with CW direction */

  ol->dir = DIR_CW;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_ol_zero_f32
 *
 * Description:
 *   Zero the open-loop FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_ol_zero_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_openloop_f32_s *ol = NULL;

  DEBUGASSERT(h);

  /* Get open-loop data */

  DEBUGASSERT(h->data);
  ol = h->data;

  /* Reset angle */

  ol->data.angle = 0.0f;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_ol_dir_f32
 *
 * Description:
 *   Set the open-loop FOC angle handler direction (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_ol_dir_f32(FAR foc_angle_f32_t *h, float dir)
{
  FAR struct foc_openloop_f32_s *ol = NULL;

  DEBUGASSERT(h);

  UNUSED(dir);

  /* Get open-loop data */

  DEBUGASSERT(h->data);
  ol = h->data;

  /* Configure direction */

  ol->dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_ol_run_f32
 *
 * Description:
 *   Process the FOC open-loop angle data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_ol_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out)
{
  FAR struct foc_openloop_f32_s *ol = NULL;

  DEBUGASSERT(h);

  /* Get open-loop data */

  DEBUGASSERT(h->data);
  ol = h->data;

  /* Update open-loop */

  motor_openloop(&ol->data, in->vel, in->dir);

  /* Get open-loop angle */

  out->type  = FOC_ANGLE_TYPE_ELE;
  out->angle = ol->dir * motor_openloop_angle_get(&ol->data);

  return OK;
}
