/****************************************************************************
 * apps/industry/foc/float/foc_align.c
 * This file implements angle sensor alignment routine for float32
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

#include "industry/foc/foc_common.h"
#include "industry/foc/foc_log.h"

#include "industry/foc/float/foc_align.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Align to -90deg = 270deg */

#define FOC_ALIGN_ANGLE        (3*M_PI/2)

/* Direction alignment */

#define ALIGN_DIR_ANGLE_STEP   (0.001f)
#define ALIGN_DIR_ANGLE_HOLD_0 (0.0f)
#define ALIGN_DIR_ANGLE_HOLD_1 (M_PI/3)
#define ALIGN_DIR_ANGLE_HOLD_2 (2*M_PI/3)
#define ALIGN_DIR_ANGLE_HOLD_3 (M_PI)
#define ALIGN_DIR_ANGLE_HOLD_4 (4*M_PI/3)
#define ALIGN_DIR_HOLD_CNTR    (10)

/* IDLE steps */

#define IDLE_STEPS             (500)

/* Index alignment */

#define ALIGN_INDEX_ANGLE_SIGN   (-2.0f*M_PI)
#define ALIGN_INDEX_ANGLE_STEP   (0.001f)
#define ALIGN_INDEX_ANGLE_NOZERO (0.1f)

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Align stage */

enum foc_align_run_stage_e
{
  FOC_ALIGN_RUN_INIT,
#ifdef CONFIG_INDUSTRY_FOC_ALIGN_INDEX
  FOC_ALIGN_RUN_INDEX,
#endif
  FOC_ALIGN_RUN_OFFSET,
  FOC_ALIGN_RUN_DIR,
  FOC_ALIGN_RUN_IDLE,
  FOC_ALIGN_RUN_DONE
};

/* Align routine private data */

