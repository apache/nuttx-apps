/****************************************************************************
 * apps/industry/foc/float/foc_ident.c
 * This file implements motor ident routine for float32
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

#include "industry/foc/float/foc_ident.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IDENT_PI_KP           (0.0f)
#define IDENT_PI_KI           (0.05f)

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* Identification stages */

enum foc_ident_run_stage_e
{
  FOC_IDENT_RUN_INIT,
  FOC_IDENT_RUN_IDLE1,
  FOC_IDENT_RUN_RES,
  FOC_IDENT_RUN_IDLE2,
  FOC_IDENT_RUN_IND,
  FOC_IDENT_RUN_IDLE3,
  FOC_IDENT_RUN_DONE
};

/* Ident routine private data */

struct foc_ident_f32_s
{
  struct foc_routine_ident_cfg_f32_s   cfg;   /* Ident configuration */
  struct foc_routine_ident_final_f32_s final; /* Ident final result */
  pid_controller_f32_t                 pi;    /* PI controller for res */
  int                                  cntr;  /* Helper counter */
  int                                  stage; /* Ident stage */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

int foc_routine_ident_init_f32(FAR foc_routine_f32_t *r);
void foc_routine_ident_deinit_f32(FAR foc_routine_f32_t *r);
int foc_routine_ident_cfg_f32(FAR foc_routine_f32_t *r, FAR void *cfg);
int foc_routine_ident_run_f32(FAR foc_routine_f32_t *r,
                              FAR struct foc_routine_in_f32_s *in,
                              FAR struct foc_routine_out_f32_s *out);
int foc_routine_ident_final_f32(FAR foc_routine_f32_t *r, FAR void *data);

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct foc_routine_ops_f32_s g_foc_routine_ident_f32 =
{
  .init   = foc_routine_ident_init_f32,
  .deinit = foc_routine_ident_deinit_f32,
  .cfg    = foc_routine_ident_cfg_f32,
  .run    = foc_routine_ident_run_f32,
  .final  = foc_routine_ident_final_f32,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_ident_idle_run_f32
 *
 * Description:
 *   Force IDLE state
 *
 * Input Parameter:
 *   ident - pointer to FOC ident routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_ident_idle_run_f32(FAR struct foc_ident_f32_s *ident,
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

  ident->cntr += 1;

  if (ident->cntr > ident->cfg.idle_steps)
    {
      /* Done */

      ret = FOC_ROUTINE_RUN_DONE;

      /* Reset counter */

      ident->cntr = 0;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_ident_res_run_f32
 *
 * Description:
 *   Run resistance identification routine
 *
 * Input Parameter:
 *   ident - pointer to FOC ident routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_ident_res_run_f32(FAR struct foc_ident_f32_s *ident,
                          FAR struct foc_routine_in_f32_s *in,
                          FAR struct foc_routine_out_f32_s *out)
{
  int   ret  = FOC_ROUTINE_RUN_NOTDONE;
  float err  = 0.0f;
  float vref = 0.0f;

  /* Initialize PI controller */

  if (ident->cntr == 0)
    {
      pi_controller_init(&ident->pi, IDENT_PI_KP, IDENT_PI_KI);
    }

  /* PI saturation */

  pi_saturation_set(&ident->pi, 0.0f, in->vbus);

  /* Lock motor with given current */

  err = ident->cfg.res_current - in->foc_state->idq.d;
  vref = pi_controller(&ident->pi, err);

  /* Force alpha voltage = vref */

  out->dq_ref.q   = 0.0f;
  out->dq_ref.d   = vref;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = 0.0f;
  out->foc_mode   = FOC_HANDLER_MODE_VOLTAGE;

  /* Increase counter */

  ident->cntr += 1;

  if (ident->cntr > ident->cfg.res_steps)
    {
      /* Get resistance */

      ident->final.res = vref / ident->cfg.res_current;

      /* Force IDLE state */

      out->dq_ref.q   = 0.0f;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = 0.0f;
      out->foc_mode   = FOC_HANDLER_MODE_IDLE;

      /* Resistance identification done */

      ret = FOC_ROUTINE_RUN_DONE;

      /* Reset counter */

      ident->cntr = 0;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_ident_ind_run_f32
 *
 * Description:
 *   Run inductance identification routine
 *
 * Input Parameter:
 *   ident - pointer to FOC ident routine
 *   in    - pointer to FOC routine input data
 *   out   - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_ident_ind_run_f32(FAR struct foc_ident_f32_s *ident,
                          FAR struct foc_routine_in_f32_s *in,
                          FAR struct foc_routine_out_f32_s *out)
{
  int          ret        = FOC_ROUTINE_RUN_NOTDONE;
  float        vref       = 0.0f;
  float        curr1_avg  = 0.0f;
  float        curr2_avg  = 0.0f;
  float        delta_curr = 0.0f;
  static float sign       = 1.0f;
  static float curr1_sum  = 0.0f;
  static float curr2_sum  = 0.0f;

  /* If previous sign was -1 then we have top current,
   * if previous sing was +1 then we have bottom current.
   */

  if (sign > 0)
    {
      /* Average bottm current */

      curr1_sum += in->foc_state->idq.d;
    }
  else
    {
      /* Average top current */

      curr2_sum += in->foc_state->idq.d;
    }

  /* Invert voltage to generate square wave D voltage */

  sign = -sign;
  vref = sign * ident->cfg.ind_volt;

  /* Force alpha voltage = vref */

  out->dq_ref.q   = 0.0f;
  out->dq_ref.d   = vref;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = 0.0f;
  out->foc_mode   = FOC_HANDLER_MODE_VOLTAGE;

  /* Increase counter */

  ident->cntr += 1;

  if (ident->cntr > ident->cfg.ind_steps)
    {
      /* Half samples from curr1, other half from curr2 */

      curr1_avg = 2 * curr1_sum / ident->cntr;
      curr2_avg = 2 * curr2_sum / ident->cntr;

      /* Average delta current */

      delta_curr = curr1_avg - curr2_avg;

      /* Get inductance
       *   L = V * t / dI
       *
       *  where:
       *   t = per
       *   V = ind_volt
       *   dI = avg(curr_bottom) - avg(curr_top)
       */

      ident->final.ind = ident->cfg.ind_volt * ident->cfg.per / delta_curr;

      /* Force IDLE state */

      out->dq_ref.q   = 0.0f;
      out->dq_ref.d   = 0.0f;
      out->vdq_comp.q = 0.0f;
      out->vdq_comp.d = 0.0f;
      out->angle      = 0.0f;
      out->foc_mode   = FOC_HANDLER_MODE_IDLE;

      /* Inductance identification done */

      ret = FOC_ROUTINE_RUN_DONE;

      /* Reset counter */

      ident->cntr = 0;

      /* Reset static data */

      sign      = 1.0f;
      curr1_sum = 0.0f;
      curr2_sum = 0.0f;
    }

  return ret;
}

/****************************************************************************
 * Name: foc_routine_ident_init_f32
 *
 * Description:
 *   Initialize the FOC ident routine (float32)
 *
 * Input Parameter:
 *   r - pointer to FOC routine
 *
 ****************************************************************************/

int foc_routine_ident_init_f32(FAR foc_routine_f32_t *r)
{
  int ret = OK;

  DEBUGASSERT(r);

  /* Connect angle data */

  r->data = zalloc(sizeof(struct foc_ident_f32_s));
  if (r->data == NULL)
    {
      ret = -ENOMEM;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_ident_deinit_f32
 *
 * Description:
 *   Deinitialize the FOC ident routine (float32)
 *
 * Input Parameter:
 *   r - pointer to FOC routine
 *
 ****************************************************************************/

void foc_routine_ident_deinit_f32(FAR foc_routine_f32_t *r)
{
  DEBUGASSERT(r);

  if (r->data)
    {
      /* Free routine data */

      free(r->data);
    }
}

/****************************************************************************
 * Name: foc_routine_ident_cfg_f32
 *
 * Description:
 *   Configure the FOC ident routine (float32)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   cfg - pointer to ident routine configuration data
 *
 ****************************************************************************/

int foc_routine_ident_cfg_f32(FAR foc_routine_f32_t *r, FAR void *cfg)
{
  FAR struct foc_ident_f32_s *i   = NULL;
  int                         ret = OK;

  DEBUGASSERT(r);

  /* Get ident data */

  DEBUGASSERT(r->data);
  i = r->data;

  /* Copy configuration */

  memcpy(&i->cfg, cfg, sizeof(struct foc_routine_ident_cfg_f32_s));

  /* Verify configuration */

  if (i->cfg.per <= 0.0f)
    {
      ret = -EINVAL;
      goto errout;
    }

  if (i->cfg.res_current <= 0.0f || i->cfg.ind_volt <= 0.0f)
    {
      ret = -EINVAL;
      goto errout;
    }

  if (i->cfg.res_steps <= 0 || i->cfg.ind_steps <= 0 ||
      i->cfg.idle_steps <= 0)
    {
      ret = -EINVAL;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_ident_run_f32
 *
 * Description:
 *   Run the FOC ident routine step (float32)
 *
 * Input Parameter:
 *   r   - pointer to FOC routine
 *   in  - pointer to FOC routine input data
 *   out - pointer to FOC routine output data
 *
 ****************************************************************************/

int foc_routine_ident_run_f32(FAR foc_routine_f32_t *r,
                              FAR struct foc_routine_in_f32_s *in,
                              FAR struct foc_routine_out_f32_s *out)
{
  FAR struct foc_ident_f32_s *i = NULL;
  int ret                       = FOC_ROUTINE_RUN_NOTDONE;

  DEBUGASSERT(r);
  DEBUGASSERT(in);
  DEBUGASSERT(out);

  /* Get ident data */

  DEBUGASSERT(r->data);
  i = r->data;

  /* Force IDLE state at default */

  out->dq_ref.q   = 0.0f;
  out->dq_ref.d   = 0.0f;
  out->vdq_comp.q = 0.0f;
  out->vdq_comp.d = 0.0f;
  out->angle      = 0.0f;
  out->foc_mode   = FOC_HANDLER_MODE_IDLE;

  /* Handle ident stage */

  switch (i->stage)
    {
      case FOC_IDENT_RUN_INIT:
        {
          i->stage += 1;
          ret = FOC_ROUTINE_RUN_NOTDONE;

          break;
        }

      case FOC_IDENT_RUN_IDLE1:
      case FOC_IDENT_RUN_IDLE2:
      case FOC_IDENT_RUN_IDLE3:
        {
          /* De-energetize motor */

          ret = foc_ident_idle_run_f32(i, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              i->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }

      case FOC_IDENT_RUN_RES:
        {
          /* Resistance */

          ret = foc_ident_res_run_f32(i, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              FOCLIBLOG("IDENT RES done!\n");

              i->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }

      case FOC_IDENT_RUN_IND:
        {
          /* Inductance */

          ret = foc_ident_ind_run_f32(i, in, out);
          if (ret < 0)
            {
              goto errout;
            }

          if (ret == FOC_ROUTINE_RUN_DONE)
            {
              FOCLIBLOG("IDENT IND done!\n");

              i->stage += 1;
              ret = FOC_ROUTINE_RUN_NOTDONE;
            }

          break;
        }

      case FOC_IDENT_RUN_DONE:
        {
          ret = FOC_ROUTINE_RUN_DONE;

          break;
        }

      default:
        {
          FOCLIBERR("ERROR: invalid ident stage %d\n", i->stage);
          ret = -EINVAL;
          goto errout;
        }
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_routine_ident_final_f32
 *
 * Description:
 *   Finalize the FOC routine data (float32)
 *
 * Input Parameter:
 *   r    - pointer to FOC routine
 *   data - pointer to FOC ident routine final data
 *
 ****************************************************************************/

int foc_routine_ident_final_f32(FAR foc_routine_f32_t *r, FAR void *data)
{
  FAR struct foc_ident_f32_s *i = NULL;

  DEBUGASSERT(r);
  DEBUGASSERT(data);

  /* Get ident data */

  DEBUGASSERT(r->data);
  i = r->data;

  /* Get final data */

  memcpy(data, &i->final, sizeof(struct foc_routine_ident_final_f32_s));

  return OK;
}
