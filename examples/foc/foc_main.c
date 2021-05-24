/****************************************************************************
 * apps/examples/foc/foc_main.c
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/boardctl.h>
#include <nuttx/fs/fs.h>

#include "foc_mq.h"
#include "foc_thr.h"
#include "foc_adc.h"
#include "foc_debug.h"
#include "foc_device.h"

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
#  include <nuttx/analog/adc.h>
#  include <nuttx/analog/ioctl.h>
#endif

#include "industry/foc/foc_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Main loop sleep */

#define MAIN_LOOP_USLEEP (200000)

/* Button init state */

#if CONFIG_EXAMPLES_FOC_STATE_INIT == 1
#  define STATE_BUTTON_I (0)
#elif CONFIG_EXAMPLES_FOC_STATE_INIT == 2
#  define STATE_BUTTON_I (2)
#elif CONFIG_EXAMPLES_FOC_STATE_INIT == 3
#  define STATE_BUTTON_I (1)
#elif CONFIG_EXAMPLES_FOC_STATE_INIT == 4
#  define STATE_BUTTON_I (3)
#else
#  error
#endif

/* Enabled instnaces default state */

#define INST_EN_DEAFULT (0xff)

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

/* Application arguments */

struct args_s
{
  int      time;                /* Run time limit in sec, -1 if forever */
  int      mode;                /* Operation mode */
  int      qparam;              /* Open-loop Q setting (x1000) */
  uint32_t pi_kp;               /* PI Kp (x1000) */
  uint32_t pi_ki;               /* PI Ki (x1000) */
  uint32_t velmax;              /* Velocity max (x1000) */
  int      state;               /* Example state (FREE, CW, CCW, STOP) */
  int8_t   en;                  /* Enabled instances (bit-encoded) */
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
/* Example state */

static const int g_state_list[5] =
{
  FOC_EXAMPLE_STATE_FREE,
  FOC_EXAMPLE_STATE_CW,
  FOC_EXAMPLE_STATE_STOP,
  FOC_EXAMPLE_STATE_CCW,
  0
};
#endif

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

#ifdef CONFIG_BUILTIN
/****************************************************************************
 * Name: foc_help
 ****************************************************************************/

static void foc_help(void)
{
  PRINTF("Usage: foc [OPTIONS]\n");
  PRINTF("  [-t] run time\n");
  PRINTF("  [-h] shows this message and exits\n");
  PRINTF("  [-m] controller mode\n");
  PRINTF("       1 - IDLE mode\n");
  PRINTF("       2 - voltage open-loop velocity \n");
  PRINTF("       3 - current open-loop velocity \n");
  PRINTF("  [-o] openloop Vq/Iq setting [x1000]\n");
  PRINTF("  [-i] PI Ki coefficient [x1000]\n");
  PRINTF("  [-p] KI Kp coefficient [x1000]\n");
  PRINTF("  [-v] velocity [x1000]\n");
  PRINTF("  [-s] motor state\n");
  PRINTF("       1 - motor free\n");
  PRINTF("       2 - motor stop\n");
  PRINTF("       3 - motor CW\n");
  PRINTF("       4 - motor CCW\n");
  PRINTF("  [-j] enable specific instnaces\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR int *value)
{
  FAR char *string;
  int       ret;

  ret = arg_string(arg, &string);
  *value = atoi(string);

  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct args_s *args, int argc, FAR char **argv)
{
  FAR char *ptr;
  int       index;
  int       nargs;
  int       i_value;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          PRINTF("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          /* Get time */

          case 't':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if (i_value <= 0 && i_value != -1)
                {
                  PRINTF("Invalid time value %d s\n", i_value);
                  exit(1);
                }

              args->time = i_value;
              break;
            }

          case 'm':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if (i_value != FOC_OPMODE_IDLE &&
                  i_value != FOC_OPMODE_OL_V_VEL &&
                  i_value != FOC_OPMODE_OL_C_VEL)
                {
                  PRINTF("Invalid op mode value %d s\n", i_value);
                  exit(1);
                }

              args->mode = i_value;
              break;
            }

          case 'o':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->qparam = i_value;
              break;
            }

          case 'p':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->pi_kp = i_value;
              break;
            }

