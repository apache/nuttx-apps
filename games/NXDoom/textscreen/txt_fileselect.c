/****************************************************************************
 * apps/games/NXDoom/textscreen/txt_fileselect.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Routines for selecting files.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "doomkeys.h"

#include "txt_fileselect.h"
#include "txt_gui.h"
#include "txt_inputbox.h"
#include "txt_io.h"
#include "txt_main.h"
#include "txt_widget.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Linux version: invoke the Zenity command line program to pop up a
 * dialog box. This avoids adding Gtk+ as a compile dependency.
 */

#define ZENITY_BINARY "/usr/bin/zenity"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct txt_fileselect_s
{
  txt_widget_t widget;
  txt_inputbox_t *inputbox;
  int size;
  const char *prompt;
  const char **extensions;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void txt_file_select_size_calc(TXT_UNCAST_ARG(fileselect));
static void txt_file_select_drawer(TXT_UNCAST_ARG(fileselect));
static int txt_file_select_keypress(TXT_UNCAST_ARG(fileselect), int key);
static void txt_file_select_destructor(TXT_UNCAST_ARG(fileselect));
static void txt_file_select_mousepress(TXT_UNCAST_ARG(fileselect), int x,
                                       int y, int b);
static void txt_file_select_focused(TXT_UNCAST_ARG(fileselect), int focused);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Dummy value to select a directory. */

const char *TXT_DIRECTORY[] =
{
  "__directory__",
  NULL,
};

txt_widget_class_t txt_fileselect_class =
{
  txt_always_selectable,
  txt_file_select_size_calc,
  txt_file_select_drawer,
  txt_file_select_keypress,
  txt_file_select_destructor,
  txt_file_select_mousepress,
  NULL,
  txt_file_select_focused,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static char *exec_read_output(char **argv)
{
  char *result;
  int completed;
  int pid;
  int status;
  int result_len;
  int pipefd[2];

  if (pipe(pipefd) != 0)
    {
      return NULL;
    }

  pid = fork();

  if (pid == 0)
    {
      dup2(pipefd[1], fileno(stdout));
      execv(argv[0], argv);
      exit(-1);
    }

  fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

  /* Read program output into 'result' string.
   * Wait until the program has completed and (if it was successful)
   * a full line has been read.
   */

  result = NULL;
  result_len = 0;
  completed = 0;

  while (!completed ||
         (status == 0 && (result == NULL || strchr(result, '\n') == NULL)))
    {
      char buf[64];
      int bytes;

      if (!completed && waitpid(pid, &status, WNOHANG) != 0)
        {
          completed = 1;
        }

      bytes = read(pipefd[0], buf, sizeof(buf));

      if (bytes < 0)
        {
          if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
              status = -1;
              break;
            }
        }
      else
        {
          char *new_result = realloc(result, result_len + bytes + 1);
          if (new_result == NULL)
            {
              break;
            }

          result = new_result;
          memcpy(result + result_len, buf, bytes);
          result_len += bytes;
          result[result_len] = '\0';
        }

      usleep(100 * 1000);
      txt_sleep(1);
      txt_update_screen();
    }

  close(pipefd[0]);
  close(pipefd[1]);

  /* Must have a success exit code. */

  if (WEXITSTATUS(status) != 0)
    {
      free(result);
      result = NULL;
    }

  /* Strip off newline from the end. */

  if (result != NULL && result[result_len - 1] == '\n')
    {
      result[result_len - 1] = '\0';
    }

  return result;
}

static unsigned int num_extensions(const char **extensions)
{
  unsigned int result = 0;

  if (extensions != NULL)
    {
      for (result = 0; extensions[result] != NULL; ++result)
        ;
    }

  return result;
}

/* ExpandExtension
 * given an extension (like wad)
 * return a pointer to a string that is a case-insensitive
 * pattern representation (like [Ww][Aa][Dd])
 */

static char *expand_extension(const char *orig)
{
  int oldlen;
  int newlen;
  int i;
  char *c;
  char *newext = NULL;

  oldlen = strlen(orig);
  newlen = oldlen * 4; /* pathological case: 'w' => '[Ww]' */
  newext = malloc(newlen + 1);

  if (newext == NULL)
    {
      return NULL;
    }

  c = newext;

  for (i = 0; i < oldlen; ++i)
    {
      if (isalpha(orig[i]))
        {
          *c++ = '[';
          *c++ = tolower(orig[i]);
          *c++ = toupper(orig[i]);
          *c++ = ']';
        }
      else
        {
          *c++ = orig[i];
        }
    }

  *c = '\0';
  return newext;
}

static char *txt_select_file(const char *window_title,
                             const char **extensions)
{
  return NULL;
}

static void txt_file_select_size_calc(TXT_UNCAST_ARG(fileselect))
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);

  /* Calculate widget size, but override the width to always
   * be the configured size.
   */

  txt_calc_widget_size(fileselect->inputbox);
  fileselect->widget.w = fileselect->size;
  fileselect->widget.h = fileselect->inputbox->widget.h;
}

static void txt_file_select_drawer(TXT_UNCAST_ARG(fileselect))
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);

  /* Input box widget inherits all the properties of the
   * file selector.
   */

  fileselect->inputbox->widget.x = fileselect->widget.x + 2;
  fileselect->inputbox->widget.y = fileselect->widget.y;
  fileselect->inputbox->widget.w = fileselect->widget.w - 2;
  fileselect->inputbox->widget.h = fileselect->widget.h;

  /* Triple bar symbol gives a distinguishing look to the file selector. */

  txt_draw_code_page_string("\xf0 ");
  txt_bgcolour(TXT_COLOR_BLACK, 0);
  txt_draw_widget(fileselect->inputbox);
}

static void txt_file_select_destructor(TXT_UNCAST_ARG(fileselect))
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);

  txt_destroy_widget(fileselect->inputbox);
}

static int txt_file_select_keypress(TXT_UNCAST_ARG(fileselect), int key)
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);
  return txt_widget_key_press(fileselect->inputbox, key);
}

static void txt_file_select_mousepress(TXT_UNCAST_ARG(fileselect), int x,
                                       int y, int b)
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);
  txt_widget_mouse_press(fileselect->inputbox, x, y, b);
}

static void txt_file_select_focused(TXT_UNCAST_ARG(fileselect), int focused)
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);

  txt_set_widget_focus(fileselect->inputbox, focused);
}

/* If the (inner) inputbox widget is changed, emit a change to the
 * outer (fileselect) widget.
 */

static void input_box_changed(TXT_UNCAST_ARG(widget),
                              TXT_UNCAST_ARG(fileselect))
{
  TXT_CAST_ARG(txt_fileselect_t, fileselect);

  txt_emit_signal(&fileselect->widget, "changed");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

txt_fileselect_t *txt_new_file_selector(char **variable, int size,
                                        const char *prompt,
                                        const char **extensions)
{
  txt_fileselect_t *fileselect;

  fileselect = malloc(sizeof(txt_fileselect_t));
  txt_init_widget(fileselect, &txt_fileselect_class);
  fileselect->inputbox = txt_new_input_box(variable, 1024);
  fileselect->inputbox->widget.parent = &fileselect->widget;
  fileselect->size = size;
  fileselect->prompt = prompt;
  fileselect->extensions = extensions;

  txt_signal_connect(fileselect->inputbox, "changed", input_box_changed,
                     fileselect);

  return fileselect;
}
