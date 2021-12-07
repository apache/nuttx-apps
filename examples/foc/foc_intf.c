/****************************************************************************
 * apps/examples/foc/foc_intf.c
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
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <sys/ioctl.h>

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
#  include <nuttx/analog/adc.h>
#  include <nuttx/analog/ioctl.h>
#endif

#include "foc_thr.h"
#include "foc_adc.h"
#include "foc_intf.h"
#include "foc_debug.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_FOC_HAVE_ADC) ||      \
    defined(CONFIG_EXAMPLES_FOC_HAVE_BUTTON) ||   \
    defined(CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL)
#  define FOC_HAVE_INTF
#endif

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

/****************************************************************************
 * Private Type Definition
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
/* Button interface data */

struct foc_intf_btn_s
{
  int fd;
};
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
/* ADC interface data */

struct foc_intf_adc_s
{
  int  fd;
  bool adc_trigger;
};
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
/* Character control interface data */

struct foc_intf_chc_s
{
  int fd;
};
#endif

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

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
/* Button interface data */

static struct foc_intf_btn_s g_btn_intf;
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
/* ADC interface data */

static struct foc_intf_adc_s g_adc_intf;
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
/* Character control interface data */

static struct foc_intf_chc_s g_chc_intf;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
/****************************************************************************
 * Name: foc_button_init
 ****************************************************************************/