struct foc_align_f32_s
{
  struct foc_routine_align_cfg_f32_s   cfg;
  struct foc_routine_aling_final_f32_s final;
  int                                  cntr;
  int                                  stage;
  int                                  dir_step;
#ifdef CONFIG_INDUSTRY_FOC_ALIGN_INDEX
  int                                  index_step;
  float                                index_angle;
#endif
  float                                dir_angle;
  float                                angle_last;
  float                                angle_now;
  float                                diff_cw;
  float                                diff_ccw;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

int foc_routine_align_init_f32(FAR foc_routine_f32_t *r);
void foc_routine_align_deinit_f32(FAR foc_routine_f32_t *r);
int foc_routine_align_cfg_f32(FAR foc_routine_f32_t *r, FAR void *cfg);
int foc_routine_align_run_f32(FAR foc_routine_f32_t *r,
                              FAR struct foc_routine_in_f32_s *in,
                              FAR struct foc_routine_out_f32_s *out);
int foc_routine_align_final_f32(FAR foc_routine_f32_t *r, FAR void *data);

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct foc_routine_ops_f32_s g_foc_routine_align_f32 =
{
  .init   = foc_routine_align_init_f32,
  .deinit = foc_routine_align_deinit_f32,
  .cfg    = foc_routine_align_cfg_f32,
  .run    = foc_routine_align_run_f32,
  .final  = foc_routine_align_final_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_ALIGN_INDEX
/****************************************************************************
 * Name: foc_align_index_run_f32
 *
 * Description:
 *   Run index alignment routine
 *
 * Input Parameter:
 *   align - pointer to FOC align routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

static int foc_align_index_run_f32(FAR struct foc_align_f32_s *align,
                                   FAR struct foc_routine_in_f32_s *in,
                                   FAR struct foc_routine_out_f32_s *out)
{
  static float dir         = DIR_CW;
  float        sign        = 0.0f;
  int          ret         = FOC_ROUTINE_RUN_NOTDONE;

  /* Get mechanical angles */

  align->angle_last = align->angle_now;
  align->angle_now  = in->angle_m;

  /* Normalize angle to <-M_PI, M_PI> range */

  align->angle_now = align->angle_now - M_PI;
  angle_norm_2pi(&align->angle_now, -M_PI, M_PI);

  /* The product of the previous angle with an angle now gives us
   * information about the encoder overflow.
   * This procedure assumes that the encoder index signal resets the
   * encoder value to zero.
   */

  sign = align->angle_now * align->angle_last;

  /* Move the motor shaft to approach index signal from both direction.
   * After that, the encoder zero should be aligned with the encoder index.
   */

  if (align->index_step == 0)
    {
      /* Move the motor shaft away from zero */

      align->index_angle += dir * ALIGN_INDEX_ANGLE_STEP;

      if (align->angle_now < ALIGN_INDEX_ANGLE_NOZERO &&
          align->angle_now > -ALIGN_INDEX_ANGLE_NOZERO)
        {
          align->index_step += 1;
        }
    }

  else if (align->index_step == 1)
    {
      /* Move the motor shaft to zero in positive direction */

      align->index_angle += dir * ALIGN_INDEX_ANGLE_STEP;

      if (sign < 0.0f && sign < ALIGN_INDEX_ANGLE_SIGN)
        {
          dir = DIR_CCW * dir;
          align->index_step += 1;
        }
    }

  else if (align->index_step == 2)
    {
      /* Move the motor shaft away from zero */

      align->index_angle += dir * ALIGN_INDEX_ANGLE_STEP;

      if (align->angle_now < ALIGN_INDEX_ANGLE_NOZERO &&
          align->angle_now > -ALIGN_INDEX_ANGLE_NOZERO)
        {
          align->index_step += 1;
        }
    }

  else if (align->index_step == 3)
    {
      /* Move the motor shaft to zero in negative direction */

      align->index_angle += dir * ALIGN_INDEX_ANGLE_STEP;

      if (sign < 0.0f && sign < ALIGN_INDEX_ANGLE_SIGN)
        {
          dir = DIR_CCW * dir;
          align->index_step += 1;
        }
    }

  else if (align->index_step == 4)
    {
      /* Done */

      ret = FOC_ROUTINE_RUN_DONE;
    }

  /* Get output */

  if (ret == FOC_ROUTINE_RUN_DONE)
    {
      /* Force IDLE mode */

      out->dq_ref.q   = 0.0f;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = 0.0f;
      out->foc_mode   = FOC_HANDLER_MODE_IDLE;

      /* Reset data */

      align->angle_last = 0.0f;
      align->angle_now  = 0.0f;
      align->cntr       = 0;
    }
  else
    {
      /* Force DQ voltage vector */

      out->dq_ref.q   = align->cfg.volt;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = align->index_angle;
      out->foc_mode   = FOC_HANDLER_MODE_VOLTAGE;

      /* Increase counter */

      align->cntr += 1;
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: foc_align_offset_run_f32
 *
 * Description:
 *   Run offset alignment routine
 *
 * Input Parameter:
 *   align - pointer to FOC align routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

static int foc_align_offset_run_f32(FAR struct foc_align_f32_s *align,
                                    FAR struct foc_routine_in_f32_s *in,
                                    FAR struct foc_routine_out_f32_s *out)
{
  int ret = FOC_ROUTINE_RUN_NOTDONE;

  /* Reset Align angle sensor */

  if (align->cntr > align->cfg.offset_steps)
    {
      /* Align angle sensor */

      if (align->cfg.cb.zero != NULL)
        {
          ret = align->cfg.cb.zero(align->cfg.cb.priv, in->angle);
          if (ret < 0)
            {
              FOCLIBERR("ERROR: align offset callback failed %d!\n", ret);
              goto errout;
            }
        }

      /* Offset align done */

      align->cntr = 0;
      ret         = FOC_ROUTINE_RUN_DONE;

      /* Force IDLE mode */

      out->dq_ref.q   = 0.0f;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = 0.0f;
      out->foc_mode   = FOC_HANDLER_MODE_IDLE;
    }
  else
    {
      /* Force DQ voltage vector */

      out->dq_ref.q   = align->cfg.volt;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = FOC_ALIGN_ANGLE;
      out->foc_mode   = FOC_HANDLER_MODE_VOLTAGE;

      /* Increase counter */

      align->cntr += 1;
    }

  /* Store offset if done */

  if (ret == FOC_ROUTINE_RUN_DONE)
    {
      align->final.offset = in->angle;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_align_dir_move_f32
 ****************************************************************************/

static void foc_align_dir_move_f32(FAR struct foc_align_f32_s *align,
                                   float dir, float dest)
{
  bool next = false;

  DEBUGASSERT(align);

  /* Update angle */

  align->dir_angle += dir * ALIGN_DIR_ANGLE_STEP;

  /* Check destination */

  if (dir == DIR_CW)
    {
      if (align->dir_angle >= dest)
        {
          next = true;
        }
    }

  else if (dir == DIR_CCW)
    {
      if (align->dir_angle <= dest)
        {
          next = true;
        }
    }
  else
    {
      DEBUGASSERT(0);
    }

  /* Next step */

  if (next == true)
    {
      align->dir_step += 1;
    }
}

/****************************************************************************
 * Name: foc_align_dir_hold_f32
 ****************************************************************************/

static void foc_align_dir_hold_f32(FAR struct foc_align_f32_s *align,
                                   float dir, bool last, bool diff)
{
  DEBUGASSERT(align);

  /* Lock angle */

  align->dir_angle = align->dir_angle;

  /* Increase counter */

  align->cntr += 1;

  if (align->cntr > ALIGN_DIR_HOLD_CNTR)
    {
      /* Next step */

      align->dir_step += 1;

      /* Accumulate diff */

      if (diff == true)
        {
          if (dir == DIR_CW)
            {
              align->diff_cw += (align->angle_now - align->angle_last);
            }
          else if (dir == DIR_CCW)
            {
              align->diff_ccw += (align->angle_now - align->angle_last);
            }
          else
            {
              DEBUGASSERT(0);
            }
        }

      /* Store last angle */

      if (last == true)
        {
          align->angle_last = align->angle_now;
        }

      /* Reset counter */

      align->cntr = 0;
    }
}

/****************************************************************************
 * Name: foc_align_dir_run_f32
 *
 * Description:
 *   Run direction alignment routine
 *
 * Input Parameter:
 *   align - pointer to FOC align routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_align_dir_run_f32(FAR struct foc_align_f32_s *align,
                          FAR struct foc_routine_in_f32_s *in,
                          FAR struct foc_routine_out_f32_s *out)
{
  int  ret  = FOC_ROUTINE_RUN_NOTDONE;
  bool last = false;

  /* Store angle now */

  align->angle_now = in->angle;

  /* Handle dir align */

  if (align->dir_step == 0)
    {
      /* Start with position 0 */

      align->dir_angle = ALIGN_DIR_ANGLE_HOLD_0;

      /* Step 0 - hold position 0 */

      foc_align_dir_hold_f32(align, DIR_CW, true, true);
    }

  else if (align->dir_step == 1)
    {
      /* Step 1 - move motor in CW direction to position 1 */

      foc_align_dir_move_f32(align, DIR_CW, ALIGN_DIR_ANGLE_HOLD_1);
    }

  else if (align->dir_step == 2)
    {
      /* Step 2 - hold position 1 */

      foc_align_dir_hold_f32(align, DIR_CW, true, false);
    }

  else if (align->dir_step == 3)
    {
      /* Step 3 - move motor in CW direction to position 2 */

      foc_align_dir_move_f32(align, DIR_CW, ALIGN_DIR_ANGLE_HOLD_2);
    }

  else if (align->dir_step == 4)
    {
      /* Step 4 - hold position 2 */

      foc_align_dir_hold_f32(align, DIR_CW, true, true);
    }

  else if (align->dir_step == 5)
    {
      /* Step 5 - move motor in CW direction to position 3 */

      foc_align_dir_move_f32(align, DIR_CW, ALIGN_DIR_ANGLE_HOLD_3);
    }

  else if (align->dir_step == 6)
    {
      /* Step 6 - hold position 3 */

      foc_align_dir_hold_f32(align, DIR_CW, true, true);
    }

  else if (align->dir_step == 6)
    {
      /* Step 6 - move motor in CW direction to position 4 */

      foc_align_dir_move_f32(align, DIR_CW, ALIGN_DIR_ANGLE_HOLD_4);
    }

  else if (align->dir_step == 7)
    {
      /* Step 7 - hold position 4 */

      foc_align_dir_hold_f32(align, DIR_CW, true, true);
    }

  else if (align->dir_step == 8)
    {
      /* Step 8 - move motor in CCW direction to position 3 */

      foc_align_dir_move_f32(align, DIR_CCW, ALIGN_DIR_ANGLE_HOLD_3);
    }

  else if (align->dir_step == 9)
    {
      /* Step 9 - hold position 3 */

      foc_align_dir_hold_f32(align, DIR_CCW, true, true);
    }

  else if (align->dir_step == 10)
    {
      /* Step 10 - move motor in CCW direction to position 2 */

      foc_align_dir_move_f32(align, DIR_CCW, ALIGN_DIR_ANGLE_HOLD_2);
    }

  else if (align->dir_step == 11)
    {
      /* Step 11 - hold position 2 */

      foc_align_dir_hold_f32(align, DIR_CCW, true, true);
    }

  else if (align->dir_step == 12)
    {
      /* Step 12 - move motor in CCW direction to position 1 */

      foc_align_dir_move_f32(align, DIR_CCW, ALIGN_DIR_ANGLE_HOLD_1);
    }

  else if (align->dir_step == 13)
    {
      /* Step 13 - hold position 1 */

      foc_align_dir_hold_f32(align, DIR_CCW, true, true);
    }

  else if (align->dir_step == 14)
    {
      /* Step 14 - move motor in CCW direction to position 0 */

      foc_align_dir_move_f32(align, DIR_CCW, ALIGN_DIR_ANGLE_HOLD_0);
    }

  else if (align->dir_step == 15)
    {
      /* Step 15 - hold position 0 */

      foc_align_dir_hold_f32(align, DIR_CCW, true, true);
    }

  else
    {
      /* Set flag */

      last = true;

      /* Get sensor direction according to sampled data */

      if (align->diff_cw > 0.0f && align->diff_ccw < 0.0f)
        {
          align->final.dir = DIR_CW;
        }

      else if (align->diff_cw < 0.0f && align->diff_ccw > 0.0f)
        {
          align->final.dir = DIR_CCW;
        }

      else
        {
          /* Invalid data */

          FOCLIBERR("ERROR: direction align failed !\n");

          align->final.dir = DIR_NONE;
          ret              = -EINVAL;

          goto errout;
        }

      /* Align angle sensor */

      if (align->cfg.cb.dir != NULL)
        {
          ret = align->cfg.cb.dir(align->cfg.cb.priv, align->final.dir);
          if (ret < 0)
            {
              FOCLIBERR("ERROR: align dir callback failed %d!\n", ret);
              goto errout;
            }
        }

      /* Direction alignment done */

      ret = FOC_ROUTINE_RUN_DONE;
    }

  /* Force DQ voltage vector */

  out->dq_ref.q   = align->cfg.volt;
  out->dq_ref.d   = 0.0f;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = align->dir_angle;
  out->foc_mode   = FOC_HANDLER_MODE_VOLTAGE;

errout:

  /* Force current to zero if alignment done */

  if (last == true)
    {
      out->dq_ref.q = 0.0f;
      out->dq_ref.d = 0.0f;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_align_idle_run_f32
 *
 * Description:
 *   Force IDLE state
 *
 * Input Parameter:
 *   align - pointer to FOC align routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

static int foc_align_idle_run_f32(FAR struct foc_align_f32_s *align,
                                    FAR struct foc_routine_in_f32_s *in,
                                    FAR struct foc_routine_out_f32_s *out)
{
  int ret = FOC_ROUTINE_RUN_NOTDONE;

  /* Get output */

  out->dq_ref.q   = 0.0f;
  out->dq_ref.d   = 0.0f;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = 0.0f;
  out->foc_mode   = FOC_HANDLER_MODE_IDLE;

  /* Increase counter */

  align->cntr += 1;

  if (align->cntr > IDLE_STEPS)
    {
      /* Done */

      ret = FOC_ROUTINE_RUN_DONE;

      /* Reset counter */

      align->cntr = 0;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_routine_align_init_f32
 *
 * Description:
 *   Initialize the FOC align routine (float32)
 *
 * Input Parameter:
 *   r - pointer to FOC routine
 *
 ****************************************************************************/

int foc_routine_align_init_f32(FAR foc_routine_f32_t *r)
{
  int ret = OK;

  DEBUGASSERT(r);

  /* Connect angle data */

  r->data = zalloc(sizeof(struct foc_align_f32_s));
  if (r->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_align_deinit_f32
 *
 * Description:
 *   Deinitialize the FOC align routine (float32)
 *
 * Input Parameter:
 *   r - pointer to FOC routine
 *
 ****************************************************************************/

void foc_routine_align_deinit_f32(FAR foc_routine_f32_t *r)
{
  DEBUGASSERT(r);

  if (r->data)
    {
      /* Free routine data */

      free(r->data);
    }
}

/****************************************************************************
 * Name: foc_routine_align_cfg_f32
 *
 * Description:
 *   Configure the FOC align routine (float32)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   cfg - pointer to align routine configuration data
 *
 ****************************************************************************/

int foc_routine_align_cfg_f32(FAR foc_routine_f32_t *r, FAR void *cfg)
{
  FAR struct foc_align_f32_s *a   = NULL;
  int                         ret = OK;

  DEBUGASSERT(r);
  DEBUGASSERT(cfg);

  /* Get aling data */

  DEBUGASSERT(r->data);
  a = r->data;

  /* Copy configuration */

  memcpy(&a->cfg, cfg, sizeof(struct foc_routine_align_cfg_f32_s));

  /* Verify configuration */

  if (a->cfg.volt <= 0.0f)
    {
      ret = -EINVAL;
      goto errout;
    }

  if (a->cfg.offset_steps <= 0)
    {
      ret = -EINVAL;
      goto errout;
    }

  if (a->cfg.cb.zero == NULL || a->cfg.cb.dir == NULL ||
      a->cfg.cb.priv == NULL)
    {
      FOCLIBWARN("WARN: no align cb data!\n");
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_align_run_f32
 *
 * Description:
 *   Run the FOC align routine step (float32)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   in  - pointer to FOC routine input data
 *   out - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_routine_align_run_f32(FAR foc_routine_f32_t *r,
                              FAR struct foc_routine_in_f32_s *in,
                              FAR struct foc_routine_out_f32_s *out)
{
  FAR struct foc_align_f32_s *a   = NULL;
  int                         ret = FOC_ROUTINE_RUN_NOTDONE;

  DEBUGASSERT(r);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Get aling data */

  DEBUGASSERT(r->data);
  a = r->data;

  /* Force IDLE state at default */

  out->dq_ref.q   = 0.0f;
  out->dq_ref.d   = 0.0f;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = 0.0f;
  out->foc_mode   = FOC_HANDLER_MODE_IDLE;

  /* Handle alignment stage */

  switch (a->stage)
    {
      case FOC_ALIGN_RUN_INIT:
        {
          a->stage += 1;
          ret = FOC_ROUTINE_RUN_NOTDONE;

          break;
        }

#ifdef CONFIG_INDUSTRY_FOC_ALIGN_INDEX
      case FOC_ALIGN_RUN_INDEX:
        {
          /* Align zero procedure */

          ret = foc_align_index_run_f32(a, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              FOCLIBLOG("ALIGN INDEX done!\n");

              a->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }
#endif

    case FOC_ALIGN_RUN_OFFSET:
      {
        /* Align zero procedure */

        ret = foc_align_offset_run_f32(a, in, out);
        if (ret < 0)
          {
            goto errout;
          }

        if (ret == FOC_ROUTINE_RUN_DONE)
          {
            FOCLIBLOG("ALIGN OFFSET done!\n");

            a->stage += 1;
            ret = FOC_ROUTINE_RUN_NOTDONE;
          }

        break;
      }

      case FOC_ALIGN_RUN_DIR:
        {
          /* Align direction procedure */

          ret = foc_align_dir_run_f32(a, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              FOCLIBLOG("ALIGN DIR done!\n");

              a->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }

      case FOC_ALIGN_RUN_IDLE:
        {
          /* De-energetize motor */

          ret = foc_align_idle_run_f32(a, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              FOCLIBLOG("ALIGN IDLE done!\n");

              a->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }

      case FOC_ALIGN_RUN_DONE:
        {
          ret = FOC_ROUTINE_RUN_DONE;

          break;
        }

      default:
        {
          FOCLIBERR("ERROR: invalid alignment stage %d\n", a->stage);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_align_final_f32
 *
 * Description:
 *   Finalize the FOC routine data (float32)
 *
 * Input Parameter:
 *   r    - pointer to FOC routine
 *   data - pointer to FOC align routine final data
 *
 ****************************************************************************/

int foc_routine_align_final_f32(FAR foc_routine_f32_t *r, FAR void *data)
{
  FAR struct foc_align_f32_s *a = NULL;

  DEBUGASSERT(r);
  DEBUGASSERT(data);

  /* Get aling data */

  DEBUGASSERT(r->data);
  a = r->data;

  /* Get final data */

  memcpy(data, &a->final, sizeof(struct foc_routine_aling_final_f32_s));

  return OK;
}
