/****************************************************************************
 * apps/industry/foc/float/foc_ang_hall.c
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

#include <nuttx/sensors/hall3ph.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/float/foc_angle.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HALL_MAX        (6)
#define ONE_BY_HALL_MAX (1.0f / HALL_MAX)
#define HALL_ANGLE_STEP (ONE_BY_HALL_MAX * MOTOR_ANGLE_E_MAX)

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Hall private data */

struct foc_hall_f32_s
{
  int                       fd;
  int8_t                    sector;
  int8_t                    offset;
  float                     sensor_dir;
  float                     angle;
#ifdef CONFIG_INDUSTRY_FOC_ANGLE_HALL_EST
  int8_t                    sector_last;
  int8_t                    sector_diff;
  int8_t                    sector_diff_last;
  float                     per_acc;
  float                     vel_est;
  float                     angle_diff;
#endif
  struct foc_hall_cfg_f32_s cfg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int foc_angle_hl_init_f32(FAR foc_angle_f32_t *h);
static void foc_angle_hl_deinit_f32(FAR foc_angle_f32_t *h);
static int foc_angle_hl_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg);
static int foc_angle_hl_zero_f32(FAR foc_angle_f32_t *h);
static int foc_angle_hl_dir_f32(FAR foc_angle_f32_t *h, float dir);
static int foc_angle_hl_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* FOC angle float interface */

struct foc_angle_ops_f32_s g_foc_angle_hl_f32 =
{
  .init   = foc_angle_hl_init_f32,
  .deinit = foc_angle_hl_deinit_f32,
  .cfg    = foc_angle_hl_cfg_f32,
  .zero   = foc_angle_hl_zero_f32,
  .dir    = foc_angle_hl_dir_f32,
  .run    = foc_angle_hl_run_f32,
};

/* Sector angles */

static float g_sector_angle[7] =
{
  0 * HALL_ANGLE_STEP,
  1 * HALL_ANGLE_STEP,
  2 * HALL_ANGLE_STEP,
  3 * HALL_ANGLE_STEP,
  4 * HALL_ANGLE_STEP,
  5 * HALL_ANGLE_STEP,
  6 * HALL_ANGLE_STEP
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_angle_hl_decode
 ****************************************************************************/

static int8_t foc_angle_hl_decode_sector(uint8_t hall)
{
  int8_t sector = -1;

#if defined(CONFIG_INDUSTRY_FOC_ANGLE_HALL_120DEG)
  switch (hall)
    {
      case HALL3_120DEG_POS_1:
        {
          sector = 0;
          break;
        }

      case HALL3_120DEG_POS_2:
        {
          sector = 1;
          break;
        }

      case HALL3_120DEG_POS_3:
        {
          sector = 2;
          break;
        }

      case HALL3_120DEG_POS_4:
        {
          sector = 3;
          break;
        }

      case HALL3_120DEG_POS_5:
        {
          sector = 4;
          break;
        }

      case HALL3_120DEG_POS_6:
        {
          sector = 5;
          break;
        }

      default:
        {
          sector = -1;
        }
    }
#elif defined(CONFIG_INDUSTRY_FOC_ANGLE_HALL_60DEG)
  switch (hall)
    {
      case HALL3_60DEG_POS_1:
        {
          sector = 0;
          break;
        }

      case HALL3_60DEG_POS_2:
        {
          sector = 1;
          break;
        }

      case HALL3_60DEG_POS_3:
        {
          sector = 2;
          break;
        }

      case HALL3_60DEG_POS_4:
        {
          sector = 3;
          break;
        }

      case HALL3_60DEG_POS_5:
        {
          sector = 4;
          break;
        }

      case HALL3_60DEG_POS_6:
        {
          sector = 5;
          break;
        }

      default:
        {
          sector = -1;
        }
    }
#else
#  error Invalid configuration
#endif

  return sector;
}

/****************************************************************************
 * Name: foc_angle_hl_sector_get()
 ****************************************************************************/

static uint8_t foc_angle_hl_sector_get(FAR foc_angle_f32_t *h)
{
  FAR struct foc_hall_f32_s *hl     = NULL;
  int                        ret    = OK;
  int8_t                     sector = -1;
  uint8_t                    hall   = 0;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  /* Get the positions from hall */

  ret = ioctl(hl->fd, SNIOC_GET_POSITION, (unsigned long)((uintptr_t)&hall));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: H3PH_POSITION failed, errno=%d\n", errno);
      goto errout;
    }

  /* Decode hall sector */

  sector = foc_angle_hl_decode_sector(hall);
  if (sector == -1)
    {
      goto errout;
    }

errout:
  return sector;
}

/****************************************************************************
 * Name: foc_angle_hl_init_f32
 *
 * Description:
 *   Initialize hall the FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_hl_init_f32(FAR foc_angle_f32_t *h)
{
  int ret = OK;

  DEBUGASSERT(h);

  /* Connect angle data */

  h->data = zalloc(sizeof(struct foc_hall_f32_s));
  if (h->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_hl_deinit_f32
 *
 * Description:
 *   De-initialize hall the FOC angle handler (float32)
 *
 * Input Parameter:
 *   h - pointer to FOC angle handler
 *
 ****************************************************************************/

static void foc_angle_hl_deinit_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_hall_f32_s *hl  = NULL;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  if (h->data)
    {
      /* Close file */

      if (hl->fd > 0)
        {
          close(hl->fd);
        }

      /* Free angle data */

      free(h->data);
    }
}

