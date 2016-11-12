/****************************************************************************
 * apps/graphics/traveler/tools/tcledit/tcl_edit.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <tk.h>

#include "trv_types.h"
#include "wld_world.h"
#include "wld_debug.h"
#include "wld_utils.h"
#include "tcl_x11graphics.h"
#include "tcl_colors.h"

/****************************************************************************
 * Private Variables
 ***************************************************************************/

static Tcl_Interp *g_tcledit_interp;
static const char *g_in_filename;
static char *g_out_filename;
static char g_tcledit_filename[] = "tcl_edit.tk";
static char *g_tcledit_path = g_tcledit_filename;

/****************************************************************************
 * Public Variables
 ***************************************************************************/

enum edit_mode_e g_edit_mode = EDITMODE_NONE;
enum edit_plane_e g_edit_plane = EDITPLANE_X;
int g_view_size = WORLD_INFINITY;
int g_grid_step = WORLD_SIZE / 16;

int g_coord_offset[NUM_PLANES];
int g_plane_position[NUM_PLANES];
tcl_window_t g_windows[NUM_PLANES] =
{
  {
    .title = "X-Plane",
    .plane = EDITPLANE_X,
    .width = WINDOW_SIZE,
    .height = WINDOW_SIZE,
    .palette =
    {
      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      LightGray, DarkGray, DarkGray, DarkGray,
      LightSteelBlue, SlateGray, SlateGray, SteelBlue,
    },
  },
  {
   .title = "Y-Plane",
   .plane = EDITPLANE_Y,
   .width = WINDOW_SIZE,
   .height = WINDOW_SIZE,
   .palette =
    {
      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      LightGray, DarkGray, DarkGray, DarkGray,
      LightSteelBlue, SlateGray, SlateGray, SteelBlue,
    },
  },
  {
   .title = "Z-Plane",
   .plane = EDITPLANE_Z,
   .width = WINDOW_SIZE,
   .height = WINDOW_SIZE,
   .palette =
    {
      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      Lavender, Red, Red, LemonChiffon,
      LightCyan, LightSkyBlue, DeepSkyBlue, LightGray,

      LightGray, DarkGray, DarkGray, DarkGray,
      LightSteelBlue, SlateGray, SlateGray, SteelBlue,
    },
  }
};

rect_data_t g_edit_rect;

/****************************************************************************
 * Private Functions
 ***************************************************************************/

/* Called when any change is made to the display while in POS mode */

static void tcledit_update_posmode_display(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      tcl_window_t *w = &g_windows[i];
      tcl_paint_background(w);
      tcl_paint_grid(w);
      tcl_paint_rectangles(w);
      tcl_paint_position(w);
      x11_update_screen(w);
    }
}

/* Called when any change is made to the display while in NEW mode */

static void tcledit_update_newmode_display(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      tcl_window_t *w = &g_windows[i];
      tcl_paint_background(w);
      tcl_paint_grid(w);
      tcl_paint_rectangles(w);
      x11_update_screen(w);
    }
}

/* Called in response to the "tcl_seteditmode" Tcl command to set the
 * current edit mode
 */

static int tcledit_setmode(ClientData clientData,
                           Tcl_Interp *interp, int argc, const char *argv[])
{
  info("Processing command: %s\n", argv[0]);

  if (argc != 3)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }
  else if (strcmp(argv[1], "POS") == 0)
    {
      info("Entering POS mode\n");
      g_edit_mode = EDITMODE_POS;
      tcledit_update_posmode_display();
    }

  else if (strcmp(argv[1], "NEW") == 0)
    {
      g_edit_mode = EDITMODE_NEW;
      memset(&g_edit_rect, 0, sizeof(rect_data_t));
      if (strcmp(argv[2], "x") == 0)
        {
          info("Entering NEWX mode\n");
          g_edit_plane = EDITPLANE_X;
          g_edit_rect.plane = g_plane_position[EDITPLANE_X];
          g_edit_rect.hstart = g_plane_position[EDITPLANE_Y];
          g_edit_rect.hend = g_edit_rect.hstart;
          g_edit_rect.vstart = g_plane_position[EDITPLANE_Z];
          g_edit_rect.vend = g_edit_rect.vstart;
        }
      else if (strcmp(argv[2], "y") == 0)
        {
          info("Entering NEWY mode\n");
          g_edit_plane = EDITPLANE_Y;
          g_edit_rect.plane = g_plane_position[EDITPLANE_Y];
          g_edit_rect.hstart = g_plane_position[EDITPLANE_X];
          g_edit_rect.hend = g_edit_rect.hstart;
          g_edit_rect.vstart = g_plane_position[EDITPLANE_Z];
          g_edit_rect.vend = g_edit_rect.vstart;
        }
      else if (strcmp(argv[2], "z") == 0)
        {
          info("Entering NEWZ mode\n");
          g_edit_plane = EDITPLANE_Z;
          g_edit_rect.plane = g_plane_position[EDITPLANE_Z];
          g_edit_rect.hstart = g_plane_position[EDITPLANE_X];
          g_edit_rect.hend = g_edit_rect.hstart;
          g_edit_rect.vstart = g_plane_position[EDITPLANE_Y];
          g_edit_rect.vend = g_edit_rect.vstart;
        }
      else
        {
          wld_fatal_error("%s: Unrecognized NEW plane: %s\n",
                          __FUNCTION__, argv[2]);
        }

      tcledit_update_newmode_display();
    }
  else
    {
      wld_fatal_error("%s: Unrecognized mode: %s\n", __FUNCTION__, argv[1]);
    }

  return TCL_OK;
}

