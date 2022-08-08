/****************************************************************************
 * apps/games/shift/shift_input_joystick.h
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

#include <nuttx/input/djoystick.h>

#include "shift_inputs.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define DJOYSTICK_DEVNAME "/dev/djoy0"
#define DJOYSTICK_SIGNO 13

/* The set of supported joystick discretes */

static djoy_buttonset_t g_djoysupported;

/* Last sampled discrete set */

static djoy_buttonset_t g_djoylast;

struct djoy_notify_s notify;

/****************************************************************************
 * dev_input_init
 ****************************************************************************/

int dev_input_init(FAR struct input_state_s *dev)
{
  int tmp;
  int ret;

  /* Open the djoystick device */

  dev->fd_joy = open(DJOYSTICK_DEVNAME, O_RDONLY);
  if (dev->fd_joy < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              DJOYSTICK_DEVNAME, errno);
      return -ENODEV;
    }

  /* Get the set of supported discretes */

  ret = ioctl(dev->fd_joy, DJOYIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&tmp));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(DJOYIOC_SUPPORTED) failed: %d\n", errno);
      goto errout_with_fd;
    }

  g_djoysupported = (djoy_buttonset_t)tmp;

  /* Register to receive a signal on any change in the joystick discretes. */

  notify.dn_press   = g_djoysupported;
  notify.dn_release = g_djoysupported;

  notify.dn_event.sigev_notify = SIGEV_SIGNAL;
  notify.dn_event.sigev_signo  = DJOYSTICK_SIGNO;

  ret = ioctl(dev->fd_joy, DJOYIOC_REGISTER,
              (unsigned long)((uintptr_t)&notify));
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ioctl(DJOYIOC_REGISTER) failed: %d\n", errno);
      goto errout_with_fd;
    }

  /* Ignore the default signal action */

  signal(DJOYSTICK_SIGNO, SIG_IGN);

  return OK;

errout_with_fd:
  close(dev->fd_joy);
  return -ENODEV;
}

/****************************************************************************
 * dev_read_input
 ****************************************************************************/

int dev_read_input(FAR struct input_state_s *dev)
{
  struct siginfo value;
  sigset_t set;
  djoy_buttonset_t newset;
  ssize_t nread;
  int ret;

  /* Wait for a signal */

  sigemptyset(&set);
  sigaddset(&set, DJOYSTICK_SIGNO);
  ret = sigwaitinfo(&set, &value);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: sigwaitinfo() failed: %d\n", errno);
    }

  newset = (djoy_buttonset_t)value.si_value.sival_int;

  g_djoylast = newset;

  /* Read the signal set and compare */

  nread = read(dev->fd_joy, &newset, sizeof(djoy_buttonset_t));
  if (nread < 0)
    {
      fprintf(stderr, "ERROR: read() failed: %d\n", errno);
    }
  else if (nread != sizeof(djoy_buttonset_t))
    {
      fprintf(stderr, "ERROR: read() unexpected size: %ld vs %d\n",
              (long)nread, sizeof(djoy_buttonset_t));
    }

  g_djoylast = newset;

  switch (newset)
    {
      case DJOY_UP_BIT:
        dev->dir = DIR_UP;
        break;
      case DJOY_DOWN_BIT:
        dev->dir = DIR_DOWN;
        break;
      case DJOY_LEFT_BIT:
        dev->dir = DIR_LEFT;
        break;
      case DJOY_RIGHT_BIT:
        dev->dir = DIR_RIGHT;
        break;
      default:
        dev->dir = DIR_NONE;
    }

  return OK;
}