          case 'i':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->pi_ki = i_value;
              break;
            }

          case 'v':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->velmax = i_value;
              break;
            }

          case 's':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              if (i_value != FOC_EXAMPLE_STATE_FREE &&
                  i_value != FOC_EXAMPLE_STATE_STOP &&
                  i_value != FOC_EXAMPLE_STATE_CW &&
                  i_value != FOC_EXAMPLE_STATE_CCW)
                {
                  PRINTF("Invalid state value %d s\n", i_value);
                  exit(1);
                }

              args->state = i_value;
              break;
            }

          case 'j':
            {
              nargs = arg_decimal(&argv[index], &i_value);
              index += nargs;

              args->en = i_value;
              break;
            }

          case 'h':
            {
              foc_help();
              exit(0);
            }

          default:
            {
              PRINTF("Unsupported option: %s\n", ptr);
              foc_help();
              exit(1);
            }
        }
    }
}
#endif

/****************************************************************************
 * Name: init_args
 ****************************************************************************/

static void init_args(FAR struct args_s *args)
{
  args->time =
    (args->time == 0 ? CONFIG_EXAMPLES_FOC_TIME_DEFAULT : args->time);
  args->mode =
    (args->mode == 0 ? CONFIG_EXAMPLES_FOC_OPMODE : args->mode);
  args->qparam =
    (args->qparam == 0 ? CONFIG_EXAMPLES_FOC_OPENLOOP_Q : args->qparam);
  args->pi_kp =
    (args->pi_kp == 0 ? CONFIG_EXAMPLES_FOC_IDQ_KP : args->pi_kp);
  args->pi_ki =
    (args->pi_ki == 0 ? CONFIG_EXAMPLES_FOC_IDQ_KI : args->pi_ki);
#ifdef CONFIG_EXAMPLES_FOC_VEL_ADC
  args->velmax =
    (args->velmax == 0 ? CONFIG_EXAMPLES_FOC_VEL_ADC_MAX : args->velmax);
#else
  args->velmax =
    (args->velmax == 0 ? CONFIG_EXAMPLES_FOC_VEL_CONST_VALUE : args->velmax);
#endif
  args->state =
    (args->state == 0 ? CONFIG_EXAMPLES_FOC_STATE_INIT : args->state);
  args->en = (args->en == -1 ? INST_EN_DEAFULT : args->en);
}

/****************************************************************************
 * Name: validate_args
 ****************************************************************************/

static int validate_args(FAR struct args_s *args)
{
  int ret = -EINVAL;

  if (args->pi_kp == 0 && args->pi_ki == 0)
    {
      PRINTF("ERROR: missign Kp/Ki configuration\n");
      goto errout;
    }

  /* Otherwise OK */

  ret = OK;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_mq_send
 ****************************************************************************/

static int foc_mq_send(mqd_t mqd, uint8_t msg, FAR void *data)
{
  int      ret = OK;
  uint8_t  buffer[5];
  uint32_t tmp = 0;

  DEBUGASSERT(data);

  /* Data max 4B */

  tmp = *((FAR uint32_t *) data);

  buffer[0] = msg;
  buffer[1] = ((tmp & 0x000000ff) >> 0);
  buffer[2] = ((tmp & 0x0000ff00) >> 8);
  buffer[3] = ((tmp & 0x00ff0000) >> 16);
  buffer[4] = ((tmp & 0xff000000) >> 24);

  ret = mq_send(mqd, (FAR char *)buffer, 5, 42);
  if (ret < 0)
    {
      PRINTF("foc_main: mq_send failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_vbus_send
 ****************************************************************************/

static int foc_vbus_send(mqd_t mqd, uint32_t vbus)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_VBUS, (FAR void *)&vbus);
}

/****************************************************************************
 * Name: foc_vel_send
 ****************************************************************************/

static int foc_vel_send(mqd_t mqd, uint32_t vel)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_VEL, (FAR void *)&vel);
}

