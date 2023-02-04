/****************************************************************************
 * apps/examples/abntcodi/abntcodi_main.c
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
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>

#include "industry/abnt_codi.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void print_abnt_codi(FAR struct abnt_codi_proto_s *proto)
{
  printf("Seconds missing to end of the active demand: %d\n",
         proto->end_act_dem);
  printf("Current Bill Indicator: %d\n", proto->bill_indicator);
  printf("Reactive Interval Indicator: %d\n", proto->react_interval);
  printf("Capacitive Reactive Pulses are used to calculate consumption: "
         "%s\n", boolstr(proto->react_cap_pulse));
  printf("Inductive Reactive Pulses are used to calculate consumption: %s\n",
         boolstr(proto->react_ind_pulse));
  printf("Segment type: %s\n",
         proto->segment_type == SEGMENT_PEEK     ? "PEEK"        :
         proto->segment_type == SEGMENT_OUT_PEEK ? "OUT OF PEEK" :
         proto->segment_type == SEGMENT_RESERVED ? "RESERVED"    :
                                                   "UNKNOWN");
  printf("Charges type: %s\n",
         proto->charge_type == CHARGES_BLUE        ? "BLUE"       :
         proto->charge_type == CHARGES_GREEN       ? "GREEN"      :
         proto->charge_type == CHARGES_IRRIGATORS  ? "IRRIGATORS" :
                                                     "OTHERS");
  printf("Number of Active pulses since the beginning of current demand: "
         "%d\n", proto->pulses_act_dem);
  printf("Number of Reactive pulses since beginning of current demand: %d\n",
         proto->pulses_react_dem);
}

/****************************************************************************
 * abntcodi_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct abnt_codi_proto_s *proto;
  int fd;
  int cnt;
  int ret;
  uint8_t byte;
  uint8_t data[8];

  /* Allocate memory to protocol struct */

  proto = malloc(sizeof(struct abnt_codi_proto_s));
  if (!proto)
    {
      printf("Failed to allocate memory to abnt_codi_proto_s!\n");
      return -ENOMEM;
    }

  /* Open the serial port used to read ABNT CODI (Baudrate: 110bps) */

  fd = open("/dev/ttyS1", O_RDONLY);
  if (fd < 0)
    {
      printf("Unable to open file /dev/ttyS1\n");
      return -ENODEV;
    }

  /* Run forever */

  for (; ; )
    {
      /* Read until we complete a sequence of 8 bytes */

      cnt = 0;
      do
        {
          ret = read(fd, &byte, 1);
          if (ret != 1)
            {
              continue;
            }

          data[cnt++] = byte;
        }
      while (cnt < 8);

      /* Parse the received data */

      if (abnt_codi_parse(data, proto))
        {
          print_abnt_codi(proto);
        }

      /* Avoid busy wait using 100% CPU cycles */

      usleep(5000000);
    }

  return 0;
}
