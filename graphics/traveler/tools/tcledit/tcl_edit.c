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
#include <string.h>
#include <unistd.h>
#include <tk.h>

#include "trv_types.h"
#include "debug.h"
#include "wld_plane.h"
#include "wld_utils.h"
#include "tcl_x11graphics.h"
#include "tcl_colors.h"

/****************************************************************************
 * Private Variables
 ***************************************************************************/

static Tcl_Interp *astInterp;
static const char *astInFileName;
static const char *astOutFileName;
static const char astDefaultFileName[] = "planes.pll";

/****************************************************************************
 * Public Variables
 ***************************************************************************/

enum editModeEnum editMode = EDITMODE_NONE;
enum editPlaneEnum editPlane = EDITPLANE_X;
int viewSize = WORLD_INFINITY;
int gridStep = WORLD_SIZE / 16;

int coordOffset[NUM_PLANES];
int planePosition[NUM_PLANES];
tcl_window_t windows[NUM_PLANES] =
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

rect_data_t editRect;

/****************************************************************************
 * Private Functions
 ***************************************************************************/

/* Called when any change is made to the display while in POS mode */

static void astUpdatePOSModeDisplay(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      tcl_window_t *w = &windows[i];
      tcl_paint_background(w);
      tcl_paint_grid(w);
      tcl_paint_rectangles(w);
      tcl_paint_position(w);
      tcl_update_screen(w);
    }
}

/* Called when any change is made to the display while in NEW mode */

static void astUpdateNEWModeDisplay(void)
{
  int i;

  for (i = 0; i < NUM_PLANES; i++)
    {
      tcl_window_t *w = &windows[i];
      tcl_paint_background(w);
      tcl_paint_grid(w);
      tcl_paint_rectangles(w);
      tcl_update_screen(w);
    }
}

/* Called in response to the "ast_seteditmode" Tcl command to set the
 * current edit mode
 */

static int astSetEditMode(ClientData clientData,
                          Tcl_Interp * interp, int argc, const char *argv[])
{
  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 3)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }
  else if (strcmp(argv[1], "POS") == 0)
    {
      ginfo("Entering POS mode\n");
      editMode = EDITMODE_POS;
      astUpdatePOSModeDisplay();
    }

  else if (strcmp(argv[1], "NEW") == 0)
    {
      editMode = EDITMODE_NEW;
      memset(&editRect, 0, sizeof(rect_data_t));
      if (strcmp(argv[2], "x") == 0)
        {
          ginfo("Entering NEWX mode\n");
          editPlane = EDITPLANE_X;
          editRect.plane = planePosition[EDITPLANE_X];
          editRect.hStart = planePosition[EDITPLANE_Y];
          editRect.hEnd = editRect.hStart;
          editRect.vStart = planePosition[EDITPLANE_Z];
          editRect.vEnd = editRect.vStart;
        }
      else if (strcmp(argv[2], "y") == 0)
        {
          ginfo("Entering NEWY mode\n");
          editPlane = EDITPLANE_Y;
          editRect.plane = planePosition[EDITPLANE_Y];
          editRect.hStart = planePosition[EDITPLANE_X];
          editRect.hEnd = editRect.hStart;
          editRect.vStart = planePosition[EDITPLANE_Z];
          editRect.vEnd = editRect.vStart;
        }
      else if (strcmp(argv[2], "z") == 0)
        {
          ginfo("Entering NEWZ mode\n");
          editPlane = EDITPLANE_Z;
          editRect.plane = planePosition[EDITPLANE_Z];
          editRect.hStart = planePosition[EDITPLANE_X];
          editRect.hEnd = editRect.hStart;
          editRect.vStart = planePosition[EDITPLANE_Y];
          editRect.vEnd = editRect.vStart;
        }
      else
        {
          wld_fatal_error("%s: Unrecognized NEW plane: %s\n",
                          __FUNCTION__, argv[2]);
        }
      astUpdateNEWModeDisplay();
    }
  else
    {
      wld_fatal_error("%s: Unrecognized mode: %s\n", __FUNCTION__, argv[1]);
    }

  return TCL_OK;
}

/* Called in response to the "ast_position" Tcl command */

static int astNewPosition(ClientData clientData,
                          Tcl_Interp * interp, int argc, const char *argv[])
{
  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 4)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  planePosition[0] = atoi(argv[1]);
  planePosition[1] = atoi(argv[2]);
  planePosition[2] = atoi(argv[3]);

  ginfo("New plane positions: {%d,%d,%d}\n",
        planePosition[0], planePosition[1], planePosition[2]);

  astUpdatePOSModeDisplay();
  return TCL_OK;
}

