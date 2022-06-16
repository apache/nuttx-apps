/****************************************************************************
 * apps/testing/irtest/cmd.cxx
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

#include <nuttx/lirc.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>

#include "enum.hpp"
#include "cmd.hpp"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ERROR -1

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int g_irdevs[CONFIG_TESTING_IRTEST_MAX_NIRDEV];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void wake(int)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

static void print_cmd(const cmd *cmd)
{
  printf("%s(", cmd->name);
  for (int i = 0; cmd->args[i].name; i++)
    {
      printf(i ? ", %s" : "%s", cmd->args[i].name);
    }

  printf(")\n");
}

static void print_all_cmds()
{
  for (int i = 0; g_cmd_table[i]; i++)
    {
      print_cmd(g_cmd_table[i]);
    }
}

static void print_enum(const enum_type *e)
{
  printf("%s\n", e->type);
  for (int i = 0; e->value[i].name; i++)
    {
      printf(e->fmt, e->value[i].value);
      printf(" %s\n", e->value[i].name);
    }
}

static int print_enum(const char *type)
{
  for (int i = 0; g_enum_table[i]; i++)
    {
      if (strcmp(type, g_enum_table[i]->type) == 0)
        {
          print_enum(g_enum_table[i]);
          return 0;
        }
    }

  return -ENOENT;
}

static void print_all_enums()
{
  for (int i = 0; g_enum_table[i]; i++)
    {
      print_enum(g_enum_table[i]);
    }
}

static void print_cmd_and_enum(const cmd *cmd)
{
  print_cmd(cmd);
  for (int i = 0; cmd->args[i].type; i++)
    {
      print_enum(cmd->args[i].type);
    }
}

static int print_cmd_and_enum(const char *name)
{
  for (int i = 0; g_cmd_table[i]; i++)
    {
      if (strcmp(name, g_cmd_table[i]->name) == 0)
        {
          print_cmd_and_enum(g_cmd_table[i]);
          return 0;
        }
    }

  return -ENOENT;
}

CMD0(quit)
{
  exit(0);
  return 0;
}

CMD1(sleep, float, seconds)
{
  int ret;

  pthread_mutex_lock(&mutex);
  signal(SIGINT, wake);
  if (seconds)
    {
      struct timeval now;
      struct timespec ts;

      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + seconds;
      ts.tv_nsec = now.tv_usec * 1000;
      ret = pthread_cond_timedwait(&cond, &mutex, &ts);
    }
  else
    {
      ret = pthread_cond_wait(&cond, &mutex);
    }

  pthread_mutex_unlock(&mutex);
  signal(SIGINT, SIG_DFL);
  return ret == ETIMEDOUT ? 0 : ret;
}

CMD1(help, const char *, name)
{
  int r = 0;
  if (name != 0 && *name != 0)
    {
      r = print_cmd_and_enum(name);
    }
  else
    {
      print_all_enums();
      print_all_cmds();
    }

  return r;
}

CMD1(open_device, const char *, file_name)
{
  int irdev = open(file_name, O_RDWR);
  if (irdev < 0)
    {
      return irdev;
    }

  int index = 0;
  for (; index < CONFIG_TESTING_IRTEST_MAX_NIRDEV; index++)
    {
      if (g_irdevs[index] == -1)
        {
          g_irdevs[index] = irdev;
          break;
        }
    }

  if (index == CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return index;
}

CMD1(close_device, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  if (g_irdevs[index] != -1)
    {
      close(g_irdevs[index]);
      g_irdevs[index] = -1;
      return OK;
    }

  return ERROR;
}

CMD1(write_data, size_t, index)
{
  unsigned int data[CONFIG_TESTING_IRTEST_MAX_SIRDATA];

  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  int size = 0;
  for (; size < CONFIG_TESTING_IRTEST_MAX_SIRDATA; size++)
    {
      unsigned int tmp = get_next_arg < unsigned int > ();
      if (tmp == 0)
        {
          break;
        }

      data[size] = tmp;
    }

  /* lirc require the odd length */

  if (size % 2 == 0)
    {
      int result = write(g_irdevs[index], data,
                         sizeof(unsigned int) * (size - 1));
      usleep(data[size - 1]);
      return result;
    }
  else
    {
      return write(g_irdevs[index], data,
                   sizeof(unsigned int) * size);
    }
}

CMD2(read_data, size_t, index, size_t, size)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int data[size];
  int result = read(g_irdevs[index], data, sizeof(data));
  if (result > 0)
    {
      result /= sizeof(unsigned int);
      for (int i = 0; i < result; i++)
        {
          if (i + 1 == result)
            {
              printf("%d\n", data[i]);
            }
          else
            {
              printf("%d, ", data[i]);
            }
        }
    }

  return result;
}

