/****************************************************************************
 * apps/industry/foc/fixed16/foc_cordic.c
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
#include "industry/foc/fixed16/foc_cordic.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_DQSAT

/****************************************************************************
 * Name: foc_cordic_dqsat_b16
 *
 * Description:
 *   CORDIC DQ-frame saturation (fixed16)
 *
 * Input Parameter:
 *   fd      - the file descriptor for CORDIC device
 *   dq_ref  - DQ vector
 *   mag_max - vector magnitude max
 *
 ****************************************************************************/

int foc_cordic_dqsat_b16(int fd, FAR dq_frame_b16_t *dq, b16_t mag_max)
{
  struct cordic_calc_s io;
  b16_t                dnorm   = 0;
  b16_t                qnorm   = 0;
  b16_t                dqscale = 0;
  b16_t                mag     = 0;
  b16_t                tmp     = 0;
  int                  ret     = OK;

  DEBUGASSERT(dq);

  /* Normalize DQ to [-1, 1] */

  if (dq->d > dq->q)
    {
      dqscale = b16divb16(b16ONE, dq->d);
    }
  else
    {
      dqscale = b16divb16(b16ONE, dq->q);
    }

  dnorm = b16mulb16(dq->d, dqscale);
  qnorm = b16mulb16(dq->d, dqscale);

  /* Get modulus */

  io.func      = CORDIC_CALC_FUNC_MOD;
  io.res2_incl = false;
  io.arg1      = b16toq31(dnorm);
  io.arg2      = b16toq31(qnorm);
  io.res1      = 0;
  io.res2      = 0;

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  /* Get real magnitude */

  mag = b16mulb16(q31tof(io.res1), dqscale);

  /* Magnitude bottom limit */

  if (mag < 1)
    {
      mag = 1;
    }

  if (mag > mag_max)
    {
      /* Saturate vector */

      tmp = b16divb16(mag_max, mag);
      dq->d = b16mulb16(dq->d, tmp);
      dq->q = b16mulb16(dq->q, tmp);
    }

  return OK;
}
#endif  /* CONFIG_INDUSTRY_FOC_CORDIC_DQSAT */

#ifdef CONFIG_INDUSTRY_FOC_CORDIC_ANGLE

/****************************************************************************
 * Name: foc_cordic_angle_b16
 *
 * Description:
 *   CORDIC angle update (fixed16)
 *
 * Input Parameter:
 *   fd    - the file descriptor for CORDIC device
 *   angle - phase angle data
 *   a     - phase angle in rad
 *
 ****************************************************************************/

int foc_cordic_angle_b16(int fd, FAR phase_angle_b16_t *angle, b16_t a)
{
  const b16_t          onebypi = b16divb16(b16ONE, b16PI);
  struct cordic_calc_s io;
  b16_t                anorm   = 0;
  int                  ret     = OK;

  DEBUGASSERT(angle);

  /* Copy angle */

  anorm = a;

  /* Normalize angle to [-PI, PI] */

  angle_norm_2pi_b16(&anorm, -b16PI, b16PI);

  /* Normalize angle to [-1, 1] */

  anorm = b16mulb16(anorm, onebypi);

  /* Get cosine and sine from single call */

  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = b16toq31(anorm);
  io.arg2      = b16toq31(b16ONE);
  io.res1      = 0;
  io.res2      = 0;

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      FOCLIBERR("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  /* Fill phase angle struct */

  angle->angle = a;
  angle->cos   = q31tob16(io.res1);
  angle->sin   = q31tob16(io.res2);

  return OK;
}
#endif  /* CONFIG_INDUSTRY_FOC_CORDIC_ANGLE */
