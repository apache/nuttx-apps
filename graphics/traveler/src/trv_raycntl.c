/****************************************************************************
 * apps/graphics/traveler/src/trv_raycntl.c
 *  This file contains the high-level ray caster control logic
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_debug.h"
#include "trv_world.h"
#include "trv_plane.h"
#include "trv_bitmaps.h"
#include "trv_paltable.h"
#include "trv_trigtbl.h"
#include "trv_graphics.h"
#include "trv_pov.h"
#include "trv_rayrend.h"
#include "trv_rayprune.h"
#include "trv_raycast.h"
#include "trv_raycntl.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These definitions simplify creation of the initial ray casting cell */

#define TOP_HEIGHT  (VGULP_SIZE/2)
#define BOT_HEIGHT  (VGULP_SIZE - TOP_HEIGHT + 1)

#define TOP_ROW     0
#define MID_ROW     (TOP_ROW + TOP_HEIGHT - 1)
#define BOT_ROW     (VGULP_SIZE-1)

#define LEFT_WIDTH  ((HGULP_SIZE+1)/2)
#define RIGHT_WIDTH (HGULP_SIZE - LEFT_WIDTH + 2)

#define LEFT_COL    0
#define MID_COL     (LEFT_COL + LEFT_WIDTH - 1)
#define RIGHT_COL   HGULP_SIZE

/* The following macro converts a g_yaw[] array index into a relative screen
 * yaw.  This is used by the ray caster to perform view corrections.
 */

#define RELYAW(i)  (((i) + WINDOW_LEFT) - (WINDOW_WIDTH/2))

/* Macro to determine if two hits "hit" the same object */

#define SAME_CELL(i1,j1,i2,j2) \
  (g_ray_hit[i1][j1].rect == g_ray_hit[i2][j2].rect)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The following array describes the hits from X/Y/Z-ray casting for the
 * current HGULP_SIZE x VGULP_SIZE cell
 */

struct trv_raycast_s g_ray_hit[VGULP_SIZE][HGULP_SIZE + 1];

/* This array points to the screen buffer row corresponding to the
 * pitch angle
 */

FAR uint8_t *g_buffer_row[VGULP_SIZE];

/* The is the "column" offset in g_buffer_row for the current cell being
 * operated on.  This value is updated in a loop by trv_raycaster. */

int16_t g_cell_column;

/* This structure holds the parameters used in the current ray cast */

struct trv_camera_s g_camera;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* These are all of the yaw angles which will be used by the ray caster
 * on a given cycle
 */

static int16_t g_yaw[IMAGE_WIDTH + 1];

/* These are all of the pitch angles which will be used by the ray caster
 * on each horizontal pass of a given cycle
 */

