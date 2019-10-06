/****************************************************************************
 * examples/hello/abntcodi_main.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  printf("Capacitive Reactive Pulses are used to calculate consumption: %s\n",
         boolstr(proto->react_cap_pulse));
  printf("Inductive Reactive Pulses are used to calculate consumption: %s\n",
         boolstr(proto->react_ind_pulse));
  printf("Segment type: %s\n",
         proto->segment_type == SEGMENT_PEEK     ? "PEEK"        :
         proto->segment_type == SEGMENT_OUT_PEEK ? "OUT OF PEEK" :
         proto->segment_type == SEGMENT_RESERVED ? "RESERVED"    : "UNKNOWN");
  printf("Charges type: %s\n",
         proto->charge_type == CHARGES_BLUE        ? "BLUE"       :
         proto->charge_type == CHARGES_GREEN       ? "GREEN"      :
         proto->charge_type == CHARGES_IRRIGATORS  ? "IRRIGATORS" : "OTHERS");
  printf("Number of Active pulses since the beginning of current demand: %d\n",
         proto->pulses_act_dem);
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