/* Called in response to the "tcl_position" Tcl command */

static int tcledit_new_position(ClientData clientData,
                                Tcl_Interp *interp, int argc, const char *argv[])
{
  info("Processing command: %s\n", argv[0]);

  if (argc != 4)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  g_plane_position[0] = atoi(argv[1]);
  g_plane_position[1] = atoi(argv[2]);
  g_plane_position[2] = atoi(argv[3]);

  info("New plane positions: {%d,%d,%d}\n",
        g_plane_position[0], g_plane_position[1], g_plane_position[2]);

  tcledit_update_posmode_display();
  return TCL_OK;
}

/* Called in response to the "tcl_zoom" Tcl command */

static int tcledit_new_zoom(ClientData clientData,
                            Tcl_Interp *interp, int argc, const char *argv[])
{
  info("Processing command: %s\n", argv[0]);

  if (argc != 5)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Get the zoom settings */

  g_view_size = atoi(argv[1]);
  g_coord_offset[0] = atoi(argv[2]);
  g_coord_offset[1] = atoi(argv[3]);
  g_coord_offset[2] = atoi(argv[4]);

  /* The grid size will be determined by the scaling */

  if (g_view_size <= 256)
    {
      g_grid_step = 16;            /* 16 lines at 256 */
    }
  else if (g_view_size <= 512)
    {
      g_grid_step = 32;            /* 16 lines at 512 */
    }
  else if (g_view_size <= 1024)
    {
      g_grid_step = 64;            /* 16 lines at 1024 */
    }
  else if (g_view_size <= 2048)
    {
      g_grid_step = 128;           /* 16 lines at 2048 */
    }
  else if (g_view_size <= 4096)
    {
      g_grid_step = 256;           /* 16 lines at 4096 */
    }
  else if (g_view_size <= 8192)
    {
      g_grid_step = 512;           /* 16 lines at 8196 */
    }
  else if (g_view_size <= 16384)
    {
      g_grid_step = 1024;          /* 16 lines at 16384 */
    }
  else                          /* if (g_view_size <= 32768) */
    {
      g_grid_step = 2048;          /* 16 lines at 32768 */
    }

  info("New g_view_size, g_grid_step: %d, %d\n", g_view_size, g_grid_step);
  info("New coordinate offsets: {%d,%d,%d}\n",
        g_coord_offset[0], g_coord_offset[1], g_coord_offset[2]);

  if (g_edit_mode == EDITMODE_POS)
    {
      tcledit_update_posmode_display();
    }
  else
    {
      tcledit_update_newmode_display();
    }
  return TCL_OK;
}

/* Called in response to the "tcl_edit" Tcl command */

