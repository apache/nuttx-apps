/****************************************************************************
 * apps/examples/foc/foc_thr.c
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

#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "foc_mq.h"
#include "foc_thr.h"
#include "foc_debug.h"

#include "industry/foc/foc_common.h"

/****************************************************************************
 * Extern Functions Prototypes
 ****************************************************************************/

#ifdef CONFIG_INDUSTRY_FOC_FLOAT
extern int foc_float_thr(FAR struct foc_ctrl_env_s *envp);
#endif

#ifdef CONFIG_INDUSTRY_FOC_FIXED16
extern int foc_fixed16_thr(FAR struct foc_ctrl_env_s *envp);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

pthread_mutex_t g_cntr_lock;

#ifdef CONFIG_INDUSTRY_FOC_FLOAT
static int g_float_thr_cntr = 0;
#endif
#ifdef CONFIG_INDUSTRY_FOC_FIXED16
static int g_fixed16_thr_cntr = 0;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_control_thr
 ****************************************************************************/

static FAR void *foc_control_thr(FAR void *arg)
{
  FAR struct foc_ctrl_env_s *envp = (FAR struct foc_ctrl_env_s *) arg;
  char                       mqname[10];
  int                        ret  = OK;

  DEBUGASSERT(envp);

  /* Get controller type */

  pthread_mutex_lock(&g_cntr_lock);

#ifdef CONFIG_INDUSTRY_FOC_FLOAT
  if (g_float_thr_cntr < CONFIG_EXAMPLES_FOC_FLOAT_INST)
    {
      envp->type = FOC_NUMBER_TYPE_FLOAT;
    }
  else
#endif
#ifdef CONFIG_INDUSTRY_FOC_FIXED16
  if (g_fixed16_thr_cntr < CONFIG_EXAMPLES_FOC_FIXED16_INST)
    {
      envp->type = FOC_NUMBER_TYPE_FIXED16;
    }
  else
#endif
    {
      /* Invalid configuration */

      ASSERT(0);
    }

  pthread_mutex_unlock(&g_cntr_lock);

  PRINTF("FOC device %d type = %d!\n", envp->id, envp->type);

  /* Get queue name */

  sprintf(mqname, "%s%d", CONTROL_MQ_MQNAME, envp->id);

  /* Open queue */

  envp->mqd = mq_open(mqname, (O_RDONLY | O_NONBLOCK), 0666, NULL);
  if (envp->mqd == (mqd_t)-1)
    {
      PRINTF("ERROR: mq_open failed errno=%d\n", errno);
      goto errout;
    }

  /* Select control logic according to FOC device type */

  switch (envp->type)
    {
#ifdef CONFIG_INDUSTRY_FOC_FLOAT
      case FOC_NUMBER_TYPE_FLOAT:
        {
          pthread_mutex_lock(&g_cntr_lock);
          envp->inst = g_float_thr_cntr;
          g_float_thr_cntr += 1;
          pthread_mutex_unlock(&g_cntr_lock);

          /* Start thread */

          ret = foc_float_thr(envp);

          pthread_mutex_lock(&g_cntr_lock);
          g_float_thr_cntr -= 1;
          pthread_mutex_unlock(&g_cntr_lock);

          break;
        }
#endif

#ifdef CONFIG_INDUSTRY_FOC_FIXED16
      case FOC_NUMBER_TYPE_FIXED16:
        {
          pthread_mutex_lock(&g_cntr_lock);
          envp->inst = g_fixed16_thr_cntr;
          g_fixed16_thr_cntr += 1;
          pthread_mutex_unlock(&g_cntr_lock);

          /* Start thread */

          ret = foc_fixed16_thr(envp);

          pthread_mutex_lock(&g_cntr_lock);
          g_fixed16_thr_cntr -= 1;
          pthread_mutex_unlock(&g_cntr_lock);

          break;
        }
#endif

      default:
        {
          PRINTF("ERROR: unknown FOC device type %d\n", envp->type);
          goto errout;
        }
    }

  if (ret < 0)
    {
      PRINTF("ERROR: foc control thread failed %d\n", ret);
    }

errout:

  /* Close queue */

  if (envp->mqd == (mqd_t)-1)
    {
      mq_close(envp->mqd);
    }

  PRINTFV("foc_control_thr %d exit\n", envp->id);

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_threads_init
 ****************************************************************************/

int foc_threads_init(void)
{
  int ret = OK;

  /* Initialize mutex */

  ret = pthread_mutex_init(&g_cntr_lock, NULL);
  if (ret != 0)
    {
      PRINTF("ERROR: pthread_mutex_init failed %d\n", errno);
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_threads_deinit
 ****************************************************************************/

void foc_threads_deinit(void)
{
  /* Free/uninitialize data structures */

  pthread_mutex_destroy(&g_cntr_lock);
}

/****************************************************************************
 * Name: foc_threads_terminated
 ****************************************************************************/

bool foc_threads_terminated(void)
{
  bool ret = false;

  pthread_mutex_unlock(&g_cntr_lock);

  if (1
#ifdef CONFIG_INDUSTRY_FOC_FLOAT
      && g_float_thr_cntr <= 0
#endif
#ifdef CONFIG_INDUSTRY_FOC_FIXED16
      && g_fixed16_thr_cntr <= 0
#endif
    )
    {
      ret = true;
    }

  pthread_mutex_lock(&g_cntr_lock);

  return ret;
}

/****************************************************************************
 * Name: foc_ctrlthr_init
 ****************************************************************************/

int foc_ctrlthr_init(FAR struct foc_ctrl_env_s *foc, int i, FAR mqd_t *mqd,
                     FAR pthread_t *thread)
{
  char                mqname[10];
  int                 ret = OK;
  pthread_attr_t      attr;
  struct mq_attr      mqattr;
  struct sched_param  param;

  DEBUGASSERT(foc);
  DEBUGASSERT(mqd);
  DEBUGASSERT(thread);

  /* Store device id */

  foc->id = i;

  /* Fill in attributes for message queue */

  mqattr.mq_maxmsg  = CONTROL_MQ_MAXMSG;
  mqattr.mq_msgsize = CONTROL_MQ_MSGSIZE;
  mqattr.mq_flags   = 0;

  /* Get queue name */

  sprintf(mqname, "%s%d", CONTROL_MQ_MQNAME, foc->id);

  /* Initialize thread recv queue */

  *mqd = mq_open(mqname, (O_WRONLY | O_CREAT | O_NONBLOCK),
                 0666, &mqattr);
  if (*mqd < 0)
    {
      PRINTF("ERROR: mq_open %s failed errno=%d\n", mqname, errno);
      goto errout;
    }

  /* Configure thread */

  pthread_attr_init(&attr);
  param.sched_priority = CONFIG_EXAMPLES_FOC_CONTROL_PRIO;
  pthread_attr_setschedparam(&attr, &param);
  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_FOC_CONTROL_STACKSIZE);

  /* Create FOC threads */

  ret = pthread_create(thread, &attr, foc_control_thr, foc);
  if (ret != 0)
    {
      PRINTF("ERROR: pthread_create ctrl failed %d\n", ret);
      ret = -ret;
      goto errout;
    }

errout:
  return ret;
}
