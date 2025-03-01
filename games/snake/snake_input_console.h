/****************************************************************************
 * apps/games/snake/snake_input_console.h
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
#include <termios.h>

#include "snake_inputs.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Termios functions to have getch on Linux/NuttX */

static struct termios g_old;
static struct termios g_new;

/****************************************************************************
 * Name: init_termios
 *
 * Description:
 *   Initialize g_new terminal I/O settings.
 *
 * Parameters:
 *   echo - Enable echo flag
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void init_termios(int echo)
{
  tcgetattr(0, &g_old);                      /* grab old terminal i/o settings */
  g_new = g_old;                             /* use old settings as starting */
  g_new.c_lflag &= ~ICANON;                  /* disable buffered I/O */
  g_new.c_lflag &= ~ECHO;                    /* disable ECHO bit */
  g_new.c_lflag |= echo ? ECHO : 0;          /* set echo mode if requested */
  tcsetattr(0, TCSANOW, &g_new);             /* apply terminal I/O settings */
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);  /* set non-blocking mode */
}

/****************************************************************************
 * Name: reset_termios
 *
 * Description:
 *   Restore g_old terminal i/o settings.
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void reset_termios(void)
{
  tcsetattr(0, TCSANOW, &g_old);
}

/****************************************************************************
 * Name: getch_
 *
 * Description:
 *   Read 1 character.
 *
 * Parameters:
 *   echo - Enable echo mode flag
 *
 * Returned Value:
 *   Read character.
 *
 ****************************************************************************/

char getch_(int echo)
{
  char ch;

  init_termios(echo);
  ch = getchar();
  reset_termios();

  return ch;
}

/****************************************************************************
 * Name: getch
 *
 * Description:
 *   Read 1 character without echo.
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   Read character.
 *
 ****************************************************************************/

char getch(void)
{
  return getch_(0);
}

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
 *   Zero (OK)
 *
 ****************************************************************************/

int dev_input_init(FAR struct input_state_s *dev)
{
  init_termios(0);

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
  char ch;

  /* Arrows keys return three bytes: 27 91 [65-68] */

  if ((ch = getch()) == 27)
    {
      if ((ch = getch()) == 91)
        {
          ch = getch();
          if (ch == 65)
            {
              dev->dir = DIR_UP;
            }
          else if (ch == 66)
            {
              dev->dir = DIR_DOWN;
            }
          else if (ch == 67)
            {
              dev->dir = DIR_RIGHT;
            }
          else if (ch == 68)
            {
              dev->dir = DIR_LEFT;
            }
        }
      else
        {
          dev->dir = DIR_NONE;
        }
    }
  else
    {
      dev->dir = DIR_NONE;
    }

  return OK;
}
