/****************************************************************************
 * apps/industry/foc/fixed16/foc_vel_odiv.c
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

#include <dspb16.h>

#include "industry/foc/foc_common.h"
#include "industry/foc/foc_log.h"
#include "industry/foc/fixed16/foc_velocity.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Div private data */

struct foc_div_b16_s
{
  struct foc_vel_div_b16_cfg_s     cfg;
  struct motor_sobserver_div_b16_s data;
  struct motor_sobserver_b16_s     o;
  b16_t                            sensor_dir;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_velocity_div_init_b16(FAR foc_velocity_b16_t *h);
static void foc_velocity_div_deinit_b16(FAR foc_velocity_b16_t *h);
static int foc_velocity_div_cfg_b16(FAR foc_velocity_b16_t *h,
                                    FAR void *cfg);
static int foc_velocity_div_zero_b16(FAR foc_velocity_b16_t *h);
static int foc_velocity_div_dir_b16(FAR foc_velocity_b16_t *h, b16_t dir);
static int foc_velocity_div_run_b16(FAR foc_velocity_b16_t *h,
                                    FAR struct foc_velocity_in_b16_s *in,
                                    FAR struct foc_velocity_out_b16_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC velocity b16_t interface */

struct foc_velocity_ops_b16_s g_foc_velocity_odiv_b16 =
{
  .init   = foc_velocity_div_init_b16,
  .deinit = foc_velocity_div_deinit_b16,
  .cfg    = foc_velocity_div_cfg_b16,
  .zero   = foc_velocity_div_zero_b16,
  .dir    = foc_velocity_div_dir_b16,
  .run    = foc_velocity_div_run_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_velocity_div_init_b16
 *
 * Description:
 *   Initialize the DIV velocity observer (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC velocity handler
 *
 ****************************************************************************/

static int foc_velocity_div_init_b16(FAR foc_velocity_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect velocity data */

  h->data = zalloc(sizeof(struct foc_div_b16_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_velocity_div_deinit_b16
 *
 * Description:
 *   De-initialize the DIV velocity observer (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC velocity handler
 *
 ****************************************************************************/

static void foc_velocity_div_deinit_b16(FAR foc_velocity_b16_t *h)
{
  DEBUGASSERT(h);

  if (h->data)
    {
      /* Free velocity data */

      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_velocity_div_cfg_b16
 *
 * Description:
 *   Configure the DIV velocity observer (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   cfg - pointer to velocity handler configuration data
 *         (struct foc_div_b16_s)
 *
 ****************************************************************************/

static int foc_velocity_div_cfg_b16(FAR foc_velocity_b16_t *h, FAR void *cfg)
{
  FAR struct foc_div_b16_s *div = NULL;
  int                       ret = OK;

  DEBUGASSERT(h);

  /* Get div data */

  DEBUGASSERT(h->data);
  div = h->data;

  /* Copy configuration */

  memcpy(&div->cfg, cfg, sizeof(struct foc_vel_div_b16_cfg_s));

  /* Configure observer */

  motor_sobserver_div_init_b16(&div->data,
                               div->cfg.samples,
                               div->cfg.filter,
                               div->cfg.per);

  motor_sobserver_init_b16(&div->o, &div->data, div->cfg.per);

  /* Initialize with CW direction */

  div->sensor_dir = DIR_CW_B16;

  return ret;
}

/****************************************************************************
 * Name: foc_velocity_div_zero_b16
 *
 * Description:
 *   Zero the DIV velocity observer (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *
 ****************************************************************************/

static int foc_velocity_div_zero_b16(FAR foc_velocity_b16_t *h)
{
  FAR struct foc_div_b16_s *div = NULL;
  int                       ret = OK;

  DEBUGASSERT(h);

  /* Get div data */

  DEBUGASSERT(h->data);
  div = h->data;

  /* Reinitialize observer */

  motor_sobserver_div_init_b16(&div->data,
                               div->cfg.samples,
                               div->cfg.filter,
                               div->cfg.per);

  return ret;
}

/****************************************************************************
 * Name: foc_velocity_div_dir_b16
 *
 * Description:
 *   Set the DIV velocity observer direction (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_velocity_div_dir_b16(FAR foc_velocity_b16_t *h, b16_t dir)
{
  FAR struct foc_div_b16_s *div = NULL;

  DEBUGASSERT(h);

  /* Get div data */

  DEBUGASSERT(h->data);
  div = h->data;

  /* Set direction */

  div->sensor_dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_velocity_div_run_b16
 *
 * Description:
 *   Process the DIV velocity observer (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC velocity handler
 *   in  - pointer to FOC velocity handler input data
 *   out - pointer to FOC velocity handler output data
 *
 ****************************************************************************/

static int foc_velocity_div_run_b16(FAR foc_velocity_b16_t *h,
                                    FAR struct foc_velocity_in_b16_s *in,
                                    FAR struct foc_velocity_out_b16_s *out)
{
  FAR struct foc_div_b16_s *div = NULL;

  DEBUGASSERT(h);

  /* Get div data */

  DEBUGASSERT(h->data);
  div = h->data;

  /* Run observer */

  motor_sobserver_div_b16(&div->o, in->angle);

  /* Copy data */

  out->velocity = b16mulb16(div->sensor_dir,
                            motor_sobserver_speed_get_b16(&div->o));

  return OK;
}
