/****************************************************************************
 * apps/games/shift/shift_inputs.h
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

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifndef DIR_NONE
#  define DIR_NONE    0
#endif

#ifndef DIR_LEFT
#  define DIR_LEFT    1
#endif

#ifndef DIR_RIGHT
#  define DIR_RIGHT   2
#endif

#ifndef DIR_UP
#  define DIR_UP      3
#endif

#ifndef DIR_DOWN
#  define DIR_DOWN    4
#endif

struct input_state_s
{
#ifdef CONFIG_GAMES_SHIFT_USE_CONSOLEKEY
  int fd_con;
#endif
#ifdef CONFIG_GAMES_SHIFT_USE_DJOYSTICK
  int fd_joy;
#endif
#ifdef CONFIG_GAMES_SHIFT_USE_GESTURE
  int fd_gest;
#endif
  int dir;      /* Direction to move the blocks */
};