/****************************************************************************
 * Name: foc_state_send
 ****************************************************************************/

static int foc_state_send(mqd_t mqd, uint32_t state)
{
  return foc_mq_send(mqd, CONTROL_MQ_MSG_APPSTATE, (FAR void *)&state);
}

/****************************************************************************
 * Name: foc_start_send
 ****************************************************************************/

static int foc_start_send(mqd_t mqd)
{
  int tmp = 0;
  return foc_mq_send(mqd, CONTROL_MQ_MSG_START, (FAR void *)&tmp);
}

/****************************************************************************
 * Name: foc_kill_send
 ****************************************************************************/

static int foc_kill_send(mqd_t mqd)
{
  int tmp = 0;
  return foc_mq_send(mqd, CONTROL_MQ_MSG_KILL, (FAR void *)&tmp);
}

/****************************************************************************
 * Name: foc_control_thr
 ****************************************************************************/

FAR void *foc_control_thr(FAR void *arg)
{
  FAR struct foc_ctrl_env_s *envp = (FAR struct foc_ctrl_env_s *) arg;
  char                       mqname[10];
  int                        ret  = OK;

  DEBUGASSERT(envp);

  /* Open FOC device as blocking */

  ret = foc_device_open(&envp->dev, envp->id);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_open failed %d!\n", ret);
      goto errout;
    }

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

  /* Close FOC control device */

  ret = foc_device_close(&envp->dev);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_device_close %d failed %d\n", envp->id, ret);
    }

  /* Close queue */

  if (envp->mqd == (mqd_t)-1)
    {
      mq_close(envp->mqd);
    }

  PRINTFV("foc_control_thr %d exit\n", envp->id);

  return NULL;
}

/****************************************************************************
 * Name: foc_threads_init
 ****************************************************************************/

static int foc_threads_init(FAR struct foc_ctrl_env_s *foc, int i,
                            FAR mqd_t *mqd, FAR pthread_t *thread)
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct foc_ctrl_env_s  foc[CONFIG_MOTOR_FOC_INST];
  pthread_t              threads[CONFIG_MOTOR_FOC_INST];
  mqd_t                  mqd[CONFIG_MOTOR_FOC_INST];
  struct args_s          args;
#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  int                    adc_fd       = 0;
  bool                   adc_trigger  = false;
  struct adc_msg_s       adc_sample[ADC_SAMPLES];
#endif
#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  btn_buttonset_t        b_sample     = 0;
  int                    b_fd         = 0;
  int                    state_i      = 0;
#endif
  uint32_t               state        = 0;
  uint32_t               vbus_raw     = 0;
  int32_t                vel_raw      = 0;
  bool                   vbus_update  = false;
  bool                   state_update = false;
  bool                   vel_update   = false;
  bool                   terminate    = false;
  bool                   started      = false;
  int                    ret          = OK;
  int                    i            = 0;
  int                    time         = 0;

  /* Reset some data */

  memset(&args, 0, sizeof(struct args_s));
  memset(mqd, 0, sizeof(mqd_t) * CONFIG_MOTOR_FOC_INST);
  memset(foc, 0, sizeof(struct foc_ctrl_env_s) * CONFIG_MOTOR_FOC_INST);
  memset(threads, 0, sizeof(pthread_t) * CONFIG_MOTOR_FOC_INST);

  /* Initialize args before parse */

  args.en = -1;

#ifdef CONFIG_BUILTIN
  /* Parse the command line */

  parse_args(&args, argc, argv);