static int tcledit_new_edit(ClientData clientData,
                            Tcl_Interp *interp, int argc, const char *argv[])
{
  int start;
  int end;
  int extent;

  info("Processing command: %s\n", argv[0]);

  if (argc != 4)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Ignore the command if we are not in NEW mode */

  if (g_edit_mode == EDITMODE_NEW)
    {
      /* Get the new position information */

      start = atoi(argv[2]);
      extent = atoi(argv[3]);
      end = start + extent - 1;

      /* Which plane are we editting? */

      switch (g_edit_plane)
        {
        case EDITPLANE_X:
          if (strcmp(argv[1], "x") == 0)
            {
              info("New X plane position: %d\n", start);
              g_edit_rect.plane = start;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              info("New horizontal Y coordinates: {%d,%d}\n", start, end);
              g_edit_rect.hstart = start;
              g_edit_rect.hend = end;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              info("New vertical Z coordinates: {%d,%d}\n", start, end);
              g_edit_rect.vstart = start;
              g_edit_rect.vend = end;
            }
          else
            {
              wld_fatal_error("%s: Unrecognized EDITX plane: %s\n",
                              __FUNCTION__, argv[1]);
            }
          break;

        case EDITPLANE_Y:
          if (strcmp(argv[1], "x") == 0)
            {
              info("New horizontal X coordinates: {%d,%d}\n", start, end);
              g_edit_rect.hstart = start;
              g_edit_rect.hend = end;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              info("New Y plane position: %d\n", start);
              g_edit_rect.plane = start;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              info("New vertical Z coordinates: {%d,%d}\n", start, end);
              g_edit_rect.vstart = start;
              g_edit_rect.vend = end;
            }
          else
            {
              wld_fatal_error("%s: Unrecognized EDITY plane: %s\n",
                              __FUNCTION__, argv[1]);
            }
          break;

        case EDITPLANE_Z:
          if (strcmp(argv[1], "x") == 0)
            {
              info("New horizontal X coordinates: {%d,%d}\n", start, end);
              g_edit_rect.hstart = start;
              g_edit_rect.hend = end;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              info("New vertical Y coordinates: {%d,%d}\n", start, end);
              g_edit_rect.vstart = start;
              g_edit_rect.vend = end;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              info("New Z plane position: %d\n", start);
              g_edit_rect.plane = start;
            }
          else
            {
              wld_fatal_error("%s: Unrecognized EDITZ plane: %s\n",
                              __FUNCTION__, argv[1]);
            }
          break;

        default:
          break;
        }
      tcledit_update_newmode_display();
    }

  return TCL_OK;
}

/* Called in response to the "tcl_attributes" Tcl command */

static int tcledit_new_attributes(ClientData clientData,
                                  Tcl_Interp *interp, int argc, const char *argv[])
{
  const char *attributes;
  int tmp;

  info("Processing command: %s\n", argv[0]);

  if (argc != 4)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  attributes = argv[1];
  if (strlen(attributes) != 3)
    {
      wld_fatal_error("%s: Unexpected attribute string length: %s\n",
                      __FUNCTION__, argv[1]);
    }

  /* Ignore the command if we are not in NEW mode */

  if (g_edit_mode == EDITMODE_NEW)
    {
      if (attributes[0] == '1')
        {
          g_edit_rect.attribute |= SHADED_PLANE;
        }
      else
        {
          g_edit_rect.attribute &= ~SHADED_PLANE;
        }

      if (attributes[1] == '1')
        {
          g_edit_rect.attribute |= TRANSPARENT_PLANE;
        }
      else
        {
          g_edit_rect.attribute &= ~TRANSPARENT_PLANE;
        }

      if (attributes[2] == '1')
        {
          g_edit_rect.attribute |= DOOR_PLANE;
        }
      else
        {
          g_edit_rect.attribute &= ~DOOR_PLANE;
        }
      info("attributes: %s->%02x\n", attributes, g_edit_rect.attribute);

      tmp = atoi(argv[2]);
      if ((tmp >= 0) && (tmp < 256))
        {
          g_edit_rect.texture = tmp;
        }
      else
        {
          fprintf(stderr, "Texture index out of range: %d\n", tmp);
        }
      info("texture: %s->%d\n", argv[2], g_edit_rect.texture);

      tmp = atoi(argv[3]);
      if ((tmp >= 0) && (tmp <= MAXX_SCALING))
        {
          g_edit_rect.scale = tmp;
        }
      else
        {
          fprintf(stderr, "Scaling not supported: %d\n", tmp);
        }
      info("scale: %s->%d\n", argv[3], g_edit_rect.scale);
    }

  return TCL_OK;
}

/* Called in response to the "tcl_addrectangle" Tcl command */

static int tcledit_add_rectangle(ClientData clientData,
                                 Tcl_Interp *interp, int argc, const char *argv[])
{

  info("Processing command: %s\n", argv[0]);

  if (argc != 1)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Ignore the command if we are not in NEW mode */

  if (g_edit_mode == EDITMODE_NEW)
    {
      /* Get a new container for the rectangle information */

      rect_list_t *rect = wld_new_plane();

      /* Copy the rectangle data into the container */

      rect->d = g_edit_rect;

      /* Save the rectangle in the correct list */

      switch (g_edit_plane)
        {
        case EDITPLANE_X:
          wld_add_plane(rect, &g_xplane_list);
          break;

        case EDITPLANE_Y:
          wld_add_plane(rect, &g_yplane_list);
          break;

        case EDITPLANE_Z:
          wld_add_plane(rect, &g_zplane_list);
          break;
        }
    }

  return TCL_OK;
}

/* Called in response to the "tcl_save" Tcl command */

static int tcledit_save_rectangles(ClientData clientData,
                                   Tcl_Interp *interp, int argc, const char *argv[])
{

  info("Processing command: %s\n", argv[0]);

  if (argc != 1)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  wld_save_planes(g_out_filename);
  return TCL_OK;
}

