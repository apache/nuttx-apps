/****************************************************************************
 * examples/ft80x/ft80x_main.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "graphics/ft80x.h"
#include "ft80x.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ft80x_exampleinfo_s
{
  FAR const char *name;
  ft80x_example_t func;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* GPU Primitive display examples.  Most primitives are used, but not many of
 * their various options.
 *
 *  FUNCTION               PRIMITIVE USED  DESCRIPTION
 *  ---------------------- --------------- ----------------------------------
 *  ft80x_prim_bitmaps     BITMAPS         Bitmap drawing primitive
 *  ft80x_prim_points      POINTS          Point drawing primitive
 *  ft80x_prim_lines       LINES           Line drawing primitive
 *  ft80x_prim_linestrip   LINE_STRIP      Line strip drawing primitive
 *  ft80x_prim_edgestrip_r EDGE_STRIP_R    Edge strip right side drawing
                                           primitive
 *  --None--               EDGE_STRIP_L    Edge strip left side drawing
                                           primitive
 *  --None--               EDGE_STRIP_A    Edge strip above side drawing
                                           primitive
 *  --None--               EDGE_STRIP_B    Edge strip below side drawing
                                           primitive
 *  ft80x_prim_rectangles  RECTS           Rectangle drawing primitive
 *  ft80x_prim_scissor     SCISSOR         Scissor primitive
 *  ft80x_prim_alphablend  COLOR_A         Additive blend
 */

static const struct ft80x_exampleinfo_s g_primitives[] =
{
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
  { "Bitmaps",      ft80x_prim_bitmaps },
#endif
  { "Points",       ft80x_prim_points },
  { "Lines",        ft80x_prim_lines },
  { "Line Strip",   ft80x_prim_linestrip },
  { "Edge Strip R", ft80x_prim_edgestrip_r },
  { "Rectangles",   ft80x_prim_rectangles },
  { "Scissor",      ft80x_prim_scissor },
  { "Alpha Blend",  ft80x_prim_alphablend }
};

#define NPRIMITIVES (sizeof(g_primitives) / sizeof(ft80x_example_t))

/* Co-processor display examples.  Only a small, but interesting, subset
 * here co-processor command are exercised and these with only a few of the
 * possible options.
 *
 *  FUNCTION                 CoProc CMD USED DESCRIPTION
 *  ------------------------ --------------- ----------------------------------
 *  ft80x_coproc_button      CMD_BUTTON      Draw a button
 *  --None--                 CMD_CLOCK       Draw an analog clock
 *  --None--                 CMD_GAUGE       Draw a gauge
 *  --None--                 CMD_KEYS        Draw a row of keys
 *  ft80x_coproc_progressbar CMD_PROGRESS    Draw a progress bar
 *  ft80x_coproc_scrollbar   CMD_SCROLLBAR   Draw a scroll bar
 *  ft80x_coproc_slider      CMD_SLIDER      Draw a slider
 *  ft80x_coproc_dial        CMD_DIAL        Draw a rotary dial control
 *  ft80x_coproc_toggle      CMD_TOGGLE      Draw a toggle switch
 *  ft80x_coproc_number      CMD_NUMBER      Draw a decimal number
 *  ft80x_coproc_calibrate   CMD_CALIBRATE   Execute the touch screen
                                             calibration routine
 *  --None--                 CMD_SPINNER     Start an animated spinner
 *  --None--                 CMD_SCREENSAVER Start an animated screensaver
 *  --None--                 CMD_SKETCH      Start a continuous sketch update
 *  --None--                 CMD_SNAPSHOT    Take a snapshot of the current
                                             screen
 *  ft80x_coproc_logo        CMD_LOGO        Play device log animation
 */

static const struct ft80x_exampleinfo_s g_coproc[] =
{
  { "Button",         ft80x_coproc_button },
  { "Progress Bar",   ft80x_coproc_progressbar },
  { "Scroll Bar",     ft80x_coproc_scrollbar },
  { "Slider",         ft80x_coproc_slider },
  { "Dial",           ft80x_coproc_dial },
  { "Toggle",         ft80x_coproc_toggle },
  { "Number",         ft80x_coproc_number },
  { "Calibrate",      ft80x_coproc_calibrate },
  { "Logo",           ft80x_coproc_logo }
};

#define NCOPROC (sizeof(g_primitives) / sizeof(ft80x_example_t))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_showname
 *
 * Description:
 *   Show the name of the test
 *
 ****************************************************************************/

static int ft80x_showname(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                          FAR const char *name)
{
  struct ft80x_cmd_text_s text;
  int ret;

  /* Create the display list */

  ret = ft80x_dl_start(fd, buffer, false);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_start failed: %d\n", ret);
      return ret;
    }

  /* Use the CMD_TEXT co-processor command */

  text.cmd     = FT80X_CMD_TEXT;
  text.x       = 80;
  text.y       = 60;
  text.font    = 31;
  text.options = FT80X_OPT_CENTER;

  ret = ft80x_dl_data(fd, buffer, &text, sizeof(struct ft80x_cmd_text_s));
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_data failed: %d\n", ret);
      return ret;
    }

  ret = ft80x_dl_string(fd, buffer, name);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_string failed: %d\n", ret);
      return ret;
    }

  /* And terminate the display list */

  ret = ft80x_dl_end(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_dl_end failed: %d\n", ret);
      return ret;
    }

  /* Wait bit so that the user can read the name */

  sleep(2);
  return OK;
}

/****************************************************************************
 * Name: ft80x_example
 *
 * Description:
 *   Execute one example
 *
 ****************************************************************************/

static int ft80x_example(int fd, FAR struct ft80x_dlbuffer_s *buffer,
                         FAR const struct ft80x_exampleinfo_s *example)
{
  int ret;

  ft80x_info("Example %s\n", example->name);

  /* Show the name of the example */

  ret = ft80x_showname(fd, buffer, example->name);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_showname failed: %d\n", ret);
      return ret;
    }

  /* Then executte the example */

  ret = example->func(fd, buffer);
  if (ret < 0)
    {
      ft80x_err("ERROR: \"%s\" example failed: %d\n", example->name, ret);
      return ret;
    }

  /* Wait a bit */

  sleep(4);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ft80x_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int ft80x_main(int argc, char *argv[])
#endif
{
  FAR struct ft80x_dlbuffer_s *buffer;
  int fd;
  int i;

  /* Open the configured FT80x device */

  fd = open(CONFIG_EXAMPLES_FT80X_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: Failed to open %s: %d\n",
                CONFIG_EXAMPLES_FT80X_DEVPATH, errcode);
      UNUSED(errcode);
      return EXIT_FAILURE;
    }

  /* Allocate the display list buffer structure */

  buffer = (FAR struct ft80x_dlbuffer_s *)
    malloc(sizeof(struct ft80x_dlbuffer_s));

  if (buffer == NULL)
    {
      ft80x_err("ERROR: Failed to allocate display list buffer\n");
      close(fd);
      return EXIT_FAILURE;
    }

  /* Perform tests on a few of the FT80x primitive functions */

  ft80x_info("FT80x Primitive Functions\n");

  for (i = 0; i < NPRIMITIVES; i++)
    {
      (void)ft80x_example(fd, buffer, &g_primitives[i]);
    }

  /* Perform tests on a few of the FT80x Co-processor functions */

  ft80x_info("FT80x Co-processor Functions\n");

  for (i = 0; i < NCOPROC; i++)
    {
      (void)ft80x_example(fd, buffer, &g_coproc[i]);
    }

  free(buffer);
  close(fd);
  return EXIT_SUCCESS;
}