/* Called in response to the "ast_zoom" Tcl command */

static int astNewZoom(ClientData clientData,
                      Tcl_Interp * interp, int argc, const char *argv[])
{
  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 5)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Get the zoom settings */

  viewSize = atoi(argv[1]);
  coordOffset[0] = atoi(argv[2]);
  coordOffset[1] = atoi(argv[3]);
  coordOffset[2] = atoi(argv[4]);

  /* The grid size will be determined by the scaling */

  if (viewSize <= 256)
    {
      gridStep = 16;            /* 16 lines at 256 */
    }
  else if (viewSize <= 512)
    {
      gridStep = 32;            /* 16 lines at 512 */
    }
  else if (viewSize <= 1024)
    {
      gridStep = 64;            /* 16 lines at 1024 */
    }
  else if (viewSize <= 2048)
    {
      gridStep = 128;           /* 16 lines at 2048 */
    }
  else if (viewSize <= 4096)
    {
      gridStep = 256;           /* 16 lines at 4096 */
    }
  else if (viewSize <= 8192)
    {
      gridStep = 512;           /* 16 lines at 8196 */
    }
  else if (viewSize <= 16384)
    {
      gridStep = 1024;          /* 16 lines at 16384 */
    }
  else                          /* if (viewSize <= 32768) */
    {
      gridStep = 2048;          /* 16 lines at 32768 */
    }

  ginfo("New viewSize, gridStep: %d, %d\n", viewSize, gridStep);
  ginfo("New coordinate offsets: {%d,%d,%d}\n",
        coordOffset[0], coordOffset[1], coordOffset[2]);

  if (editMode == EDITMODE_POS)
    {
      astUpdatePOSModeDisplay();
    }
  else
    {
      astUpdateNEWModeDisplay();
    }
  return TCL_OK;
}

/* Called in response to the "ast_edit" Tcl command */

