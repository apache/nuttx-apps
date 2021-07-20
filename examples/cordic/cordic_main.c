/****************************************************************************
 * apps/examples/cordic/cordic_main.c
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

#include <sys/types.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <math.h>
#include <fixedmath.h>

#include <nuttx/math/cordic.h>
#include <nuttx/math/math_ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_MATH_CORDIC_USE_Q31
#  error Not supported
#endif

#ifndef CONFIG_LIBC_FLOATINGPOINT
#  error CONFIG_LIBC_FLOATINGPOINT must be enabled
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cordic_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct cordic_calc_s io;
  int                  fd      = 0;
  int                  ret     = OK;
  float                arg1f   = 0.0f;
  float                arg2f   = 0.0f;
  float                res1f   = 0.0f;
  float                res2f   = 0.0f;
  b16_t                arg1b16 = 0;
  b16_t                arg2b16 = 0;
  b16_t                res1b16 = 0;
  b16_t                res2b16 = 0;

  /* Reset data */

  memset(&io, 0, sizeof(struct cordic_calc_s));

  /* Open the CORDIC device */

  fd = open(CONFIG_EXAMPLES_CORDIC_DEVPATH, O_RDWR);
  if (fd < 0)
    {
      printf("ERROR: open %s failed: %d\n",
             CONFIG_EXAMPLES_CORDIC_DEVPATH, errno);
      goto errout;
    }

  /* Get cosine */

  arg1f        = 0.05f;
  arg2f        = 1.0f;
  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = false;
  io.arg1      = ftoq31(arg1f);
  io.arg2      = ftoq31(arg2f);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1f = q31tof(io.res1);
  res2f = q31tof(io.res2);

  printf("[float] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1=%0.5f res2=%0.5f\n",
         io.func, io.res2_incl, arg1f, arg2f, res1f, res2f);

  /* Get cosine and sine from single call */

  arg1f        = -0.5f;
  arg2f        = 1.0f;
  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = ftoq31(arg1f);
  io.arg2      = ftoq31(arg2f);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1f = q31tof(io.res1);
  res2f = q31tof(io.res2);

  printf("[float] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1f=%0.5f res2f=%0.5f\n",
         io.func, io.res2_incl, arg1f, arg2f, res1f, res2f);

  /* Get cosine and sine from single call */

  arg1f        = -0.1f;
  arg2f        = 1.0f;
  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = ftoq31(arg1f);
  io.arg2      = ftoq31(arg2f);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1f = q31tof(io.res1);
  res2f = q31tof(io.res2);

  printf("[float] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1=%0.5f res2=%0.5f\n",
         io.func, io.res2_incl, arg1f, arg2f, res1f, res2f);

  /* Get cosine and sine from single call */

  arg1f        = 0.8f;
  arg2f        = 1.0f;
  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = ftoq31(arg1f);
  io.arg2      = ftoq31(arg2f);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1f = q31tof(io.res1);
  res2f = q31tof(io.res2);

  printf("[float] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1=%0.5f res2=%0.5f\n",
         io.func, io.res2_incl, arg1f, arg2f, res1f, res2f);

  /* Get phase and modulus from single call */

  arg1f        = -0.5f;
  arg2f        = -0.5f;
  io.func      = CORDIC_CALC_FUNC_PHASE;
  io.res2_incl = true;
  io.arg1      = ftoq31(arg1f);
  io.arg2      = ftoq31(arg2f);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1f = q31tof(io.res1);
  res2f = q31tof(io.res2);

  printf("[float] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1f=%0.5f res2f=%0.5f\n",
         io.func, io.res2_incl, arg1f, arg2f, res1f, res2f);

  /* Get cosine and sine from single call (fixed16) */

  arg1b16      = ftob16(-0.5f);
  arg2b16      = ftob16(1.0f);
  io.func      = CORDIC_CALC_FUNC_COS;
  io.res2_incl = true;
  io.arg1      = b16toq31(arg1b16);
  io.arg2      = b16toq31(arg2b16);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1b16 = q31tob16(io.res1);
  res2b16 = q31tob16(io.res2);

  printf("[fixed16] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1=%0.5f res2=%0.5f\n",
         io.func, io.res2_incl, b16tof(arg1b16), b16tof(arg2b16),
         b16tof(res1b16), b16tof(res2b16));

  /* Get phase and modulus from single call (fixed16) */

  arg1b16      = ftob16(-0.5f);
  arg2b16      = ftob16(-0.5f);
  io.func      = CORDIC_CALC_FUNC_PHASE;
  io.res2_incl = true;
  io.arg1      = b16toq31(arg1b16);
  io.arg2      = b16toq31(arg2b16);

  ret = ioctl(fd, MATHIOC_CORDIC_CALC, (unsigned long)((uintptr_t)&io));
  if (ret < 0)
    {
      printf("ERROR: MATHIOC_CORDIC_CALC failed, errno=%d\n", errno);
    }

  res1b16 = q31tob16(io.res1);
  res2b16 = q31tob16(io.res2);

  printf("[fxied16] func=%" PRIu8 " res2_inc=%d "
         "arg1f=%0.5f arg2=%0.5f res1=%0.5f res2=%0.5f\n",
         io.func, io.res2_incl, b16tof(arg1b16), b16tof(arg2b16),
         b16tof(res1b16), b16tof(res2b16));

errout:

  /* Close CORDIC device */

  if (fd > 0)
    {
      close(fd);
    }

  return 0;
}