#endif

  /* Initialize args */

  init_args(&args);

  /* Validate arguments */

  ret = validate_args(&args);
  if (ret < 0)
    {
      PRINTF("ERROR: validate args failed\n");
      goto errout_no_mutex;
    }

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#  ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#  endif
#endif

  PRINTF("\nStart foc_main application!\n\n");

  /* Initialize mutex */

  ret = pthread_mutex_init(&g_cntr_lock, NULL);
  if (ret != 0)
    {
      PRINTF("ERROR: pthread_mutex_init failed %d\n", errno);
      goto errout_no_mutex;
    }

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  /* Open ADC */

  adc_fd = open(CONFIG_EXAMPLES_FOC_ADC_DEVPATH, (O_RDONLY | O_NONBLOCK));
  if (adc_fd <= 0)
    {
      PRINTF("ERROR: failed to open %s %d\n",
             CONFIG_EXAMPLES_FOC_ADC_DEVPATH, errno);

      ret = -errno;
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  /* Open button driver */

  b_fd = open(CONFIG_EXAMPLES_FOC_BUTTON_DEVPATH, (O_RDONLY | O_NONBLOCK));
  if (b_fd < 0)
    {
      PRINTF("ERROR: failed to open %s %d\n",
             CONFIG_EXAMPLES_FOC_BUTTON_DEVPATH, errno);
      goto errout;
    }
#endif

  /* Initialzie FOC controllers */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      /* Get configuration */

      foc[i].qparam = args.qparam;
      foc[i].mode   = args.mode;
      foc[i].pi_kp  = args.pi_kp;
      foc[i].pi_ki  = args.pi_ki;
      foc[i].velmax = args.velmax;

      if (args.en & (1 << i))
        {
          /* Initialize controller thread if enabled */

          ret = foc_threads_init(&foc[i], i, &mqd[i], &threads[i]);
          if (ret < 0)
            {
              PRINTF("ERROR: foc_threads_init failed %d!\n", ret);
              goto errout;
            }
        }
    }

  /* Wait some time to finish all controllers initialziation */

  usleep(10000);

  /* Initial update for VBUS and VEL */

#ifndef CONFIG_EXAMPLES_FOC_VBUS_ADC
  vbus_update  = true;
  vbus_raw     = VBUS_CONST_VALUE;
#endif
#ifndef CONFIG_EXAMPLES_FOC_VEL_ADC
  vel_update   = true;
  vel_raw      = 1;
#endif
  state_update = true;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  /* Initial ADC trigger */

  ret = ioctl(adc_fd, ANIOC_TRIGGER, 0);
  if (ret < 0)
    {
      PRINTF("ERROR: ANIOC_TRIGGER ioctl failed: %d\n", errno);
      goto errout;
    }

  /* Make sure that conversion is done before first read form ADC device */

  usleep(10000);

  /* Read ADC data if the first loop cylce */

  adc_trigger = false;
