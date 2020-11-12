/****************************************************************************
 * testing/irtest/main.cpp
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

#include <stdio.h>

#include "system/readline.h"
#include "cmd.hpp"
#include "enum.hpp"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern cmd *quit_cmd;
extern cmd *sleep_cmd;
extern cmd *help_cmd;
extern cmd *open_device_cmd;
extern cmd *close_device_cmd;
extern cmd *write_data_cmd;
extern cmd *get_features_cmd;
extern cmd *get_send_mode_cmd;
extern cmd *get_rec_mode_cmd;
extern cmd *get_rec_resolution_cmd;
extern cmd *get_min_timeout_cmd;
extern cmd *get_max_timeout_cmd;
extern cmd *get_length_cmd;
extern cmd *read_data_cmd;
extern cmd *set_send_mode_cmd;
extern cmd *set_rec_mode_cmd;
extern cmd *set_send_carrier_cmd;
extern cmd *set_rec_carrier_cmd;
extern cmd *set_send_duty_cycle_cmd;
extern cmd *set_transmitter_mask_cmd;
extern cmd *set_rec_timeout_cmd;
extern cmd *set_rec_timeout_reports_cmd;
extern cmd *set_measure_carrier_mode_cmd;
extern cmd *set_rec_carrier_range_cmd;
extern enum_type *mode_t_type;
extern enum_type *features_t_type;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct enum_type *g_enum_table[] =
{
  mode_t_type,
  features_t_type,
};

static const struct cmd *g_cmd_table[] =
{
  /* CMD0 */

  quit_cmd,

  /* CMD1 */

  sleep_cmd,
  help_cmd,
  open_device_cmd,
  close_device_cmd,
  write_data_cmd,
  get_features_cmd,
  get_send_mode_cmd,
  get_rec_mode_cmd,
  get_rec_resolution_cmd,
  get_min_timeout_cmd,
  get_max_timeout_cmd,
  get_length_cmd,

  /* CMD2 */

  read_data_cmd,
  set_send_mode_cmd,
  set_rec_mode_cmd,
  set_send_carrier_cmd,
  set_rec_carrier_cmd,
  set_send_duty_cycle_cmd,
  set_transmitter_mask_cmd,
  set_rec_timeout_cmd,
  set_rec_timeout_reports_cmd,
  set_measure_carrier_mode_cmd,
  set_rec_carrier_range_cmd,

  /* CMD3 */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t prompt(const char *p, char *buf, size_t len)
{
  fputs(p, stdout);
  fflush(stdout);
  return readline(buf, len, stdin, stdout);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C"
{
int main(int argc, char *argv[])
{
  char cmdline[512];

  init_device();

  while (prompt("$", cmdline, sizeof(cmdline)))
    {
      const char *name = get_fisrt_arg(cmdline);
      if (name != 0 && *name != 0)
        {
          int res =  -ENOSYS;
          const cmd *cmd = &__start_cmds;
          for (; cmd < &__stop_cmds; cmd++)
            {
              if (strcmp(name, cmd->name) == 0)
                {
                  res = cmd->exec();
                  break;
                }
            }

          if (res < 0)
            {
              printf("%s: %s(%d)\n", name, strerror(-res), res);
            }
        }
    }

  return 0;
}
}
