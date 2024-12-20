/****************************************************************************
 * apps/lte/alt1250/alt1250_devif.c
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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <nuttx/modem/alt1250.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "alt1250_devif.h"
#include "alt1250_usockevent.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: ioctl_wrapper
 ****************************************************************************/

static int ioctl_wrapper(int fd, int cmd, unsigned long arg)
{
  int ret;

  ret = ioctl(fd, cmd, arg);
  if (ret < 0)
    {
      ret = -errno;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: init_alt1250_device
 ****************************************************************************/

int init_alt1250_device(void)
{
  return open(DEV_ALT1250, O_RDONLY);
}

/****************************************************************************
 * name: finalize_alt1250_device
 ****************************************************************************/

void finalize_alt1250_device(int fd)
{
  close(fd);
}

/****************************************************************************
 * name: altdevice_exchange_selcontainer
 ****************************************************************************/

FAR struct alt_container_s *altdevice_exchange_selcontainer(int fd,
    FAR struct alt_container_s *container)
{
  ioctl(fd, ALT1250_IOC_EXCHGCONTAINER, &container);
  return container;
}

/****************************************************************************
 * name: altdevice_send_command
 ****************************************************************************/

int altdevice_send_command(FAR struct alt1250_s *dev, int fd,
                           FAR struct alt_container_s *container,
                           FAR int32_t *usock_res)
{
  int ret;

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
  if (dev->is_resuming)
    {
      *usock_res = -EOPNOTSUPP;
      return REP_SEND_ACK;
    }
#endif

  ret = ioctl_wrapper(fd, ALT1250_IOC_SEND, (unsigned long)container);
  if (ret < 0)
    {
      *usock_res = ret;
      if (ret == -ENETRESET)
        {
          ret = REP_SEND_ACK_WOFREE;
        }
      else
        {
          ret = REP_SEND_ACK;
        }
    }
  else
    {
      /* In case of send successed */

      ret = container->outparam ? REP_NO_ACK_WOFREE : REP_NO_ACK;
    }

  return ret;
}

/****************************************************************************
 * name: altdevice_powercontrol
 ****************************************************************************/

int altdevice_powercontrol(int fd, uint32_t cmd)
{
  struct alt_power_s req;

  req.cmdid = cmd;
  return ioctl_wrapper(fd, ALT1250_IOC_POWER, (unsigned long)&req);
}

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
/****************************************************************************
 * name: altdevice_powerresponse
 ****************************************************************************/

int altdevice_powerresponse(int fd, uint32_t cmd, int resp)
{
  struct alt_power_s req;

  req.cmdid = cmd;
  req.resp = resp;
  return ioctl_wrapper(fd, ALT1250_IOC_POWER, (unsigned long)&req);
}
#endif

/****************************************************************************
 * name: altdevice_seteventbuff
 ****************************************************************************/

int altdevice_seteventbuff(int fd, FAR struct alt_evtbuffer_s *buffer)
{
  return (buffer) ? ioctl_wrapper(fd, ALT1250_IOC_SETEVTBUFF,
                                  (unsigned long)buffer) : -EINVAL;
}

/****************************************************************************
 * name: altdevice_getevent
 ****************************************************************************/

int altdevice_getevent(int fd, FAR uint64_t *evtbitmap,
                       FAR struct alt_container_s **replys)
{
  int ret = -EIO;
  struct alt_readdata_s dat;

  if (read(fd, &dat, sizeof(struct alt_readdata_s))
                      == sizeof(struct alt_readdata_s))
    {
      ret = OK;
      *evtbitmap = dat.evtbitmap;
      *replys = dat.head;
    }

  return ret;
}

/****************************************************************************
 * name: altdevice_reset
 ****************************************************************************/

void altdevice_reset(int fd)
{
  altdevice_powercontrol(fd, LTE_CMDID_POWEROFF);
  usleep(1);
  altdevice_powercontrol(fd, LTE_CMDID_POWERON);
}
