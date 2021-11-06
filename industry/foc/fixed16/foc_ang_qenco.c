/****************************************************************************
 * apps/industry/foc/fixed16/foc_ang_qenco.c
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
#include "industry/foc/fixed16/foc_angle.h"

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Qenco private data */

struct foc_qenco_b16_s
{
  int                        fd;
  int32_t                    pos;
  int32_t                    offset;
  b16_t                      one_by_posmax;
  b16_t                      dir;
  b16_t                      angle;
  struct foc_qenco_cfg_b16_s cfg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_qe_init_b16(FAR foc_angle_b16_t *h);
static void foc_angle_qe_deinit_b16(FAR foc_angle_b16_t *h);
static int foc_angle_qe_cfg_b16(FAR foc_angle_b16_t *h, FAR void *cfg);
static int foc_angle_qe_zero_b16(FAR foc_angle_b16_t *h);
static int foc_angle_qe_dir_b16(FAR foc_angle_b16_t *h, b16_t dir);
static int foc_angle_qe_run_b16(FAR foc_angle_b16_t *h,
                                FAR struct foc_angle_in_b16_s *in,
                                FAR struct foc_angle_out_b16_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle fixed16 interface */

struct foc_angle_ops_b16_s g_foc_angle_qe_b16 =
{
  .init   = foc_angle_qe_init_b16,
  .deinit = foc_angle_qe_deinit_b16,
  .cfg    = foc_angle_qe_cfg_b16,
  .zero   = foc_angle_qe_zero_b16,
  .dir    = foc_angle_qe_dir_b16,
  .run    = foc_angle_qe_run_b16,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_qe_init_b16
 *
 * Description:
 *   Initialize qenco the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_qe_init_b16(FAR foc_angle_b16_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_qenco_b16_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_deinit_b16
 *
 * Description:
 *   De-initialize qenco the FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_qe_deinit_b16(FAR foc_angle_b16_t *h)
{
  FAR struct foc_qenco_b16_s *qe  = NULL;

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
 * Name: foc_angle_qe_cfg_b16
 *
 * Description:
 *   Configure the qenco FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_qenco_b16_s)
 *
 ****************************************************************************/

static int foc_angle_qe_cfg_b16(FAR foc_angle_b16_t *h, FAR void *cfg)
{
  FAR struct foc_qenco_b16_s *qe  = NULL;
  int                         ret = OK;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Copy configuration */

  memcpy(&qe->cfg, cfg, sizeof(struct foc_qenco_cfg_b16_s));

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

  qe->one_by_posmax = b16divi(b16ONE, qe->cfg.posmax);

  /* Initialize with CW direction */

  qe->dir = DIR_CW_B16;

  /* Reset offset */

  qe->offset = 0;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_zero_b16
 *
 * Description:
 *   Zero the qenco FOC angle handler (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_qe_zero_b16(FAR foc_angle_b16_t *h)
{
  FAR struct foc_qenco_b16_s *qe  = NULL;
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

  qe->angle = 0;
  qe->pos   = 0;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_qe_dir_b16
 *
 * Description:
 *   Set the qenco FOC angle handler direction (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_qe_dir_b16(FAR foc_angle_b16_t *h, b16_t dir)
{
  FAR struct foc_qenco_b16_s *qe = NULL;

  DEBUGASSERT(h);

  /* Get qenco data */

  DEBUGASSERT(h->data);
  qe = h->data;

  /* Configure direction */

  qe->dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_qe_run_b16
 *
 * Description:
 *   Process the qenco FOC angle data (fixed16)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_qe_run_b16(FAR foc_angle_b16_t *h,
                                FAR struct foc_angle_in_b16_s *in,
                                FAR struct foc_angle_out_b16_s *out)
{
  FAR struct foc_qenco_b16_s *qe   = NULL;
  int                         ret  = OK;
  b16_t                       tmp1 = 0;
  b16_t                       tmp2 = 0;
  b16_t                       tmp3 = 0;

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

  tmp1 = (qe->pos - qe->offset);
  tmp2 = b16muli(qe->dir, tmp1);
  tmp3 = b16mulb16(qe->one_by_posmax, MOTOR_ANGLE_M_MAX_B16);

  qe->angle = b16mulb16(tmp2, tmp3);

  /* Normalize angle */

  angle_norm_2pi_b16(&qe->angle, MOTOR_ANGLE_M_MIN_B16,
                     MOTOR_ANGLE_M_MAX_B16);

  /* Copy data */

  out->type  = FOC_ANGLE_TYPE_MECH;
  out->angle = qe->angle;

errout:
  return ret;
}
