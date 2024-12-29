/****************************************************************************
 * apps/examples/foc/foc_nxscope.c
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

#include <nuttx/config.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "foc_debug.h"
#include "foc_nxscope.h"
#include "foc_thr.h"

#include "industry/foc/foc_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LOGGING_NXSCOPE_DISABLE_PUTLOCK
#  error CONFIG_LOGGING_NXSCOPE_DISABLE_PUTLOCK must be set to proper operation.
#endif

#if defined(CONFIG_LOGGING_NXSCOPE_INTF_SERIAL) && !defined(CONFIG_SERIAL_RTT)
#  ifndef CONFIG_SERIAL_TERMIOS
#    error CONFIG_SERIAL_TERMIOS must be set to proper operation.
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE_THREAD
/****************************************************************************
 * Name: foc_nxscope_thr
 ****************************************************************************/

static FAR void *foc_nxscope_thr(FAR void *arg)
{
  FAR struct foc_nxscope_s *nxs = (FAR struct foc_nxscope_s *)arg;

  DEBUGASSERT(nxs);

  while (1)
    {
      /* NxScope work */

      foc_nxscope_work(nxs);

      usleep(10000);
    }

  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_nxscope_init
 ****************************************************************************/

int foc_nxscope_init(FAR struct foc_nxscope_s *nxs)
{
  union nxscope_chinfo_type_u u;
  struct nxscope_cfg_s        nxs_cfg;
#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE_THREAD
  struct sched_param          param;
  pthread_attr_t              attr;
  pthread_t                   nxsthr = 0;
#endif
  int                         ret    = OK;
  int                         i      = 0;
  int                         j      = 0;

  DEBUGASSERT(nxs);

  /* Initialize serial interface */

  nxs->ser_cfg.path     = CONFIG_EXAMPLES_FOC_NXSCOPE_SERIAL_PATH;
  nxs->ser_cfg.nonblock = true;
  nxs->ser_cfg.baud     = CONFIG_EXAMPLES_FOC_NXSCOPE_SERIAL_BAUD;

  ret = nxscope_ser_init(&nxs->intf, &nxs->ser_cfg);
  if (ret < 0)
    {
      PRINTF("ERROR: nxscope_ser_init failed %d\n", ret);
      goto errout;
    }

  /* Default serial protocol */

  ret = nxscope_proto_ser_init(&nxs->proto, NULL);
  if (ret < 0)
    {
      PRINTF("ERROR: nxscope_proto_ser_init failed %d\n", ret);
      goto errout;
    }

  /* Initialize nxscope */

  nxs_cfg.intf_cmd      = &nxs->intf;
  nxs_cfg.proto_cmd     = &nxs->proto;
  nxs_cfg.intf_stream   = &nxs->intf;
  nxs_cfg.proto_stream  = &nxs->proto;
  nxs_cfg.channels      = CONFIG_EXAMPLES_FOC_NXSCOPE_CHANNELS;
  nxs_cfg.streambuf_len = CONFIG_EXAMPLES_FOC_NXSCOPE_STREAMBUF_LEN;
  nxs_cfg.rxbuf_len     = CONFIG_EXAMPLES_FOC_NXSCOPE_RXBUF_LEN;
  nxs_cfg.rx_padding    = CONFIG_EXAMPLES_FOC_NXSCOPE_RXPADDING;
  nxs_cfg.callbacks     = &nxs->cb;

  ret = nxscope_init(&nxs->nxs, &nxs_cfg);
  if (ret < 0)
    {
      PRINTF("ERROR: nxscope_init failed %d\n", ret);
      goto errout;
    }

  /* Get channels per instance */

  for (j = 0; j < CONFIG_MOTOR_FOC_INST; j += 1)
    {
      if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & (1 << j))
        {
          nxs->ch_per_inst += 1;
        }
    }

  /* For all FOC controllers */

  for (j = 0; j < CONFIG_MOTOR_FOC_INST; j += 1)
    {
      /* Get controller data type */

      switch (foc_thread_type(j))
        {
#if CONFIG_EXAMPLES_FOC_FLOAT_INST > 0
          case FOC_NUMBER_TYPE_FLOAT:
            {
              u.s.dtype = NXSCOPE_TYPE_FLOAT;
              break;
            }
#endif

#if CONFIG_EXAMPLES_FOC_FIXED16_INST > 0
          case FOC_NUMBER_TYPE_FIXED16:
            {
              u.s.dtype = NXSCOPE_TYPE_B16;
              break;
            }
#endif

          default:
            {
              ASSERT(0);
            }
        }

      /* Create channels */

#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG == 0)
      UNUSED(u);
#endif

#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IABC)
      nxscope_chan_init(&nxs->nxs, i++, "iabc", u.u8,
                        CONFIG_MOTOR_FOC_PHASES, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IDQ)
      nxscope_chan_init(&nxs->nxs, i++, "idq", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_IAB)
      nxscope_chan_init(&nxs->nxs, i++, "iab", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VABC)
      nxscope_chan_init(&nxs->nxs, i++, "vabc", u.u8,
                        CONFIG_MOTOR_FOC_PHASES, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VDQ)
      nxscope_chan_init(&nxs->nxs, i++, "vdq", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VAB)
      nxscope_chan_init(&nxs->nxs, i++, "vab", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_AEL)
      nxscope_chan_init(&nxs->nxs, i++, "a_el", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_AM)
      nxscope_chan_init(&nxs->nxs, i++, "a_m", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VEL)
      nxscope_chan_init(&nxs->nxs, i++, "v_el", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VM)
      nxscope_chan_init(&nxs->nxs, i++, "v_m", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VBUS)
      nxscope_chan_init(&nxs->nxs, i++, "vbus", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPTORQ)
      nxscope_chan_init(&nxs->nxs, i++, "sp_torq", u.u8, 3, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPVEL)
      nxscope_chan_init(&nxs->nxs, i++, "sp_vel", u.u8, 3, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SPPOS)
      nxscope_chan_init(&nxs->nxs, i++, "sp_pos", u.u8, 3, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_DQREF)
      nxscope_chan_init(&nxs->nxs, i++, "dqref", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VDQCOMP)
      nxscope_chan_init(&nxs->nxs, i++, "vdqcomp", u.u8, 2, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_SVM3)
      nxscope_chan_init(&nxs->nxs, i++, "svm3", u.u8, 4, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_VOBS)
      nxscope_chan_init(&nxs->nxs, i++, "vobs", u.u8, 1, 0);
#endif
#if (CONFIG_EXAMPLES_FOC_NXSCOPE_CFG & FOC_NXSCOPE_AOBS)
      nxscope_chan_init(&nxs->nxs, i++, "aobs", u.u8, 1, 0);
#endif

      if (i > CONFIG_EXAMPLES_FOC_NXSCOPE_CHANNELS)
        {
          PRINTF("ERROR: invalid nxscope channels value %d\n", i);
          ret = -ENOBUFS;
          goto errout;
        }
    }

#ifdef CONFIG_EXAMPLES_FOC_NXSCOPE_THREAD
  /* Configure thread */

  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_EXAMPLES_FOC_NXSCOPE_PRIO;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_FOC_NXSCOPE_STACKSIZE);

  /* Create nxscope thread */

  ret = pthread_create(&nxsthr, &attr, foc_nxscope_thr, nxs);
  if (ret != OK)
    {
      PRINTF("ERROR: pthread_create failed %d\n", ret);
      goto errout;
    }

  /* Set thread name */

  ret = pthread_setname_np(nxsthr, "nxsthr");
  if (ret != OK)
    {
      PRINTF("ERROR: pthread_setname_np failed %d\n", ret);
      goto errout;
    }
#endif

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_nxscope_deinit
 ****************************************************************************/

void foc_nxscope_deinit(FAR struct foc_nxscope_s *nxs)
{
  DEBUGASSERT(nxs);

  nxscope_ser_deinit(nxs->nxs.intf_cmd);
  nxscope_proto_ser_deinit(nxs->nxs.proto_cmd);
  nxscope_deinit(&nxs->nxs);
}

/****************************************************************************
 * Name: foc_nxscope_work
 ****************************************************************************/

void foc_nxscope_work(FAR struct foc_nxscope_s *nxs)
{
  int ret = OK;

  /* Flush stream data */

  ret = nxscope_stream(&nxs->nxs);
  if (ret < 0)
    {
      PRINTF("ERROR: nxscope_stream failed %d\n", ret);
    }

  ret = nxscope_recv(&nxs->nxs);
  if (ret < 0)
    {
      PRINTF("ERROR: nxscope_recv failed %d\n", ret);
    }
}
