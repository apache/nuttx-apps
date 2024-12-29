/****************************************************************************
 * apps/examples/ble/ble_main.c
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
#include <pthread.h>
#include "sensors.h"

extern void setup_ble(void);

#define STACKSIZE 2048

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ble_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  init_sensors();
  setup_ble();

  pthread_t thread;
  pthread_attr_t attr;
  struct sched_param param;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, STACKSIZE);
  param.sched_priority = SCHED_PRIORITY_DEFAULT;
  pthread_attr_setschedparam(&attr, &param);

  ret = pthread_create(&thread, &attr, monitor_sensors, NULL);
  if (ret != 0)
    {
      printf("Error: pthread_create failed: %d\n", ret);
      return 1;
    }

  pthread_join(thread, NULL);

  close_sensors_fd();
  return 0;
}