/****************************************************************************
 * Name: foc_angle_hl_cfg_f32
 *
 * Description:
 *   Configure the hall FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   cfg - pointer to angle handler configuration data
 *         (struct foc_hall_f32_s)
 *
 ****************************************************************************/

static int foc_angle_hl_cfg_f32(FAR foc_angle_f32_t *h, FAR void *cfg)
{
  FAR struct foc_hall_f32_s *hl  = NULL;
  int                        ret = OK;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  /* Copy configuration */

  memcpy(&hl->cfg, cfg, sizeof(struct foc_hall_cfg_f32_s));

  /* Open hall device */

  hl->fd = open(hl->cfg.devpath, O_RDONLY);
  if (hl->fd <= 0)
    {
      FOCLIBERR("ERROR: failed to open %s, errno=%d\n",
                hl->cfg.devpath, errno);
      ret = -errno;
      goto errout;
    }

  /* Initialize with CW direction */

  hl->sensor_dir = DIR_CW;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_hl_zero_f32
 *
 * Description:
 *   Zero the hl FOC angle handler (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *
 ****************************************************************************/

static int foc_angle_hl_zero_f32(FAR foc_angle_f32_t *h)
{
  FAR struct foc_hall_f32_s *hl  = NULL;
  int                        ret = OK;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  /* Get hall offset */

  hl->offset = foc_angle_hl_sector_get(h);
  if (hl->offset == -1)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Reset data */

  hl->angle  = 0.0f;
  hl->sector = 0;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_angle_hl_dir_f32
 *
 * Description:
 *   Set the hl FOC angle handler direction (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   dir - sensor direction (1 if normal -1 if inverted)
 *
 ****************************************************************************/

static int foc_angle_hl_dir_f32(FAR foc_angle_f32_t *h, float dir)
{
  FAR struct foc_hall_f32_s *hl = NULL;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  /* Configure direction */

  hl->sensor_dir = dir;

  return OK;
}

/****************************************************************************
 * Name: foc_angle_hl_run_f32
 *
 * Description:
 *   Process the hall FOC angle data (float32)
 *
 * Input Parameter:
 *   h   - pointer to FOC angle handler
 *   in  - pointer to FOC angle handler input data
 *   out - pointer to FOC angle handler output data
 *
 ****************************************************************************/

static int foc_angle_hl_run_f32(FAR foc_angle_f32_t *h,
                                FAR struct foc_angle_in_f32_s *in,
                                FAR struct foc_angle_out_f32_s *out)
{
  FAR struct foc_hall_f32_s *hl     = NULL;
  int                        ret    = OK;
  int8_t                     sector = -1;

  DEBUGASSERT(h);

  /* Get hall data */

  DEBUGASSERT(h->data);
  hl = h->data;

  /* Get hall sector now */

  sector = foc_angle_hl_sector_get(h);
  if (sector == -1)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Correct sector with offset */

  sector = sector - hl->offset;
  if (sector < 0)
    {
      sector = sector + HALL_MAX;
    }

#ifdef CONFIG_INDUSTRY_FOC_ANGLE_HALL_EST
  /* Store previous state */

  hl->sector_last = hl->sector;
#endif

  /* Store current sector */

  hl->sector = sector;

#ifdef CONFIG_INDUSTRY_FOC_ANGLE_HALL_EST
  /* Sector diff */

  hl->sector_diff = hl->sector - hl->sector_last;

  /* Handle next sector or estimate angle between sectors */

  if (hl->sector_diff != 0)
    {
      /* Only if per_acc is valid */

      if (hl->per_acc > 0.0f)
        {
          /* Do not update vel_est on boundaries */

          if (hl->sector * hl->sector_last != 0)
            {
              /* Only if velocity direction not changed */

              if (hl->sector_diff * hl->sector_diff_last > 0)
                {
                  /* Next sector - estimate velocity */

                  hl->vel_est = (hl->sector_diff * HALL_ANGLE_STEP /
                                 hl->per_acc);
                }
              else
                {
                  /* Velocity dir changed */

                  hl->vel_est = 0.0f;
                }
            }

          /* Reser accumulators */

          hl->per_acc    = 0.0f;
          hl->angle_diff = 0.0f;
        }

      /* Store last sector diff */

      hl->sector_diff_last = hl->sector_diff;
    }
  else
    {
      /* Accumulate period for velocity estimation */

      hl->per_acc += hl->cfg.per;

      /* Accumulate angle diff */

      hl->angle_diff += hl->vel_est * hl->cfg.per;

      /* Saturate angle diff */

      if (hl->angle_diff > HALL_ANGLE_STEP)
        {
          hl->angle_diff = HALL_ANGLE_STEP;
        }
      else if (hl->angle_diff < -HALL_ANGLE_STEP)
        {
          hl->angle_diff = -HALL_ANGLE_STEP;
        }
    }

  /* Get corrected electrical angle */

  hl->angle = hl->sensor_dir * g_sector_angle[hl->sector] + hl->angle_diff;

#else
  /* Get electrical angle */

  hl->angle = hl->sensor_dir * g_sector_angle[hl->sector];

#endif  /* CONFIG_INDUSTRY_FOC_ANGLE_HALL_EST */

  /* Normalize angle */

  angle_norm_2pi(&hl->angle, MOTOR_ANGLE_E_MIN, MOTOR_ANGLE_E_MAX);

  /* Copy data */

  out->type  = FOC_ANGLE_TYPE_ELE;
  out->angle = hl->angle;

errout:
  return ret;
}