static void show_usage(const char *progname)
{
  fprintf(stderr, "USAGE:\n\t%s [-D <directory>] [-o <outfilename>] <infilename>\n",
          progname);
  exit(1);
}

/*************************************************************************
 * Public Functions
 ************************************************************************/

int main(int argc, char **argv, char **envp)
{
  char *directory;
  int option;
  int len;
  int ret;

  /* Parse command line options */

  g_out_filename = NULL;

  while ((option = getopt(argc, argv, "D:o:")) != EOF)
    {
      switch (option)
        {
        case 'D':
          /* Save the current working directory */

          g_tcledit_path = (char *)malloc(PATH_MAX);
          getcwd(g_tcledit_path, PATH_MAX);

          len = strlen(g_tcledit_path);
          g_tcledit_path[len] = '/';
          g_tcledit_path[len+1] = '\0';
          len++;

          strcat(&g_tcledit_path[len], g_tcledit_filename);
          g_tcledit_path = (char *)realloc(g_tcledit_path, strlen(g_tcledit_path) + 1);

          /* Change to the new directory */

          directory = optarg;
          ret = chdir(directory);
          if (ret < 0)
            {
              int errcode = errno;
              fprintf(stderr, "ERROR: Failed to CD to %s: %s\n",
                      directory, strerror(errcode));
              show_usage(argv[0]);
            }
          break;

        case 'o':
          g_out_filename = optarg;
          break;

        default:
          fprintf(stderr, "Unrecognized option: %c\n", option);
          show_usage(argv[0]);
          break;
        }
    }

  /* We expect at least one argument after the options: The input file name. */

  if (optind > argc - 1)
    {
      fprintf(stderr, "Expected input file name\n");
      show_usage(argv[0]);
    }

  g_in_filename = argv[optind];

  /* The output file name defaults to the same as the input file name but
   * with the extension .pll.
   */

  if (g_out_filename == NULL)
    {
      char *ptr;

      g_out_filename = strdup(g_in_filename + 5);
      for (ptr = g_out_filename; *ptr != '.' && *ptr != '\0'; ptr++);
      sprintf(ptr, ".pll");
    }

  /* Read the world files now so that we can be certain that it is a valid
   * world file.
   */

  if (wld_create_world(g_in_filename) != PLANE_SUCCESS)
    {
      exit(1);
    }

  /* Tk_Main creates a Tcl interpreter and calls Tcl_AppInit() then begins
   * processing window events and interactive commands.
   */

  Tk_Main(1, argv, Tcl_AppInit);
  exit(0);
}

/* This function can be called at any time after initialization to
 * execute a user specified script.
 */

int do_tcl_action(const char *script)
{
  return Tcl_Eval(g_tcledit_interp, script);
}

/* Tcl_AppInit is called from Tcl_Main() after the Tcl interpreter has
 * been created and before the script file or interactive command loop
 * is entered.
 */

int Tcl_AppInit(Tcl_Interp *interp)
{
  int ret;
  int i;

  /* Save the interpreter for later */

  g_tcledit_interp = interp;

  /* Initialize the edit windows before starting the Tcl parser */

  for (i = 0; i < NUM_PLANES; i++)
    {
      x11_initialize_graphics(&g_windows[i]);
    }

  /* Tcl_Init() sets up the Tcl library factility */

  ret = Tcl_Init(interp);
  if (ret == TCL_ERROR)
    {
      return TCL_ERROR;
    }

  /* Define application-specific commands */

  Tcl_CreateCommand(g_tcledit_interp, "tcl_seteditmode", tcledit_setmode,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_position", tcledit_new_position,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_zoom", tcledit_new_zoom,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_edit", tcledit_new_edit,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_attributes", tcledit_new_attributes,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_addrectangle", tcledit_add_rectangle,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(g_tcledit_interp, "tcl_save", tcledit_save_rectangles,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  /* Initialize the Tcl parser */

  ret = Tcl_EvalFile(g_tcledit_interp, g_tcledit_path);
  if (ret != TCL_OK)
    {
      Tcl_Obj *result;
      char *ptr;

      fprintf(stderr, "Tcl_EvalFile failed (%d): ", ret);

      result = Tcl_GetObjResult(g_tcledit_interp);
      for (i = 0, ptr = result->bytes; i < result->length; i++, ptr++)
        {
          fputc(*ptr, stderr);
        }

      fputc('\n', stderr);
      exit(1);
    }

  return ret;
}

void wld_fatal_error(char *message, ...)
{
  va_list args;

  va_start(args, message);
  vfprintf(stderr, message, args);
  putc('\n', stderr);
  va_end(args);

  exit(1);
}
