/****************************************************************************
 * apps/games/snake/snake_input_gpio.h
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

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/ioexpander/gpio.h>

#include "snake_inputs.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

struct gpio_struct_fd_s
{
    int fd_up;    /* File descriptor value to read up arrow key */
    int fd_down;  /* File descriptor value to read down arrow key */
    int fd_left;  /* File descriptor value to read left arrow key */
    int fd_right; /* File descriptor value to read right arrow key */
};

struct gpio_struct_fd_s fd_list;

/****************************************************************************
 * Name: dev_input_init
 *
 * Description:
 *   Initialize input method.
 *
 * Parameters:
 *   dev - Input state data
 *
 * Returned Value:
 *   Zero (OK) is returned on success. A negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int dev_input_init(FAR struct input_state_s *dev)
{
  /* Open the up key gpio device */

  fd_list.fd_up = open(CONFIG_GAMES_SNAKE_UP_KEY_PATH, O_RDONLY);
  if (fd_list.fd_up < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_GAMES_SNAKE_UP_KEY_PATH, errno);
      return -ENODEV;
    }

  /* Open the down key gpio device */

  fd_list.fd_down = open(CONFIG_GAMES_SNAKE_DOWN_KEY_PATH, O_RDONLY);
  if (fd_list.fd_down < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_GAMES_SNAKE_DOWN_KEY_PATH, errno);
      return -ENODEV;
    }

  /* Open the left key gpio device */

  fd_list.fd_left = open(CONFIG_GAMES_SNAKE_LEFT_KEY_PATH, O_RDONLY);
  if (fd_list.fd_down < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_GAMES_SNAKE_LEFT_KEY_PATH, errno);
      return -ENODEV;
    }

  /* Open the right key gpio device */

  fd_list.fd_right = open(CONFIG_GAMES_SNAKE_RIGHT_KEY_PATH, O_RDONLY);
  if (fd_list.fd_down < 0)
    {
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              CONFIG_GAMES_SNAKE_RIGHT_KEY_PATH, errno);
      return -ENODEV;
    }

  dev->fd_gpio = (int)&fd_list;

  return OK;
}

/****************************************************************************
 * Name: dev_read_input
 *
 * Description:
 *   Read inputs and returns result in input state data.
 *
 * Parameters:
 *   dev - Input state data
 *
 * Returned Value:
 *   Zero (OK)
 *
 ****************************************************************************/

int dev_read_input(FAR struct input_state_s *dev)
{
  struct gpio_struct_fd_s *fd = (struct gpio_struct_fd_s *)dev->fd_gpio;
  int invalue = 0;
  int ret;

  ret = ioctl(fd->fd_up, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read value from %s: %d\n",
              CONFIG_GAMES_SNAKE_UP_KEY_PATH, errcode);
    }
  else
    {
      if (invalue != 0)
        {
          dev->dir = DIR_UP;
        }
    }

  ret = ioctl(fd->fd_down, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read value from %s: %d\n",
              CONFIG_GAMES_SNAKE_DOWN_KEY_PATH, errcode);
    }
  else
    {
      if (invalue != 0)
        {
          dev->dir = DIR_DOWN;
        }
    }

  ret = ioctl(fd->fd_left, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read value from %s: %d\n",
              CONFIG_GAMES_SNAKE_LEFT_KEY_PATH, errcode);
    }
  else
    {
      if (invalue != 0)
        {
          dev->dir = DIR_LEFT;
        }
    }

  ret = ioctl(fd->fd_right, GPIOC_READ,
              (unsigned long)((uintptr_t)&invalue));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to read value from %s: %d\n",
              CONFIG_GAMES_SNAKE_RIGHT_KEY_PATH, errcode);
    }
  else
    {
      if (invalue != 0)
        {
          dev->dir = DIR_RIGHT;
        }
    }

  return OK;
}
