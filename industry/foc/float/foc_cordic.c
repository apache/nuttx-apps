/****************************************************************************
 * apps/industry/foc/float/foc_cordic.c
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
#include <fcntl.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/math/cordic.h>
#include <nuttx/math/math_ioctl.h>

#include "industry/foc/foc_log.h"
#include "industry/foc/float/foc_cordic.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_DQSAT

/****************************************************************************
 * Name: foc_cordic_dqsat_f32
 *
 * Description:
 *   CORDIC DQ-frame saturation (float32)
 *
 * Input Parameter:
 *   fd     - the file descriptor for CORDIC device
 *   dq_ref - DQ vector
 *   mag_max - vector magnitude max
 *
 ****************************************************************************/

int foc_cordic_dqsat_f32(int fd, FAR dq_frame_f32_t *dq, float mag_max)
{
  struct cordic_calc_s io;
  float                dnorm   = 0.0f;
  float                qnorm   = 0.0f;
  float                dqscale = 0.0f;
  float                mag     = 0.0f;
  float                tmp     = 0.0f;
  int                  ret     = OK;

  DEBUGASSERT(dq);

  /* Normalize DQ to [-1, 1] */

  if (dq->d > dq->q)
    {
      dqscale = (1.0f / dq->d);
    }
  else
    {
      dqscale = (1.0f / dq->q);
    }

  dnorm = dq->d * dqscale;
  qnorm = dq->d * dqscale;

  /* Get modulus */

  io.func      = CORDIC_CALC_FUNC_MOD;
  io.res2_incl = false;
  io.arg1      = ftoq31(dnorm);
  io.arg2      = ftoq31(qnorm);
  io.res1      = 0;
  io.res2      = 0;

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  /* Get real magnitude */

  mag = q31tof(io.res1) * dqscale;

  /* Magnitude bottom limit */

  if (mag < 1e-10f)
    {
      mag = 1e-10f;
    }

  if (mag > mag_max)
    {
      /* Saturate vector */

      tmp = mag_max / mag;
      dq->d *= tmp;
      dq->q *= tmp;
    }

  return OK;
}
#endif  /* CONFIG_INDUSTRY_FOC_CORDIC_DQSAT */

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_ANGLE

/****************************************************************************
 * Name: foc_cordic_angle_f32
 *
 * Description:
 *   CORDIC angle update (float32)
 *
 * Input Parameter:
 *   fd    - the file descriptor for CORDIC device
 *   angle - phase angle data
 *   a     - phase angle in rad
 *
 ****************************************************************************/

int foc_cordic_angle_f32(int fd, FAR phase_angle_f32_t *angle, float a)
{
  const float          onebypi = (1.0f / M_PI_F);
  struct cordic_calc_s io;
  float                anorm   = 0.0f;
  int                  ret     = OK;

  DEBUGASSERT(angle);

  /* Copy angle */

  anorm = a;

  /* Normalize angle to [-PI, PI] */

  angle_norm_2pi(&anorm, -M_PI_F, M_PI_F);

  /* Normalize angle to [-1, 1] */

  anorm = anorm * onebypi;

  /* Get cosine and sine from single call */

  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = ftoq31(anorm);
  io.arg2      = ftoq31(1.0f);
  io.res1      = 0;
  io.res2      = 0;

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  /* Fill phase angle struct */

  angle->angle = a;
  angle->cos   = q31tof(io.res1);
  angle->sin   = q31tof(io.res2);

  return OK;
}
#endif  /* CONFIG_INDUSTRY_FOC_CORDIC_ANGLE */
