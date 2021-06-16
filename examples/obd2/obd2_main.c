/****************************************************************************
 * apps/examples/obd2/obd2_main.c
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

#include "canutils/obd.h"
#include "canutils/obd_pid.h"
#include "canutils/obd_frame.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * obd2_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct obd_dev_s *dev;
  int ret;

  dev = obd_init("/dev/can0", 0, 0);
  if (!dev)
    {
      printf("Failed to initialize the OBD-II device!\n");
      return -1;
    }

  /* Request the RPM */

  ret = obd_send_request(dev, OBD_SHOW_DATA, OBD_PID_RPM);
  if (ret < 0)
    {
      printf("Failed to request RPM!\n");
      return -1;
    }

  /* Wait for response */

  ret = obd_wait_response(dev, OBD_SHOW_DATA, OBD_PID_RPM, 100);
  if (ret < 0)
    {
      printf("Failed to receive the response!\n");
      return -1;
    }

  /* Decode the received PID */

  printf("RPM = %s\n", obd_decode_pid(dev, OBD_PID_RPM));

  return 0;
}