CMD1(get_features, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int features;
  int result = ioctl(g_irdevs[index], LIRC_GET_FEATURES, &features);
  if (result == 0)
    {
      printf("0x%08x\n", features);
    }

  return result;
}

CMD1(get_send_mode, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int mode;
  int result = ioctl(g_irdevs[index], LIRC_GET_SEND_MODE, &mode);
  if (result == 0)
    {
      printf("0x%02x\n", mode);
    }

  return result;
}

CMD1(get_rec_mode, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int mode;
  int result = ioctl(g_irdevs[index], LIRC_GET_REC_MODE, &mode);
  if (result == 0)
    {
      printf("0x%02x\n", mode);
    }

  return result;
}

CMD1(get_rec_resolution, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int resolution;
  int result = ioctl(g_irdevs[index], LIRC_GET_REC_RESOLUTION, &resolution);
  if (result == 0)
    {
      printf("%d\n", resolution);
    }

  return result;
}

CMD1(get_min_timeout, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int timeout;
  int result = ioctl(g_irdevs[index], LIRC_GET_MIN_TIMEOUT, &timeout);
  if (result == 0)
    {
      printf("%d\n", timeout);
    }

  return result;
}

CMD1(get_max_timeout, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int timeout;
  int result = ioctl(g_irdevs[index], LIRC_GET_MAX_TIMEOUT, &timeout);
  if (result == 0)
    {
      printf("%d\n", timeout);
    }

  return result;
}

CMD1(get_length, size_t, index)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  unsigned int length;
  int result = ioctl(g_irdevs[index], LIRC_GET_LENGTH, &length);
  if (result == 0)
    {
      printf("%d\n", length);
    }

  return result;
}

CMD2(set_send_mode, size_t, index, unsigned int, mode)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_SEND_MODE, &mode);
}

CMD2(set_rec_mode, size_t, index, unsigned int, mode)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_REC_MODE, &mode);
}

CMD2(set_send_carrier, size_t, index, unsigned int, carrier)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_SEND_CARRIER, &carrier);
}

CMD2(set_rec_carrier, size_t, index, unsigned int, carrier)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_REC_CARRIER, &carrier);
}

CMD2(set_send_duty_cycle, size_t, index, unsigned int, duty_cycle)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_SEND_DUTY_CYCLE, &duty_cycle);
}

CMD2(set_transmitter_mask, size_t, index, unsigned int, mask)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_TRANSMITTER_MASK, &mask);
}

CMD2(set_rec_timeout, size_t, index, unsigned int, timeout)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_REC_TIMEOUT, &timeout);
}

CMD2(set_rec_timeout_reports, size_t, index, unsigned int, enable)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_REC_TIMEOUT_REPORTS, &enable);
}

CMD2(set_measure_carrier_mode, size_t, index, unsigned int, enable)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_MEASURE_CARRIER_MODE, &enable);
}

CMD2(set_rec_carrier_range, size_t, index, unsigned int, carrier)
{
  if (index >= CONFIG_TESTING_IRTEST_MAX_NIRDEV)
    {
      return ERROR;
    }

  return ioctl(g_irdevs[index], LIRC_SET_REC_CARRIER_RANGE, &carrier);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void init_device()
{
  for (int i = 0; i < CONFIG_TESTING_IRTEST_MAX_NIRDEV; i++)
    g_irdevs[i] = -1;
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct cmd *g_cmd_table[] =
{
  /* CMD0 */

  &g_quit_cmd,

  /* CMD1 */

  &g_sleep_cmd,
  &g_help_cmd,
  &g_open_device_cmd,
  &g_close_device_cmd,
  &g_write_data_cmd,
  &g_get_features_cmd,
  &g_get_send_mode_cmd,
  &g_get_rec_mode_cmd,
  &g_get_rec_resolution_cmd,
  &g_get_min_timeout_cmd,
  &g_get_max_timeout_cmd,
  &g_get_length_cmd,

  /* CMD2 */

  &g_read_data_cmd,
  &g_set_send_mode_cmd,
  &g_set_rec_mode_cmd,
  &g_set_send_carrier_cmd,
  &g_set_rec_carrier_cmd,
  &g_set_send_duty_cycle_cmd,
  &g_set_transmitter_mask_cmd,
  &g_set_rec_timeout_cmd,
  &g_set_rec_timeout_reports_cmd,
  &g_set_measure_carrier_mode_cmd,
  &g_set_rec_carrier_range_cmd,

  /* CMD3 */

  NULL,
};
