/****************************************************************************
 * apps/industry/foc/float/foc_ang_qenco.c
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

#include <sys/ioctl.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include <nuttx/sensors/qencoder.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/float/foc_angle.h"

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Qenco private data */

struct foc_qenco_f32_s
{
  int                        fd;
  int32_t                    pos;
  int32_t                    offset;
  float                      one_by_posmax;
  float                      sensor_dir;
  float                      angle;
  struct foc_qenco_cfg_f32_s cfg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_qe_init_f32(FAR foc_angle_f32_t *h);
static void foc_angle_qe_deinit_f32(FAR foc_angle_f32_t *h);
static int foc_angle_qe_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg);
static int foc_angle_qe_zero_f32(FAR foc_angle_f32_t *h);
static int foc_angle_qe_dir_f32(FAR foc_angle_f32_t *h, float dir);
static int foc_angle_qe_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle float interface */

struct foc_angle_ops_f32_s g_foc_angle_qe_f32 =
{
  .init   = foc_angle_qe_init_f32,
  .deinit = foc_angle_qe_deinit_f32,
  .cfg    = foc_angle_qe_cfg_f32,
  .zero   = foc_angle_qe_zero_f32,
  .dir    = foc_angle_qe_dir_f32,
  .run    = foc_angle_qe_run_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_qe_init_f32
 *
 * Description:
 *   Initialize qenco the FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_qe_init_f32(FAR foc_angle_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_qenco_f32_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_deinit_f32
 *
 * Description:
 *   De-initialize qenco the FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_qe_deinit_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_qenco_f32_s *qe  = NULL;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  if (h->data)
    {
      /* Close file */

      if (qe->fd > 0)
        {
          close(qe->fd);
        }

      /* Free angle data */

      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_angle_qe_cfg_f32
 *
 * Description:
 *   Configure the qenco FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_qenco_f32_s)
 *
 ****************************************************************************/

static int foc_angle_qe_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg)
{
  FAR struct foc_qenco_f32_s *qe  = NULL;
  int                         ret = OK;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Copy configuration */

  memcpy(&qe->cfg, cfg, sizeof(struct foc_qenco_cfg_f32_s));

  /* Open qenco device */

  qe->fd = open(qe->cfg.devpath, O_RDONLY);
  if (qe->fd <= 0)
    {
      FOCLIBERR("ERROR: failed to open %s, errno=%d\n",
                qe->cfg.devpath, errno);
      ret = -errno;
      goto errout;
    }

  /* Configure the maximum encoder position */

  ret = ioctl(qe->fd, QEIOC_SETPOSMAX, (unsigned long)(qe->cfg.posmax));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: QEIOC_SETPOSMAX failed, errno=%d\n", errno);
      goto errout;
    }

  /* Set the encoder index position to 0 */

  ret = ioctl(qe->fd, QEIOC_SETINDEX, (unsigned long)(0));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: QEIOC_SETINDEX failed, errno=%d\n", errno);
      goto errout;
    }

  /* Reset encoder position */

  ret = ioctl(qe->fd, QEIOC_RESET, 0);
  if (ret < 0)
    {
      FOCLIBERR("ERROR: QEIOC_RESET failed, errno=%d\n", errno);
      goto errout;
    }

  /* Get helpers */

  qe->one_by_posmax = (1.0f / qe->cfg.posmax);

  /* Initialize with CW direction */

  qe->sensor_dir = DIR_CW;

  /* Reset offset */

  qe->offset = 0.0f;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_zero_f32
 *
 * Description:
 *   Zero the qenco FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_qe_zero_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_qenco_f32_s *qe  = NULL;
  int                         ret = OK;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Get the zero offset position from encoder */

  ret = ioctl(qe->fd, QEIOC_POSITION,
              (unsigned long)((uintptr_t)&qe->offset));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: QEIOC_POSITION failed, errno=%d\n", errno);
      goto errout;
    }

  /* Reset data */

  qe->angle = 0.0f;
  qe->pos   = 0;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_dir_f32
 *
 * Description:
 *   Set the qenco FOC angle handler direction (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_qe_dir_f32(FAR foc_angle_f32_t *h, float dir)
{
  FAR struct foc_qenco_f32_s *qe = NULL;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Configure direction */

  qe->sensor_dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_qe_run_f32
 *
 * Description:
 *   Process the qenco FOC angle data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_qe_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out)
{
  FAR struct foc_qenco_f32_s *qe  = NULL;
  int                         ret = OK;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Get the positions from encoder */

  ret = ioctl(qe->fd, QEIOC_POSITION, (unsigned long)((uintptr_t)&qe->pos));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: QEIOC_POSITION failed, errno=%d\n", errno);
      goto errout;
    }

  /* Get mechanical angle */

  qe->angle = (qe->sensor_dir * (qe->pos - qe->offset) *
               qe->one_by_posmax * MOTOR_ANGLE_M_MAX);

  /* Normalize angle */

  angle_norm_2pi(&qe->angle, MOTOR_ANGLE_M_MIN, MOTOR_ANGLE_M_MAX);

  /* Copy data */

  out->type  = FOC_ANGLE_TYPE_MECH;
  out->angle = qe->angle;

errout:
  return ret;
}