static int foc_button_init(FAR struct foc_intf_btn_s *intf)
{
  int ret = 0;

  DEBUGASSERT(intf);

  /* Open button driver */

  intf->fd = open(CONFIG_EXAMPLES_FOC_BUTTON_DEVPATH,
                  (O_RDONLY | O_NONBLOCK));
  if (intf->fd < 0)
    {
      PRINTF("ERROR: failed to open %s %d\n",
             CONFIG_EXAMPLES_FOC_BUTTON_DEVPATH, errno);
      ret = -errno;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_button_deinit
 ****************************************************************************/

static int foc_button_deinit(FAR struct foc_intf_btn_s *intf)
{
  DEBUGASSERT(intf);

  if (intf->fd > 0)
    {
      close(intf->fd);
    }

  return OK;
}

/****************************************************************************
 * Name: foc_button_update
 ****************************************************************************/

static int foc_button_update(FAR struct foc_intf_btn_s *intf,
                             FAR struct foc_intf_data_s *data)
{
  static int      state_i  = STATE_BUTTON_I;
  btn_buttonset_t b_sample = 0;
  int             ret      = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(data);

  /* Get button state */

  ret = read(intf->fd, &b_sample, sizeof(btn_buttonset_t));
  if (ret < 0)
    {
      if (errno != EAGAIN)
        {
          PRINTF("ERROR: read button failed %d\n", errno);
          ret = -errno;
          goto errout;
        }
      else
        {
          ret = OK;
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

      data->state        = g_state_list[state_i];
      data->state_update = true;

      PRINTF("BUTTON STATE %" PRIu32 "\n", data->state);
    }

errout:
  return ret;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
/****************************************************************************
 * Name: foc_adc_init
 ****************************************************************************/

static int foc_adc_init(FAR struct foc_intf_adc_s *intf)
{
  int ret = OK;

  DEBUGASSERT(intf);

  /* Open ADC device */

  intf->fd = open(CONFIG_EXAMPLES_FOC_ADC_DEVPATH, (O_RDONLY | O_NONBLOCK));
  if (intf->fd <= 0)
    {
      PRINTF("ERROR: failed to open %s %d\n",
             CONFIG_EXAMPLES_FOC_ADC_DEVPATH, errno);
      ret = -errno;
      goto errout;
    }

  /* Initial ADC trigger */

  ret = ioctl(intf->fd, ANIOC_TRIGGER, 0);
  if (ret < 0)
    {
      PRINTF("ERROR: ANIOC_TRIGGER ioctl failed: %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* Make sure that conversion is done before first read form ADC device */

  usleep(10000);

  /* Read ADC data if the first loop cylce */

  intf->adc_trigger = false;

errout:
  return ret;
}

/****************************************************************************
 * Name: foc_adc_deinit
 ****************************************************************************/

static int foc_adc_deinit(FAR struct foc_intf_adc_s *intf)
{
  DEBUGASSERT(intf);

  if (intf->fd > 0)
    {
      close(intf->fd);
    }

  return OK;
}

/****************************************************************************
 * Name: foc_adc_update
 ****************************************************************************/

static int foc_adc_update(FAR struct foc_intf_adc_s *intf,
                          FAR struct foc_intf_data_s *data)
{
  struct adc_msg_s adc_sample[ADC_SAMPLES];
  int              ret = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(data);

  if (intf->adc_trigger == true)
    {
      /* Issue the software trigger to start ADC conversion */

      ret = ioctl(intf->fd, ANIOC_TRIGGER, 0);
      if (ret < 0)
        {
          PRINTF("ERROR: ANIOC_TRIGGER ioctl failed: %d\n", errno);
          ret = -errno;
          goto errout;
        }

      /* No ADC trigger next cycle */

      intf->adc_trigger = false;
    }
  else
    {
      /* Get ADC samples */

      ret = read(intf->fd, adc_sample,
                 (ADC_SAMPLES * sizeof(struct adc_msg_s)));
      if (ret < 0)
        {
          if (errno != EAGAIN)
            {
              PRINTF("ERROR: adc read failed %d\n", errno);
              ret = -errno;
              goto errout;
            }
          else
            {
              ret = OK;
            }
        }
      else
        {
          /* Verify we have received the configured number of samples */

          if (ret != ADC_SAMPLES * sizeof(struct adc_msg_s))
            {
              PRINTF("ERROR: adc read invalid read %d != %d\n", ret,
                     ADC_SAMPLES * sizeof(struct adc_msg_s));
              ret = -EINVAL;
              goto errout;
            }

#ifdef CONFIG_EXAMPLES_FOC_VBUS_ADC
          /* Get raw VBUS */

          data->vbus_raw = adc_sample[VBUS_ADC_SAMPLE].am_data;

          data->vbus_update = true;
#endif

#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_ADC
          /* Get raw VEL */

          data->sp_raw = adc_sample[SETPOINT_ADC_SAMPLE].am_data;

          data->sp_update = true;
#endif

          /* ADC trigger next cycle */

          intf->adc_trigger = true;
        }
    }

errout:
  return ret;
}
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
/****************************************************************************
 * Name: foc_charctrl_help
 ****************************************************************************/

static void foc_charctrl_help(void)
{
  PRINTF("\nCHARCTRL commands:\n"
         "  h - print this message\n"
         "  u - increase setpoint\n"
         "  d - decrease setpoint\n"
         "  q - quit\n"
         "  1..4 - change example state\n\n");
}

/****************************************************************************
 * Name: foc_charctrl_init
 ****************************************************************************/

static int foc_charctrl_init(FAR struct foc_intf_chc_s *intf)
{
  int ret = 0;

  DEBUGASSERT(intf);

  /* Character control interface on STDIN */

  intf->fd = 0;

  /* Print help msg */

  PRINTF("\nCHARCTRL mode enabled\n");
  foc_charctrl_help();

  /* Configure input to not blocking */

  fcntl(intf->fd, F_SETFL, O_NONBLOCK);

  return ret;
}

/****************************************************************************
 * Name: foc_charctrl_deinit
 ****************************************************************************/

static int foc_charctrl_deinit(FAR struct foc_intf_chc_s *intf)
{
  DEBUGASSERT(intf);

  if (intf->fd > 0)
    {
      close(intf->fd);
    }

  return OK;
}

/****************************************************************************
 * Name: foc_charctrl_update
 ****************************************************************************/

static int foc_charctrl_update(FAR struct foc_intf_chc_s *intf,
                               FAR struct foc_intf_data_s *data)
{
  char c   = 0;
  int  ret = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(data);

  ret = read(intf->fd, &c, 1);
  if (ret < 0)
    {
      if (errno != EAGAIN)
        {
          PRINTF("ERROR: read char failed %d\n", errno);
          ret = -errno;
          goto errout;
        }
      else
        {
          ret = OK;
        }
    }
  else
    {
      switch (c)
        {
#ifdef CONFIG_EXAMPLES_FOC_SETPOINT_CHAR
          case 'u':
            {
              data->sp_raw += CONFIG_EXAMPLES_FOC_CHAR_SETPOINT_STEP;

              if (data->sp_raw > CONFIG_EXAMPLES_FOC_SETPOINT_MAX)
                {
                  data->sp_raw = CONFIG_EXAMPLES_FOC_SETPOINT_MAX;
                }

              PRINTF(">> sp=%" PRIu32 "\n", data->sp_raw);
              data->sp_update = true;

              break;
            }

          case 'd':
            {
              data->sp_raw -= CONFIG_EXAMPLES_FOC_CHAR_SETPOINT_STEP;

              if (data->sp_raw < 0)
                {
                  data->sp_raw = 0;
                }

              PRINTF(">> sp=%" PRIu32 "\n", data->sp_raw);
              data->sp_update = true;

              break;
            }
#endif /* CONFIG_EXAMPLES_FOC_SETPOINT_CHAR */

          case 'h':
            {
              foc_charctrl_help();

              break;
            }

          case 'q':
            {
              PRINTF(">> QUIT\n");
              data->terminate = true;
              break;
            }

          case '1':
          case '2':
          case '3':
          case '4':
            {
              data->state = c - '1' + 1;
              PRINTF(">> state=%" PRIu32 "\n", data->state);
              data->state_update = true;

              break;
            }

          default:
            {
              PRINTF("ERROR: invalid cmd=%d\n", c);
              break;
            }
        }
    }

errout:
  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: foc_intf_init
 ****************************************************************************/

int foc_intf_init(void)
{
  int ret = OK;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  /* Initialize ADC interface */

  ret = foc_adc_init(&g_adc_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: failed to initialize adc interface %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  /* Initialize button interface */

  ret = foc_button_init(&g_btn_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: failed to initialize button interface %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
  /* Initialize character control interface */

  ret = foc_charctrl_init(&g_chc_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: failed to initialize char interface %d\n", ret);
      goto errout;
    }
#endif

#ifdef FOC_HAVE_INTF
  errout:
#endif

  return ret;
}

/****************************************************************************
 * Name: foc_intf_deinit
 ****************************************************************************/

int foc_intf_deinit(void)
{
  int ret = OK;

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  /* De-initialize adc interface */

  ret = foc_adc_deinit(&g_adc_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_adc_deinit failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  /* De-initialize button interface */

  ret = foc_button_deinit(&g_btn_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_button_deinit failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
  /* De-initialize character interface */

  ret = foc_charctrl_deinit(&g_chc_intf);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_charctrl_deinit failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef FOC_HAVE_INTF
  errout:
#endif

  return ret;
}

/****************************************************************************
 * Name: foc_intf_update
 ****************************************************************************/

int foc_intf_update(FAR struct foc_intf_data_s *data)
{
  int ret = OK;

  DEBUGASSERT(data);

#ifdef CONFIG_EXAMPLES_FOC_HAVE_CHARCTRL
  /* Update charctrl */

  ret = foc_charctrl_update(&g_chc_intf, data);
  if (ret < 0)
    {
      PRINTF("ERROR: foc_charctrl_update failed: %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_BUTTON
  /* Update button */

  ret = foc_button_update(&g_btn_intf, data);
  if (ret < 0)
    {
      PRINTF("ERROR: button update failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef CONFIG_EXAMPLES_FOC_HAVE_ADC
  /* Update ADC */

  ret = foc_adc_update(&g_adc_intf, data);
  if (ret < 0)
    {
      PRINTF("ERROR: adc update failed %d\n", ret);
      goto errout;
    }
#endif

#ifdef FOC_HAVE_INTF
  errout:
#endif

  return ret;
}
