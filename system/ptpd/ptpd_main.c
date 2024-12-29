/****************************************************************************
 * apps/system/ptpd/ptpd_main.c
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

#include <stdlib.h>
#include <stdio.h>

#include "netutils/ptpd.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int do_ptpd_start(FAR const char *interface)
{
  int pid;

  pid = ptpd_start(interface);
  if (pid < 0)
    {
      fprintf(stderr, "ERROR: ptpd_start() failed\n");
      return EXIT_FAILURE;
    }

  printf("Started the PTP daemon as PID=%d\n", pid);
  return EXIT_SUCCESS;
}

static int do_ptpd_status(int pid)
{
  struct ptpd_status_s status;
  char buf[64];
  struct tm time_tm;
  struct timespec time_now;
  int ret;

  ret = ptpd_status(pid, &status);
  if (ret != OK)
    {
      fprintf(stderr, "Failed to query PTPD status: %s\n", strerror(-ret));
      return EXIT_FAILURE;
    }

  printf("PTPD (PID %d) status:\n", pid);
  printf("- clock_source_valid: %d\n", (int)status.clock_source_valid);

  if (status.clock_source_valid)
    {
      printf("|- id: %02x %02x %02x %02x %02x %02x %02x %02x\n",
        status.clock_source_info.id[0], status.clock_source_info.id[1],
        status.clock_source_info.id[2], status.clock_source_info.id[3],
        status.clock_source_info.id[4], status.clock_source_info.id[5],
        status.clock_source_info.id[6], status.clock_source_info.id[7]
      );

      printf("|- utcoffset: %d\n", status.clock_source_info.utcoffset);
      printf("|- priority1: %d\n", status.clock_source_info.priority1);
      printf("|- class: %d\n", status.clock_source_info.clockclass);
      printf("|- accuracy: %d\n", status.clock_source_info.accuracy);
      printf("|- variance: %d\n", status.clock_source_info.variance);
      printf("|- priority2: %d\n", status.clock_source_info.priority2);

      printf("|- gm_id: %02x %02x %02x %02x %02x %02x %02x %02x\n",
        status.clock_source_info.gm_id[0], status.clock_source_info.gm_id[1],
        status.clock_source_info.gm_id[2], status.clock_source_info.gm_id[3],
        status.clock_source_info.gm_id[4], status.clock_source_info.gm_id[5],
        status.clock_source_info.gm_id[6], status.clock_source_info.gm_id[7]
      );

      printf("|- stepsremoved: %d\n", status.clock_source_info.stepsremoved);
      printf("'- timesource: %d\n", status.clock_source_info.timesource);
    }

  gmtime_r(&status.last_clock_update.tv_sec, &time_tm);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &time_tm);
  printf("- last_clock_update: %s.%09ld\n",
    buf, (long)status.last_clock_update.tv_nsec);

  printf("- last_delta_ns: %lld\n", (long long)status.last_delta_ns);
  printf("- last_adjtime_ns: %lld\n", (long long)status.last_adjtime_ns);
  printf("- drift_ppb: %ld\n", status.drift_ppb);
  printf("- path_delay_ns: %ld\n", status.path_delay_ns);

  clock_gettime(CLOCK_MONOTONIC, &time_now);

  printf("- last_received_multicast: %d s ago\n",
    (int)(time_now.tv_sec - status.last_received_multicast.tv_sec));
  printf("- last_received_announce: %d s ago\n",
    (int)(time_now.tv_sec - status.last_received_announce.tv_sec));
  printf("- last_received_sync: %d s ago\n",
    (int)(time_now.tv_sec - status.last_received_sync.tv_sec));
  printf("- last_transmitted_sync: %d s ago\n",
    (int)(time_now.tv_sec - status.last_transmitted_sync.tv_sec));
  printf("- last_transmitted_announce: %d s ago\n",
    (int)(time_now.tv_sec - status.last_transmitted_announce.tv_sec));
  printf("- last_transmitted_delayresp: %d s ago\n",
    (int)(time_now.tv_sec - status.last_transmitted_delayresp.tv_sec));
  printf("- last_transmitted_delayreq: %d s ago\n",
    (int)(time_now.tv_sec - status.last_transmitted_delayreq.tv_sec));

  return EXIT_SUCCESS;
}

int do_ptpd_stop(int pid)
{
  int ret;

  ret = ptpd_stop(pid);

  if (ret == OK)
    {
      printf("Stopped ptpd\n");
      return EXIT_SUCCESS;
    }
  else
    {
      printf("Failed to stop ptpd: %s\n", strerror(-ret));
      return EXIT_FAILURE;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ptpd_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc == 3 && strcmp(argv[1], "start") == 0)
    {
      return do_ptpd_start(argv[2]);
    }
  else if (argc == 3 && strcmp(argv[1], "status") == 0)
    {
      return do_ptpd_status(atoi(argv[2]));
    }
  else if (argc == 3 && strcmp(argv[1], "stop") == 0)
    {
      return do_ptpd_stop(atoi(argv[2]));
    }
  else
    {
      fprintf(stderr, "Usage: \n"
                      "ptpd start <interface>\n"
                      "ptpd status <pid>\n"
                      "ptpd stop <pid>\n");
      return EXIT_FAILURE;
    }
}