static int16_t g_pitch[VGULP_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Function: trv_resolve_cell
 *
 * Description:
 *   If the hits in the corners of the input cell are not the same, then
 *   recursively resolve the cell until all four corners are the same.
 *   When the are the same, then rend the cell to the display buffer.
 *
 ****************************************************************************/

static void trv_resolve_cell(uint8_t toprow, uint8_t leftcol,
                             uint8_t height, uint8_t width)
{
  uint8_t midrow;
  uint8_t botrow;
  uint8_t midcol;
  uint8_t rightcol;
  uint8_t topheight;
  uint8_t botheight;
  uint8_t leftwidth;
  uint8_t rightwidth;

  /* Check if the cell has been reduced to a vertical line */

  if (width > 1)
    {
      /* No.. Check if the cell has been reduce to a horizontal line */

      if (height > 1)
        {
          /* No.. It is still a rectangular region.  Check if the top half hit
           * the same cell type
           */

          if (!SAME_CELL(toprow, leftcol, toprow, (leftcol + width - 1)))
            {
              /* No.. the top corners are different.  Compare the top left and
               * bottom left corners to decide how to divide this up
               */

              if (!SAME_CELL(toprow, leftcol, (toprow + height - 1), leftcol))
                {
                  /* The left corners are not the same.  Check the right
                   * corners.
                   */

                  if (!SAME_CELL(toprow, (leftcol + width - 1),
                                 (toprow + height - 1), (leftcol + width - 1)))
                    {
                      /* The right corners are not the same either.  Divide the
                       * cell into three cells, retaining the bottom half
                       * (whose corners might be the same).
                       */

                      leftwidth = ((width + 1) >> 1);
                      if (leftwidth == 1)
                        {
                          rightwidth = 1;
                          midcol = leftcol + 1;
                        }
                      else
                        {
                          /* The cell is greater than 2 columns in width */

                          rightwidth = width - leftwidth + 1;
                          midcol = leftcol + leftwidth - 1;

                          /* Get the top middle hit */

                          trv_raycast(g_pitch[toprow],
                                      g_yaw[g_cell_column + midcol],
                                      RELYAW(g_cell_column + midcol),
                                      &g_ray_hit[toprow][midcol]);
                        }

                      topheight = ((height + 1) >> 1);
                      if (topheight == 1)
                        {
                          botheight = 1;
                          midrow = toprow + 1;
                        }
                      else
                        {
                          /* The cell is greater that 2 rows in height */

                          botheight = height - topheight + 1;
                          midrow = toprow + topheight - 1;

                          /* Get the middle left hit */

                          trv_raycast(g_pitch[midrow],
                                      g_yaw[g_cell_column + leftcol],
                                      RELYAW(g_cell_column + leftcol),
                                      &g_ray_hit[midrow][leftcol]);

                          /* Get the center hit */

                          if (rightwidth > 1)
                            {
                              trv_raycast(g_pitch[midrow],
                                          g_yaw[g_cell_column + midcol],
                                          RELYAW(g_cell_column + midcol),
                                          &g_ray_hit[midrow][midcol]);
                            }

                          /* Get the middle right hit */

                          rightcol = leftcol + width - 1;
                          trv_raycast(g_pitch[midrow],
                                      g_yaw[g_cell_column + rightcol],
                                      RELYAW(g_cell_column + rightcol),
                                      &g_ray_hit[midrow][rightcol]);
                        }

                      trv_resolve_cell(toprow, leftcol, topheight, leftwidth);
                      trv_resolve_cell(toprow, midcol, topheight, rightwidth);
                      trv_resolve_cell(midrow, leftcol, botheight, width);
                    }

                  /* The left corners are not the same, but the right are.
                   * Divide the cell into three cells, retaining the right half
                   */

                  else
                    {
                      leftwidth = ((width + 1) >> 1);
                      if (leftwidth == 1)
                        {
                          rightwidth = 1;
                          midcol = leftcol + 1;
                        }
                      else
                        {
                          /* The cell is greater than 2 columns in width */

                          rightwidth = width - leftwidth + 1;
                          midcol = leftcol + leftwidth - 1;

                          /* Get the top middle hit */

                          trv_raycast(g_pitch[toprow],
                                      g_yaw[g_cell_column + midcol],
                                      RELYAW(g_cell_column + midcol),
                                      &g_ray_hit[toprow][midcol]);

                          /* Get the bottom middle hit */

                          botrow = toprow + height - 1;
                          trv_raycast(g_pitch[botrow],
                                      g_yaw[g_cell_column + midcol],
                                      RELYAW(g_cell_column + midcol),
                                      &g_ray_hit[botrow][midcol]);
                        }

                      topheight = ((height + 1) >> 1);
                      if (topheight == 1)
                        {
                          botheight = 1;
                          midrow = toprow + 1;
                        }
                      else
                        {
                          /* The cell is greater that 2 rows in height */

                          botheight = height - topheight + 1;
                          midrow = toprow + topheight - 1;

                          /* Get the middle left hit */

                          trv_raycast(g_pitch[midrow],
                                      g_yaw[g_cell_column + leftcol],
                                      RELYAW(g_cell_column + leftcol),
                                      &g_ray_hit[midrow][leftcol]);

                          /* Get the center hit */

                          if (rightwidth > 1)
                            {
                              trv_raycast(g_pitch[midrow],
                                          g_yaw[g_cell_column + midcol],
                                          RELYAW(g_cell_column + midcol),
                                          &g_ray_hit[midrow][midcol]);
                            }
                        }

                      trv_resolve_cell(toprow, leftcol, topheight, leftwidth);
                      trv_resolve_cell(midrow, leftcol, botheight, leftwidth);
                      trv_resolve_cell(toprow, midcol, height, rightwidth);
                    }
                }

              /* The left sides are the same! Divide the cell vertically into
               * two cells */

              else
                {
                  leftwidth = ((width + 1) >> 1);
                  if (leftwidth == 1)
                    {
                      rightwidth = 1;
                      midcol = leftcol + 1;
                    }
                  else
                    {
                      /* The cell is greater than 2 columns in width */

                      rightwidth = width - leftwidth + 1;
                      midcol = leftcol + leftwidth - 1;

                      /* Get the top middle hit */

                      trv_raycast(g_pitch[toprow], g_yaw[g_cell_column + midcol],
                                  RELYAW(g_cell_column + midcol),
                                  &g_ray_hit[toprow][midcol]);

                      /* Get the bottom middle hit */

                      botrow = toprow + height - 1;
                      trv_raycast(g_pitch[botrow], g_yaw[g_cell_column + midcol],
                                  RELYAW(g_cell_column + midcol),
                                  &g_ray_hit[botrow][midcol]);
                    }

                  trv_resolve_cell(toprow, leftcol, height, leftwidth);
                  trv_resolve_cell(toprow, midcol, height, rightwidth);
                }
            }

          /* The top corners are the same.  Compare the top left and bottom
           * left corners
           */

          else if (!SAME_CELL(toprow, leftcol, (toprow + height - 1), leftcol))
            {
              /* The top corners are the same, but left corners are not. Divide
               * the cell into two cells horizontally
               */

              topheight = ((height + 1) >> 1);
              if (topheight == 1)
                {
                  botheight = 1;
                  midrow = toprow + 1;
                }
              else
                {
                  /* The cell is greater that 2 rows in height */

                  botheight = height - topheight + 1;
                  midrow = toprow + topheight - 1;

                  /* Get the middle left hit */

                  trv_raycast(g_pitch[midrow], g_yaw[g_cell_column + leftcol],
                              RELYAW(g_cell_column + leftcol),
                              &g_ray_hit[midrow][leftcol]);

                  /* Get the middle right hit */

                  rightcol = leftcol + width - 1;
                  trv_raycast(g_pitch[midrow], g_yaw[g_cell_column + rightcol],
                              RELYAW(g_cell_column + rightcol),
                              &g_ray_hit[midrow][rightcol]);
                }

              trv_resolve_cell(toprow, leftcol, topheight, width);
              trv_resolve_cell(midrow, leftcol, botheight, width);
            }

          /* The top and left corners are the same.  Check the lower right
           * corner
           */

          else if (!SAME_CELL(toprow, leftcol, (toprow + height - 1), (leftcol + width - 1)))
            {
              /* The lower right corner differs from all of the others.  Divide
               * the cell into three cells, retaining the left half
               */

              leftwidth = ((width + 1) >> 1);
              if (leftwidth == 1)
                {
                  rightwidth = 1;
                  midcol = leftcol + 1;
                }
              else
                {
                  /* The cell is greater than 2 columns in width */

                  rightwidth = width - leftwidth + 1;
                  midcol = leftcol + leftwidth - 1;

                  /* Get the top middle hit */

                  trv_raycast(g_pitch[toprow], g_yaw[g_cell_column + midcol],
                              RELYAW(g_cell_column + midcol),
                              &g_ray_hit[toprow][midcol]);

                  /* Get the bottom middle hit */

                  botrow = toprow + height - 1;
                  trv_raycast(g_pitch[botrow], g_yaw[g_cell_column + midcol],
                              RELYAW(g_cell_column + midcol),
                              &g_ray_hit[botrow][midcol]);
                }

              topheight = ((height + 1) >> 1);
              if (topheight == 1)
                {
                  botheight = 1;
                  midrow = toprow + 1;
                }
              else
                {
                  /* The cell is greater that 2 rows in height */

                  botheight = height - topheight + 1;
                  midrow = toprow + topheight - 1;

                  /* Get the middle right hit */

                  rightcol = leftcol + width - 1;
                  trv_raycast(g_pitch[midrow], g_yaw[g_cell_column + rightcol],
                              RELYAW(g_cell_column + rightcol),
                              &g_ray_hit[midrow][rightcol]);

                  /* Get the center hit */

                  if (rightwidth > 1)
                    {
                      trv_raycast(g_pitch[midrow], g_yaw[g_cell_column + midcol],
                                  RELYAW(g_cell_column + midcol),
                                  &g_ray_hit[midrow][midcol]);
                    }
                }

              trv_resolve_cell(toprow, leftcol, height, leftwidth);
              trv_resolve_cell(toprow, midcol, topheight, rightwidth);
              trv_resolve_cell(midrow, midcol, botheight, rightwidth);
            }

          /* The four corners are the same! */

          else
            {
              /* Apply texturing */

              trv_rend_cell(toprow, leftcol, height, width);
            }
        }

      /* The cell has been reduced to a horizontal line */

      else
        {
          /* Check if the endpoints of the horizontal line are the same */

          if (!SAME_CELL(toprow, leftcol, toprow, (leftcol + width - 1)))
            {
              /* No.. they are different.  Divide the line in half */

              leftwidth = ((width + 1) >> 1);
              if (leftwidth == 1)
                {
                  rightwidth = 1;
                  midcol = leftcol + 1;
                }
              else
                {
                  /* The cell is greater than 2 columns in width */

                  rightwidth = width - leftwidth + 1;
                  midcol = leftcol + leftwidth - 1;

                  /* Get the middle hit */

                  trv_raycast(g_pitch[toprow], g_yaw[g_cell_column + midcol],
                              RELYAW(g_cell_column + midcol),
                              &g_ray_hit[toprow][midcol]);
                }

              trv_resolve_cell(toprow, leftcol, 1, leftwidth);
              trv_resolve_cell(toprow, midcol, 1, rightwidth);
            }

          /* The endpoints of the horizontal line are the same! */

          else
            {
              /* Apply texturing */

              trv_rend_row(toprow, leftcol, width);
            }
        }
    }

  /* The cell is only 1 pixel wide.  Check if it has been reduced to a single
   * pixel
   */

  else if (height > 1)
    {
      /* No.. The cell has been reduced to a vertical line.  Check if the
       * endpoints are the same.
       */

      if (!SAME_CELL(toprow, leftcol, (toprow + height - 1), leftcol))
        {
          /* No.. they are different.  Divide the line in half */

          topheight = ((height + 1) >> 1);
          if (topheight == 1)
            {
              botheight = 1;
              midrow = toprow + 1;
            }
          else
            {
              /* The cell is greater that 2 rows in height */

              botheight = height - topheight + 1;
              midrow = toprow + topheight - 1;

              /* Get the middle hit */

              trv_raycast(g_pitch[midrow], g_yaw[g_cell_column + leftcol],
                          RELYAW(g_cell_column + leftcol),
                          &g_ray_hit[midrow][leftcol]);
            }

          trv_resolve_cell(toprow, leftcol, topheight, 1);
          trv_resolve_cell(midrow, leftcol, botheight, 1);
        }

      /* The endpoints of the vertical line are the same! */

      else
        {
          /* Apply texturing */

          trv_rend_column(toprow, leftcol, height);
        }
    }

  /* The cell has been reduced to a single pixel. */

  else
    {
      /* Apply texturing */

      trv_rend_pixel(toprow, leftcol);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: trv_raycaster
 *
 * Description:
 *   This is the heart of the system.  it casts out 320 rays and builds the
 *   3-D image from their intersections with the walls.
 *
 ****************************************************************************/

void trv_raycaster(FAR struct trv_camera_s *player,
                   FAR struct trv_graphics_info_s *ginfo)
{
  int16_t row;         /* the current row being cast 0..IMAGE_HEIGHT */
  int16_t yaw;         /* Working yaw angle */
  int16_t pitch;       /* Working pitch angle */
  uint8_t *buffer;     /* Points the screen buffer row for this pitch */
  int i;

  trv_vdebug("\ntrv_raycaster: x=%d y=%d z=%d yaw=%d pitch=%d",
             player->x, player->x, player->z, player->yaw, player->pitch);

  /* Copy the input "player" to the working "camera */

  g_camera = *player;

  /* The horizontal field of view is determined by the width of the window
   * (centered at the yaw angle)
   */

  yaw = g_camera.yaw - (IMAGE_WIDTH / 2) + (HGULP_SIZE * VIDEO_COLUMN_ANGLE);
  if (yaw < 0)
    {
      yaw += ANGLE_360;
    }

  /* The vertical field of view is determined by the currently active screen
   * HEIGHT. NOTE: camera pitch cannot exceed 90 degrees. Not check is
   * performed here.
   */

  pitch = g_camera.pitch + (IMAGE_HEIGHT / 2);
  if (pitch >= ANGLE_360)
    {
      pitch -= ANGLE_360;
    }

  /* Loop through all columns at each yaw angle on the screen */

  for (g_cell_column = IMAGE_WIDTH; g_cell_column >= 0; g_cell_column--)
    {
      /* Save the yaw angle.  By saving all of the yaw angles, we can avoid
       * complex tests for 360 degree wraps.
       */

      g_yaw[g_cell_column] = yaw;

      /* Test if viewing yaw angle needs to wrap around */

      yaw += VIDEO_COLUMN_ANGLE;
      if (yaw >= ANGLE_360)
        {
          yaw -= ANGLE_360;
        }
    }

  /* Seed the algorithm PART I: Set up the raycaster this yaw range. */

  trv_ray_yawprune(g_yaw[IMAGE_WIDTH], g_yaw[0]);

  /* Top of Ray Casting Loops */

  /* Point to the first row of the rending buffer.  This will be bumped to
   * successive rows with each change in the pitch angle.
   */

  buffer = &ginfo->swbuffer[IMAGE_TOP];

  /* Loop through each row at each pitch angle */

  for (row = 0; (row < (IMAGE_HEIGHT - VGULP_SIZE + 1)); row += VGULP_SIZE)
    {
      /* Initialize the pitch angles that will be needed in the inner loop.
       * These are pre-calculated so that once we get started, we need not have
       * to be concerned about zero crossing conditions.
       */

      g_pitch[0] = pitch;
      g_buffer_row[0] = &buffer[IMAGE_LEFT];

      for (i = 1; i < VGULP_SIZE; i++)
        {
          g_pitch[i] = g_pitch[i - 1] - VIDEO_ROW_ANGLE;
          if (g_pitch[i] < ANGLE_0)
            {
              g_pitch[i] += ANGLE_360;
            }

          g_buffer_row[i] = g_buffer_row[i - 1] + TRV_SCREEN_WIDTH;
        }

      /* Seed the algorithm PART II: Set up the raycaster for this horizontal
       * swathe
       */

      trv_ray_pitchprune(g_pitch[VGULP_SIZE - 1], g_pitch[0]);

      /* Seed the algorithm PART III: These initial hits will be moved to the
       * beginning the hit array on the first pass through the loop.
       */

      trv_raycast(g_pitch[TOP_ROW], g_yaw[IMAGE_WIDTH],
                  RELYAW(IMAGE_WIDTH), &g_ray_hit[TOP_ROW][LEFT_COL]);
      trv_raycast(g_pitch[BOT_ROW], g_yaw[IMAGE_WIDTH],
                  RELYAW(IMAGE_WIDTH), &g_ray_hit[BOT_ROW][LEFT_COL]);

      /* Loop through all columns at each yaw angle on the screen window */

      for (g_cell_column = (IMAGE_WIDTH - HGULP_SIZE + 1);
           g_cell_column >= 0; g_cell_column -= HGULP_SIZE)
        {
          trv_vdebug("\ng_cell_column=%d yaw=%d", g_cell_column,
                     g_yaw[g_cell_column]);

          /* Perform Ray VGULP_SIZE x HGULP_SIZE Casting */

          /* The hits at the right corners will be the same as the hits for for
           * the left hand corners on the next pass */

          g_ray_hit[TOP_ROW][RIGHT_COL] = g_ray_hit[TOP_ROW][LEFT_COL];
          g_ray_hit[BOT_ROW][RIGHT_COL] = g_ray_hit[BOT_ROW][LEFT_COL];

          /* Now get new hits in the right corners. */

          trv_raycast(g_pitch[TOP_ROW], g_yaw[g_cell_column],
                      RELYAW(g_cell_column), &g_ray_hit[TOP_ROW][LEFT_COL]);
          trv_raycast(g_pitch[BOT_ROW], g_yaw[g_cell_column],
                      RELYAW(g_cell_column), &g_ray_hit[BOT_ROW][LEFT_COL]);

          /* Now, resolve the cell recursively until the hits are the same in
           * all four corners */

          trv_resolve_cell(TOP_ROW, LEFT_COL, VGULP_SIZE, (HGULP_SIZE + 1));
        }

      /* End of the pitch loop.  Bump up the pitch angle and the rending buffer
       * pointer for the next time through the outer loop */

      pitch -= (VGULP_SIZE * VIDEO_ROW_ANGLE);
      if (pitch < ANGLE_0)
        {
          pitch += ANGLE_360;
        }

      buffer += (VGULP_SIZE * TRV_SCREEN_WIDTH);

      /* Inform the ray cast engine that we are done with this horizonatal
       * swathe.
       */

      trv_ray_pitchunprune();
    }

  /* Inform the ray cast engine that we are done. */

  trv_ray_yawunprune();
}

/****************************************************************************
 * Function: trv_get_texture
 *
 * Description:
 *   This function returns the cell texture at the current cell row and
 *   column.  This function is used by the texture rending functions when
 *   processing a cell on a transparent wall and a transparent pixel is
 *   encountered.  This is a very inefficient way to handle these cases!
 *   Prevention of this condition is the best approach.  However, total
 *   elimination of the condition is impossible.
 *
 ****************************************************************************/

uint8_t trv_get_texture(uint8_t row, uint8_t col)
{
  FAR struct trv_raycast_s *ptr = &g_ray_hit[row][col];
  FAR uint8_t *palptr;
  int16_t zone;

  /* Perform a ray cast to get the hit at this row & column */

  trv_raycast(g_pitch[row], g_yaw[g_cell_column + col],
              RELYAW(g_cell_column + col), ptr);

  /* Check if we hit anything */

  if (ptr->rect)
    {
      /* Get a pointer to the palette map to use on this pixel */

      if (IS_SHADED(ptr->rect))
        {
          zone = GET_ZONE(ptr->xdist, ptr->ydist);
          palptr = GET_PALPTR(zone);
        }
      else
        {
          palptr = GET_PALPTR(0);
        }

      /* We did, return the pixel at this location */
      /* PROBLEM: Need to know if the is an even or odd hit */

      return palptr[GET_FRONT_PIXEL(ptr->rect, ptr->xpos, ptr->ypos)];
    }
  else
    {
      return INVISIBLE_PIXEL;
    }
}
