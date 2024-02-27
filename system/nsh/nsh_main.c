/****************************************************************************
 * apps/system/nsh/nsh_main.c
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
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/boardctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nshlib/nshlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_main
 *
 * Description:
 *   This is the main logic for the case of the NSH task.  It will perform
 *   one-time NSH initialization and start an interactive session on the
 *   current console device.
 *
 ****************************************************************************/

static void *edr_wdt_ping(pthread_addr_t pvarg)
{
    while (true) {
        usleep(10);
    }

    return NULL;
}

static uint32_t edr_wdt_start_threads(void)
{
    struct sched_param sparam;
    cpu_set_t          cpuset;
    pthread_attr_t     tattr;
    pthread_t          threadid[2];
    uint32_t           ret;

    pthread_attr_init(&tattr);
    sparam.sched_priority = 100;
    pthread_attr_setschedparam(&tattr, &sparam);
    pthread_attr_setstacksize(&tattr, 8192);

    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_attr_setaffinity_np(&tattr, sizeof(cpu_set_t), &cpuset);
    ret = pthread_create(&threadid[0], &tattr, edr_wdt_ping, NULL);
    if (ret == OK) {
        pthread_setname_np(threadid[0], "wdt_kick0");
    }

    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    pthread_attr_setaffinity_np(&tattr, sizeof(cpu_set_t), &cpuset);
    ret = pthread_create(&threadid[1], &tattr, edr_wdt_ping, NULL);
    if (ret == OK) {
        pthread_setname_np(threadid[1], "wdt_kick1");
    }

    return ret;
}

int main(int argc, FAR char *argv[])
{
  struct sched_param param;
  int ret = 0;

  /* Check the task priority that we were started with */

  sched_getparam(0, &param);
  if (param.sched_priority != CONFIG_SYSTEM_NSH_PRIORITY)
    {
      /* If not then set the priority to the configured priority */

      param.sched_priority = CONFIG_SYSTEM_NSH_PRIORITY;
      sched_setparam(0, &param);
    }

  /* Initialize the NSH library */

  nsh_initialize();

  edr_wdt_start_threads();

#ifdef CONFIG_NSH_CONSOLE
  /* If the serial console front end is selected, run it on this thread */

  ret = nsh_consolemain(argc, argv);

  /* nsh_consolemain() should not return.  So if we get here, something
   * is wrong.
   */

  dprintf(STDERR_FILENO, "ERROR: nsh_consolemain() returned: %d\n", ret);
  ret = 1;
#endif

  return ret;
}