#endif

  /* Controller state */

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  state_i = STATE_BUTTON_I;
#endif
  state = args.state;

  /* Auxliary control loop */

  while (terminate != true)
    {
      PRINTFV("foc_main loop %d\n", time);

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
      /* Get button state */

      ret = read(b_fd, &b_sample, sizeof(btn_buttonset_t));
      if (ret < 0)
        {
          if (errno != EAGAIN)
            {
              PRINTF("ERROR: read button failed %d\n", errno);
            }
        }

      /* Next state */

      if (b_sample & (1 << 0))
        {
          state_i += 1;

          if (g_state_list[state_i] == 0)
            {
              state_i = 0;
            }

          state = g_state_list[state_i];
          state_update = true;

          PRINTF("BUTTON STATE %" PRIu32 "\n", state);
        }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
      if (adc_trigger == true)
        {
          /* Issue the software trigger to start ADC conversion */

          ret = ioctl(adc_fd, ANIOC_TRIGGER, 0);
          if (ret < 0)
            {
              PRINTF("ERROR: ANIOC_TRIGGER ioctl failed: %d\n", errno);
              goto errout;
            }

          /* No ADC trigger next cycle */

          adc_trigger = false;
        }
      else
        {
          /* Get ADC samples */

          ret = read(adc_fd, adc_sample,
                     (ADC_SAMPLES * sizeof(struct adc_msg_s)));
          if (ret < 0)
            {
              if (errno != EAGAIN)
                {
                  PRINTF("ERROR: adc read failed %d\n", errno);
                }
            }
          else
            {
              /* Verify we have received the configured number of samples */

              if (ret != ADC_SAMPLES * sizeof(struct adc_msg_s))
                {
                  PRINTF("ERROR: adc read invalid read %d != %d\n",
                         ret, ADC_SAMPLES * sizeof(struct adc_msg_s));
                  ret = -EINVAL;
                  goto errout;
                }

#  ifdef CONFIG_EXAMPLES_FOC_VBUS_ADC
              /* Get raw VBUS */

              vbus_raw    = adc_sample[VBUS_ADC_SAMPLE].am_data;

              vbus_update = true;
#  endif

#  ifdef CONFIG_EXAMPLES_FOC_VEL_ADC
              /* Get raw VEL */

              vel_raw    = adc_sample[VEL_ADC_SAMPLE].am_data;

              vel_update = true;
#  endif

              /* ADC trigger next cycle */

              adc_trigger = true;
            }
        }
#endif

      /* 1. Update VBUS */

      if (vbus_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send vbus to %d\n", i);

                  /* Send VBUS to thread */

                  ret = foc_vbus_send(mqd[i], vbus_raw);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_vbus_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          vbus_update = false;
        }

      /* 2. Update motor state */

      if (state_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send state %" PRIu32 " to %d\n", state, i);

                  /* Send STATE to thread */

                  ret = foc_state_send(mqd[i], state);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_state_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          state_update = false;
        }

      /* 3. Update motor velocity */

      if (vel_update == true)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send velocity to %d\n", i);

                  /* Send VELOCITY to threads */

                  ret = foc_vel_send(mqd[i], vel_raw);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_vel_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Reset flag */

          vel_update = false;
        }

      /* 4. One time start */

      if (started == false)
        {
          for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
            {
              if (args.en & (1 << i))
                {
                  PRINTFV("Send start to %d\n", i);

                  /* Send START to threads */

                  ret = foc_start_send(mqd[i]);
                  if (ret < 0)
                    {
                      PRINTF("ERROR: foc_start_send failed %d\n", ret);
                      goto errout;
                    }
                }
            }

          /* Set flag */

          started = true;
        }

      /* Handle run time */

      time += 1;

      if (args.time != -1)
        {
          if (time >= (args.time * (1000000 / MAIN_LOOP_USLEEP)))
            {
              /* Exit loop */

              terminate = true;
            }
        }

      usleep(MAIN_LOOP_USLEEP);
    }

errout:

  /* Stop FOC control threads */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      if (args.en & (1 << i))
        {
          if (mqd[i] != (mqd_t)-1)
            {
              /* Stop thread message */

              ret = foc_kill_send(mqd[i]);
              if (ret < 0)
                {
                  PRINTF("ERROR: foc_kill_send failed %d\n", ret);
                }
            }
        }
    }

  /* Wait some time */

  usleep(100000);

  /* De-initialize all FOC control threads */

  for (i = 0; i < CONFIG_MOTOR_FOC_INST; i += 1)
    {
      if (args.en & (1 << i))
        {
          /* Close FOC control thread queue */

          if (mqd[i] != (mqd_t)-1)
            {
              mq_close(mqd[i]);
            }
        }
    }

errout_no_mutex:

  /* Free/uninitialize data structures */

  pthread_mutex_destroy(&g_cntr_lock);

  PRINTF("foc_main exit\n");

  return 0;
}
