/****************************************************************************
 * apps/canutils/canlib/canlib_setbaud.c
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

#include <sys/ioctl.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/can/can.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: canlib_setbaud
 *
 * Description:
 *   Wrapper for CANIOC_SET_BITTIMING
 *
 * Input Parameter:
 *   fd   - file descriptor of an opened can device
 *   baud - baud rate to use on the CAN bus
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_setbaud(int fd, int bauds)
{
  int ret;
  struct canioc_bittiming_s timings;

  ret = ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)&timings);
  if (ret != OK)
    {
      canerr("CANIOC_GET_BITTIMING failed, errno=%d\n", errno);
      return ret;
    }

  timings.bt_baud = bauds;

  ret = ioctl(fd, CANIOC_SET_BITTIMING, (unsigned long)&timings);
  if (ret != OK)
    {
      canerr("CANIOC_SET_BITTIMING failed, errno=%d\n", errno);
    }

  return ret;
}