static int astNewEdit(ClientData clientData,
                      Tcl_Interp * interp, int argc, const char *argv[])
{
  int start;
  int end;
  int extent;

  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 4)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Ignore the command if we are not in NEW mode */

  if (editMode == EDITMODE_NEW)
    {
      /* Get the new position information */

      start = atoi(argv[2]);
      extent = atoi(argv[3]);
      end = start + extent - 1;

      /* Which plane are we editting? */

      switch (editPlane)
        {
        case EDITPLANE_X:
          if (strcmp(argv[1], "x") == 0)
            {
              ginfo("New X plane position: %d\n", start);
              editRect.plane = start;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              ginfo("New horizontal Y coordinates: {%d,%d}\n", start, end);
              editRect.hStart = start;
              editRect.hEnd = end;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              ginfo("New vertical Z coordinates: {%d,%d}\n", start, end);
              editRect.vStart = start;
              editRect.vEnd = end;
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
              ginfo("New horizontal X coordinates: {%d,%d}\n", start, end);
              editRect.hStart = start;
              editRect.hEnd = end;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              ginfo("New Y plane position: %d\n", start);
              editRect.plane = start;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              ginfo("New vertical Z coordinates: {%d,%d}\n", start, end);
              editRect.vStart = start;
              editRect.vEnd = end;
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
              ginfo("New horizontal X coordinates: {%d,%d}\n", start, end);
              editRect.hStart = start;
              editRect.hEnd = end;
            }
          else if (strcmp(argv[1], "y") == 0)
            {
              ginfo("New vertical Y coordinates: {%d,%d}\n", start, end);
              editRect.vStart = start;
              editRect.vEnd = end;
            }
          else if (strcmp(argv[1], "z") == 0)
            {
              ginfo("New Z plane position: %d\n", start);
              editRect.plane = start;
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
      astUpdateNEWModeDisplay();
    }
  return TCL_OK;
}

/* Called in response to the "ast_attribute" Tcl command */

static int astNewAttributes(ClientData clientData,
                            Tcl_Interp * interp, int argc, const char *argv[])
{
  const char *attributes;
  int tmp;

  ginfo("Processing command: %s\n", argv[0]);

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

  if (editMode == EDITMODE_NEW)
    {
      if (attributes[0] == '1')
        {
          editRect.attribute |= SHADED_PLANE;
        }
      else
        {
          editRect.attribute &= ~SHADED_PLANE;
        }

      if (attributes[1] == '1')
        {
          editRect.attribute |= TRANSPARENT_PLANE;
        }
      else
        {
          editRect.attribute &= ~TRANSPARENT_PLANE;
        }

      if (attributes[2] == '1')
        {
          editRect.attribute |= DOOR_PLANE;
        }
      else
        {
          editRect.attribute &= ~DOOR_PLANE;
        }
      ginfo("attributes: %s->%02x\n", attributes, editRect.attribute);

      tmp = atoi(argv[2]);
      if ((tmp >= 0) && (tmp < 256))
        {
          editRect.texture = tmp;
        }
      else
        {
          fprintf(stderr, "Texture index out of range: %d\n", tmp);
        }
      ginfo("texture: %s->%d\n", argv[2], editRect.texture);

      tmp = atoi(argv[3]);
      if ((tmp >= 0) && (tmp <= MAXX_SCALING))
        {
          editRect.scale = tmp;
        }
      else
        {
          fprintf(stderr, "Scaling not supported: %d\n", tmp);
        }
      ginfo("scale: %s->%d\n", argv[3], editRect.scale);
    }

  return TCL_OK;
}

/* Called in response to the "ast_addrectangle" Tcl command */

static int astAddRectangle(ClientData clientData,
                           Tcl_Interp * interp, int argc, const char *argv[])
{

  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 1)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  /* Ignore the command if we are not in NEW mode */

  if (editMode == EDITMODE_NEW)
    {
      /* Get a new container for the rectangle information */

      rect_list_t *rect = wld_new_plane();

      /* Copy the rectangle data into the container */

      rect->d = editRect;

      /* Save the rectangle in the correct list */

      switch (editPlane)
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

/* Called in response to the "ast_save" Tcl command */

static int astSaveRectangles(ClientData clientData,
                             Tcl_Interp * interp, int argc, const char *argv[])
{

  ginfo("Processing command: %s\n", argv[0]);

  if (argc != 1)
    {
      wld_fatal_error("%s: Unexpected number of arguments: %d\n",
                      __FUNCTION__, argc);
    }

  wld_save_planes(astOutFileName);
  return TCL_OK;
}

static void show_usage(const char *progname)
{
  fprintf(stderr, "USAGE:\n\t%s [-o <outfilename>] <infilename>\n", progname);
  exit(1);
}

/*************************************************************************
 * Public Functions
 ************************************************************************/

int main(int argc, char **argv, char **envp)
{
  int option;

  /* Parse command line options */

  astOutFileName = astDefaultFileName;

  while ((option = getopt(argc, argv, "o:")) != EOF)
    {
      switch (option)
        {
        case 'o':
          astOutFileName = optarg;
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

  astInFileName = argv[optind];

  /* Read the plane file now so that we can be certain that it is a valid
   * plaine file. */

  if (wld_load_planefile(astInFileName) != PLANE_SUCCESS)
    {
      exit(1);
    }

  /* Tk_Main creates a Tcl interpreter and calls Tcl_AppInit() then begins
   * processing window events and interactive commands. */

  Tk_Main(1, argv, Tcl_AppInit);
  exit(0);
}

/* This function can be called at any time after initialization to
 * execute a user specified script.
 */

int do_tcl_action(const char *script)
{
  return Tcl_Eval(astInterp, script);
}

/* Tcl_AppInit is called from Tcl_Main() after the Tcl interpreter has
 * been created and before the script file or interactive command loop
 * is entered.
 */

int Tcl_AppInit(Tcl_Interp * interp)
{
  int i;

  /* Save the interpreter for later */

  astInterp = interp;

  /* Initialize the edit windows before starting the Tcl parser */

  for (i = 0; i < NUM_PLANES; i++)
    {
      tcl_init_graphics(&windows[i]);
    }

  /* Tcl_Init() sets up the Tcl library factility */

  if (Tcl_Init(interp) == TCL_ERROR)
    {
      return TCL_ERROR;
    }

  if (Tk_Init(interp) == TCL_ERROR)
    {
      return TCL_ERROR;
    }

  /* Define application-specific commands */

  Tcl_CreateCommand(astInterp, "ast_seteditmode", astSetEditMode,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_position", astNewPosition,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_zoom", astNewZoom,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_edit", astNewEdit,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_attributes", astNewAttributes,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_addrectangle", astAddRectangle,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(astInterp, "ast_save", astSaveRectangles,
                    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

  /* Initialize the Tcl parser */

  if (Tcl_EvalFile(astInterp, "tcledit.tk") != TCL_OK)
    {
      fprintf(stderr, "%s\n", Tcl_GetVar(astInterp, "errorCode", 0));
      fprintf(stderr, "%s\n", Tcl_GetVar(astInterp, "errorInfo", 0));
      exit(1);
    }

  return TCL_OK;
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
