/****************************************************************************
 * apps/examples/fmsynth/operator_algorithm.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stddef.h>

#include <audioutils/fmsynth.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: fmsynthutil_algorithm0
 ****************************************************************************/

FAR fmsynth_op_t *fmsynthutil_algorithm0(void)
{
  /* [Simple sin operator]
   *
   *  +--------------+
   *  | OP (carrier) | ----> Audio out
   *  +--------------+
   */

  FAR fmsynth_op_t *carrier;
  fmsynth_eglevels_t level;

  /* Attack level and period time      : 1.0, 40ms
   * Decay Break level and period time : 0.3, 200ms
   * Decay level and period time       : 0.1, 100ms
   * Sustain level and period time     : 0.1, 100ms
   * Release level                     : 0.0, 0ms
   *
   */

  level.attack.level       = 1.0f;
  level.attack.period_ms   = 40;
  level.decaybrk.level     = 0.3f;
  level.decaybrk.period_ms = 200;
  level.decay.level        = 0.1f;
  level.decay.period_ms    = 100;
  level.sustain.level      = 0.1f;
  level.sustain.period_ms  = 100;
  level.release.level      = 0.f;
  level.release.period_ms  = 0;

  carrier = fmsynthop_create();
  if (carrier)
    {
      fmsynthop_set_envelope(carrier, &level);
      fmsynthop_select_opfunc(carrier, FMSYNTH_OPFUNC_SIN);
    }

  return carrier;
}

/****************************************************************************
 * name: fmsynthutil_algorithm1
 ****************************************************************************/

FAR fmsynth_op_t *fmsynthutil_algorithm1(void)
{
  /*        feed back
   *  +--------------------+
   *  |                    |
   *  |  +--------------+  |
   *  +->| OP (carrier) | -+--> Audio out
   *     +--------------+
   */

  FAR fmsynth_op_t *carrier;
  fmsynth_eglevels_t level;

  level.attack.level       = 1.0f;
  level.attack.period_ms   = 40;
  level.decaybrk.level     = 0.3f;
  level.decaybrk.period_ms = 200;
  level.decay.level        = 0.1f;
  level.decay.period_ms    = 100;
  level.sustain.level      = 0.1f;
  level.sustain.period_ms  = 100;
  level.release.level      = 0.f;
  level.release.period_ms  = 0;

  carrier = fmsynthop_create();
  if (carrier)
    {
      fmsynthop_set_envelope(carrier, &level);
      fmsynthop_select_opfunc(carrier, FMSYNTH_OPFUNC_SIN);
      fmsynthop_bind_feedback(carrier, carrier, 0.6f);
    }

  return carrier;
}

/****************************************************************************
 * name: fmsynthutil_algorithm2
 ****************************************************************************/

FAR fmsynth_op_t *fmsynthutil_algorithm2(void)
{
  /*        feed back
   *  +--------------------+
   *  |                    |
   *  |  +--------------+  |    +--------------+
   *  +->| OP (subop)   | -+->  | OP (carrier) | ----> Audio out
   *     +--------------+       +--------------+
   */

  FAR fmsynth_op_t *carrier;
  FAR fmsynth_op_t *subop;
  fmsynth_eglevels_t level;

  level.attack.level       = 1.0f;
  level.attack.period_ms   = 40;
  level.decaybrk.level     = 0.3f;
  level.decaybrk.period_ms = 200;
  level.decay.level        = 0.1f;
  level.decay.period_ms    = 100;
  level.sustain.level      = 0.f;
  level.sustain.period_ms  = 0;
  level.release.level      = 0.f;
  level.release.period_ms  = 0;

  carrier = fmsynthop_create();
  if (carrier)
    {
      subop = fmsynthop_create();
      if (!subop)
        {
          fmsynthop_delete(carrier);
          return NULL;
        }

      fmsynthop_set_envelope(carrier, &level);
      fmsynthop_select_opfunc(carrier, FMSYNTH_OPFUNC_SIN);

      fmsynthop_set_soundfreqrate(subop, 3.7f);
      fmsynthop_select_opfunc(subop, FMSYNTH_OPFUNC_SIN);

      fmsynthop_cascade_subop(carrier, subop);
    }

  return carrier;
}

/****************************************************************************
 * name: fmsynthutil_delete_ops
 ****************************************************************************/

void FAR fmsynthutil_delete_ops(FAR fmsynth_op_t *op)
{
  FAR fmsynth_op_t *tmp;

  while (op != NULL)
    {
      tmp = op->parallelop;

      if (op->cascadeop)
        {
          fmsynthutil_delete_ops(op->cascadeop);
        }

      fmsynthop_delete(op);
      op = tmp;
    }
}
