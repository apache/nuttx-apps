/************************************************************************************
 * apps/system/termcurses/termcurses.c
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include "tcurses_priv.h"

/************************************************************************************
 * Private Data
 ************************************************************************************/

static struct termcurses_dev_s *g_termcurses_devices[] =
{
#ifdef CONFIG_SYSTEM_TERMCURSES_VT100
  &g_vt100_tcurs,
#endif
  NULL
};

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: termcurses_initterm
 *
 * Description:
 *    Allocate and initialize a termcurses_s context based on the provided
 *    term_type string.  If the string is NULL, defaults to "vt100".
 *
 ************************************************************************************/

int termcurses_initterm(FAR const char *term_type, int in_fd, int out_fd,
                        FAR struct termcurses_s **term)
{
  FAR struct termcurses_dev_s *pnext;
  int c;

  /* Set term_type if NULL provided */

  if (term_type == NULL)
    {
      /* Check the "TERM" env variable */

      term_type = getenv("TERM");
      if (term_type == NULL)
        {
          /* Default to vt100 as a last resort */

          term_type = (FAR const char *) "vt100";
        }
    }

  /* Find the selected term_type in the list of registered handlers */

  c = 0;
  pnext = g_termcurses_devices[c];
  while (pnext != NULL)
    {
      /* Test if this item matches the selected term_type */

      if (strstr(pnext->name, term_type) != NULL)
        {
          /* Allocate a new structure for this termcurses */

          if (*term == NULL)
            {
              /* Call the termcurses_dev init function */

              *term = pnext->ops->init(in_fd, out_fd);
            }

          return OK;
        }

      /* No match.  Check next item in linked list */

      pnext = g_termcurses_devices[++c];
    }

  /* Not found! */

  *term = NULL;
  return -ENOSYS;
}

/************************************************************************************
 * Name: termcurses_deinitterm
 *
 * Description:
 *    Free all space for the termcurses terminal and perform any specific
 *    de-initialization tasks.
 *
 ************************************************************************************/

int termcurses_deinitterm(FAR struct termcurses_s *term)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;
  int result = OK;

  /* Call the dev function */

  if (dev->ops->terminate)
    {
      result = dev->ops->terminate(term);
    }

  /* Free the memory if termination is successful. */

  if (result == OK)
    free(dev);

  return result;
}

/************************************************************************************
 * Name: termcurses_moveyx
 *
 * Description:
 *   Move to location yx (row,col) on terminal
 *
 ************************************************************************************/

int termcurses_moveyx(FAR struct termcurses_s *term, int row, int col)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->move)
    {
      return dev->ops->move(term, TCURS_MOVE_YX, col, row);
    }

  return -ENOSYS;
}

/************************************************************************************
 * Name: termcurses_setcolors
 *
 * Description:
 *   Configure output text to render in the specified fg/bg colors.
 *
 ************************************************************************************/

int termcurses_setcolors(FAR struct termcurses_s *term,
                         FAR struct termcurses_colors_s *colors)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->setcolors)
    {
      return dev->ops->setcolors(term, colors);
    }

  return -ENOSYS;
}

/************************************************************************************
 * Name: termcurses_setattribute
 *
 * Description:
 *   Configure output text to render in the specified fg/bg colors.
 *
 ************************************************************************************/

int termcurses_setattribute(FAR struct termcurses_s *term, unsigned long attrib)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->setattrib)
    {
      return dev->ops->setattrib(term, attrib);
    }

  return -ENOSYS;
}

/************************************************************************************
 * Name: termcurses_getwinsize
 *
 * Description:
 *   Get size of terminal screen in terms of character rows and cols.
 *
 ************************************************************************************/

int termcurses_getwinsize(FAR struct termcurses_s *term, FAR struct winsize *winsz)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->getwinsize)
    {
      return dev->ops->getwinsize(term, winsz);
    }

  return -ENOSYS;
}

/************************************************************************************
 * Name: termcurses_getkeycode
 *
 * Description:
 *   Get a translated key code from the terminal input.
 *
 ************************************************************************************/

int termcurses_getkeycode(FAR struct termcurses_s *term, FAR int *specialkey,
      int *keymodifiers)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->getkeycode)
    {
      return dev->ops->getkeycode(term, specialkey, keymodifiers);
    }

  return -1;
}

/************************************************************************************
 * Name: termcurses_checkkey
 *
 * Description:
 *   Check if there is a key waiting to be processed.
 *
 ************************************************************************************/

bool termcurses_checkkey(FAR struct termcurses_s *term)
{
  FAR struct termcurses_dev_s *dev = (FAR struct termcurses_dev_s *) term;

  /* Call the dev function */

  if (dev->ops->checkkey)
    {
      return dev->ops->checkkey(term);
    }

  return 0;
}
