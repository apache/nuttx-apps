/****************************************************************************
 * apps/games/shift/shift_main.c
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
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

#ifdef CONFIG_GAMES_SHIFT_USE_CONSOLEKEY
#include "shift_input_console.h"
#endif

#ifdef CONFIG_GAMES_SHIFT_USE_DJOYSTICK
#include "shift_input_joystick.h"
#endif

#ifdef CONFIG_GAMES_SHIFT_USE_GESTURE
#include "shift_input_gesture.h"
#endif

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#define BOARDX_SIZE 6
#define BOARDY_SIZE 6

#define ROW         8
#define COL         8

/* Difficult mode:  4, 5, 6 */

#define GAMEMODE    6

#define NCOLORS     GAMEMODE

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct screen_state_s
{
  int fd_fb;
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
#ifdef CONFIG_FB_OVERLAY
  struct fb_overlayinfo_s oinfo;
#endif
  FAR void *fbmem;
};

struct game_screen_s
{
  int xres;     /* X display resolution */
  int yres;     /* Y display resolution */
  int xoff;     /* X offset to start drawing */
  int yoff;     /* Y offset to start drawing */
  int ncolors;  /* Supported number of colors */
  int blklen;   /* Size of side of the blocks */
  int steps;    /* Steps to slide a block for a now position */
  int stepinc;  /* Step increment during the sliding */
  int dir;      /* Direction to move the blocks */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_default_fbdev[] = "/dev/fb0";

/* The edge of the board is invisible to user */

int board[ROW][COL] =
{
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}
};

int prev_board[ROW][COL] =
{
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}
};

/* Colors used in the game plus Black */

static const uint16_t pallete[NCOLORS + 1] =
{
  RGB16_BLACK,
  RGB16_BLUE,
  RGB16_GREEN,
  RGB16_RED,
  RGB16_CYAN,
  RGB16_WHITE,
  RGB16_GOLD,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void draw_rect(FAR struct screen_state_s *state,
                      FAR struct fb_area_s *area, int color)
{
  FAR uint16_t *dest;
  FAR uint8_t *row;
  int x;
  int y;

  row = (FAR uint8_t *)state->fbmem + state->pinfo.stride * area->y;
  for (y = 0; y < area->h; y++)
    {
      dest = ((FAR uint16_t *)row) + area->x;
      for (x = 0; x < area->w; x++)
        {
          *dest++ = pallete[color];
        }

      row += state->pinfo.stride;
    }
}

void update_screen(FAR struct screen_state_s *state,
                   FAR struct fb_area_s *area)
{
  int ret;

#ifdef CONFIG_FB_UPDATE
  ret = ioctl(state->fd_fb, FBIO_UPDATE,
              (unsigned long)((uintptr_t)area));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIO_UPDATE) failed: %d\n",
              errcode);
    }
#endif
}

/* Draw the user board without the edges */

void draw_board(FAR struct screen_state_s *state,
                FAR struct fb_area_s *area,
                FAR struct game_screen_s *screen)
{
  int row;
  int col;
  int x;
  int y;
  int i;
  int color;

  /* Clear the framebuffer, but don't update yet */

  area->x = 0;
  area->y = 0;
  area->w = screen->xres;
  area->h = screen->yres;

  draw_rect(state, area, 0);

  for (i = screen->steps; i > 0; i--)
    {
      for (row = 1; row <= BOARDX_SIZE; row++)
        {
          for (col = 1; col <= BOARDY_SIZE; col++)
            {
              color = board[row][col];

              x = (col - 1) * screen->blklen + screen->xoff;
              y = (row - 1) * screen->blklen + screen->yoff;

              /* Get original width and height of blocks */

              area->w = screen->blklen;
              area->h = screen->blklen;

              /* Only do the sliding for blocks that changed */

              if (prev_board[row][col] != board[row][col])
                {
                  if (screen->dir == DIR_NONE)
                    {
                      area->x = x;
                      area->y = y;
                    }

                  if (screen->dir == DIR_UP)
                    {
                      area->x = x;
                      area->y = y + ((i - 1) * screen->stepinc);
                    }

                  if (screen->dir == DIR_DOWN)
                    {
                      area->x = x;
                      area->y = y - ((i - 1) * screen->stepinc);
                    }

                  if (screen->dir == DIR_LEFT)
                    {
                      area->x = x + ((i - 1) * screen->stepinc);
                      area->y = y;
                    }

                  if (screen->dir == DIR_RIGHT)
                    {
                      area->x = x - ((i - 1) * screen->stepinc);
                      area->y = y;
                    }
                }
              else
                {
                  area->x = x;
                  area->y = y;
                }

              /* Drawn only part of block if above margin */

              if (area->x > (screen->xres - 2 * screen->xoff))
                {
                  area->x = (screen->xres - 2 * screen->xoff) + 1;
                  area->w = 1;
                }

              if (area->x < screen->xoff)
                {
                  area->x = screen->xoff;
                }

              if (area->y > (screen->yres - 2 * screen->yoff))
                {
                  area->y = (screen->yres - 2 * screen->yoff) + 1;
                  area->h = 1;
                }

              if (area->y < screen->yoff)
                {
                  area->y = screen->yoff;
                }

              draw_rect(state, area, color);
            }
        }

      area->x = 0;
      area->y = 0;
      area->w = screen->xres;
      area->h = screen->yres;

      update_screen(state, area);
      usleep(100000);
    }
}

/****************************************************************************
 * Name: print_board
 *
 * Description:
 *   Draw the board including the user non-visible border for debugging.
 ****************************************************************************/

#ifdef DEBUG_SHIFT_GAME
void print_board(void)
{
  int row;
  int col;

  for (row = 0; row < ROW; row++)
    {
      printf("+---+---+---+---+---+---+---+---+\n");

      for (col = 0; col < COL; col++)
        {
          if (row == 0 || col == 0 || row == ROW - 1 || col == COL - 1)
            {
              printf("|>%c<", board[row][col] + 48);
            }
          else
            {
              printf("| %c ", board[row][col] + 48);
            }
        }

      printf("|\n");
    }

  printf("+---+---+---+---+---+---+---+---+\n\n\n");
}
#endif

/****************************************************************************
 * Name: move_board
 *
 * Description:
 *   Move the blocks (represented by numbers) to 'dir' direction (L,R,U,D).
 ****************************************************************************/

void move_board(int dir)
{
  int row;
  int row_ini;
  int col;
  int col_ini;
  int row_dir;
  int row_pos;
  int col_dir;
  int col_pos;
  int row_lim;
  int col_lim;

  if (dir == DIR_UP)
    {
      row_ini = 0;
      row_dir = 1;
      row_pos = 1;
      row_lim = BOARDY_SIZE + 1;

      col_ini = 0;
      col_dir = 1;
      col_pos = 0;
      col_lim = BOARDX_SIZE + 1;
    }

  if (dir == DIR_DOWN)
    {
      row_ini = BOARDY_SIZE + 1;
      row_dir = -1;
      row_pos = -1;
      row_lim = 0;

      col_ini = 0;
      col_dir = 1;
      col_pos = 0;
      col_lim = BOARDX_SIZE + 1;
    }

  if (dir == DIR_RIGHT)
    {
      row_ini = 0;
      row_dir = 1;
      row_pos = 0;
      row_lim = BOARDY_SIZE + 1;

      col_ini = BOARDX_SIZE + 1;
      col_dir = -1;
      col_pos = -1;
      col_lim = 0;
    }

  if (dir == DIR_LEFT)
    {
      row_ini = 0;
      row_dir = 1;
      row_pos = 0;
      row_lim = BOARDY_SIZE + 1;

      col_ini = 0;
      col_dir = 1;
      col_pos = 1;
      col_lim = BOARDX_SIZE + 1;
    }

  for (row = row_ini; row != row_lim; row += row_dir)
    {
      for (col = col_ini; col != col_lim; col += col_dir)
        {
          /* Never move item if we are in the cache border */

          if ((row != 0 && col != 0) &&
              (row != 0 && col != BOARDX_SIZE + 1) &&
              (row != BOARDY_SIZE + 1 && col != 0) &&
              (row != BOARDY_SIZE + 1 && col != BOARDX_SIZE + 1))
            {
              if (board[row][col] == 0)
                {
                  board[row][col] = board[row + row_pos][col + col_pos];
                  board[row + row_pos][col + col_pos] = 0;
                }
            }
        }
    }
}

/****************************************************************************
 * Name: fill_edge
 *
 * Description:
 *   Fill the invisible border with "colored" blocks
 ****************************************************************************/

void fill_edge(void)
{
  int i;

  for (i = 0; i <= BOARDX_SIZE + 1; i++)
    {
      board[0][i] = (random() % GAMEMODE) + 1;
      board[BOARDY_SIZE + 1][i] = (random() % GAMEMODE) + 1;
    }

  for (i = 0; i <= BOARDY_SIZE + 1; i++)
    {
      board[i][0] = (random() % GAMEMODE) + 1;
      board[i][BOARDY_SIZE + 1] = (random() % GAMEMODE) + 1;
    }
}

/****************************************************************************
 * Name: fill_border
 *
 * Description:
 *   Fill the visible border with "colored" blocks
 ****************************************************************************/

void fill_border(void)
{
  int i;

  for (i = 1; i < BOARDX_SIZE + 1; i++)
    {
      board[1][i] = (random() % GAMEMODE) + 1;
      board[BOARDY_SIZE][i] = (random() % GAMEMODE) + 1;
    }

  for (i = 1; i < BOARDY_SIZE + 1; i++)
    {
      board[i][1] = (random() % GAMEMODE) + 1;
      board[i][BOARDY_SIZE] = (random() % GAMEMODE) + 1;
    }
}

/****************************************************************************
 * Name: search_board_squares
 *
 * Description:
 *   Search for squares or triples of same colors.
 ****************************************************************************/

int search_board_squares(void)
{
  int row;
  int col;
  int found = 0;

  for (row = 1; row < BOARDY_SIZE; row++)
    {
      for (col = 1; col < BOARDX_SIZE; col++)
        {
          if (board[row][col] != 0)
            {
              if (fabs(board[row][col]) == fabs(board[row][col + 1]))
                {
                  /* Square */

                  if (fabs(board[row][col]) == fabs(board[row + 1][col]) &&
                      fabs(board[row][col + 1]) ==
                      fabs(board[row + 1][col + 1]))
                    {
                      found = 1;
                      board[row][col] = board[row][col] > 0 ?
                                        -board[row][col] : board[row][col];
                      board[row][col + 1] = board[row][col + 1] > 0 ?
                                            -board[row][col + 1] :
                                            board[row][col + 1];
                      board[row + 1][col] = board[row + 1][col] > 0 ?
                                            -board[row + 1][col] :
                                            board[row + 1][col];
                      board[row + 1][col + 1] = board[row + 1][col + 1] > 0 ?
                                                -board[row + 1][col + 1] :
                                                board[row + 1][col + 1];
                    }

                  /* |- */

                  if (fabs(board[row][col]) == fabs(board[row + 1][col]))
                    {
                      found = 1;
                      board[row][col] = board[row][col] > 0 ?
                                        -board[row][col] :
                                        board[row][col];
                      board[row][col + 1] = board[row][col + 1] > 0 ?
                                            -board[row][col + 1] :
                                            board[row][col + 1];
                      board[row + 1][col] = board[row + 1][col] > 0 ?
                                            -board[row + 1][col] :
                                            board[row + 1][col];
                    }

                  /* -| */

                  if (fabs(board[row][col + 1]) ==
                      fabs(board[row + 1][col + 1]))
                    {
                      found = 1;
                      board[row][col] = board[row][col] > 0 ?
                                        -board[row][col] : board[row][col];
                      board[row][col + 1] = board[row][col + 1] > 0 ?
                                            -board[row][col + 1] :
                                            board[row][col + 1];
                      board[row + 1][col + 1] = board[row + 1][col + 1] > 0 ?
                                                -board[row + 1][col + 1] :
                                                board[row + 1][col + 1];
                    }
                }

              if (fabs(board[row][col]) == fabs(board[row + 1][col]))
                {
                  /* |_ */

                  if (fabs(board[row + 1][col]) ==
                      fabs(board[row + 1][col + 1]))
                    {
                      found = 1;
                      board[row][col] = board[row][col] > 0 ?
                                        -board[row][col] : board[row][col];
                      board[row + 1][col] = board[row + 1][col] > 0 ?
                                            -board[row + 1][col] :
                                            board[row + 1][col];
                      board[row + 1][col + 1] = board[row + 1][col + 1] > 0 ?
                                                -board[row + 1][col + 1] :
                                                board[row + 1][col + 1];
                    }
                }
            }

          if (board[row + 1][col] != 0)
            {
              /* _|  */

              if (fabs(board[row + 1][col]) == fabs(board[row + 1][col + 1]))
                {
                  if (fabs(board[row + 1][col + 1]) ==
                      fabs(board[row][col + 1]))
                    {
                      found = 1;
                      board[row][col + 1] = board[row][col + 1] > 0 ?
                                            -board[row][col + 1] :
                                            board[row][col + 1];
                      board[row + 1][col] = board[row + 1][col] > 0 ?
                                            -board[row + 1][col] :
                                            board[row + 1][col];
                      board[row + 1][col + 1] = board[row + 1][col + 1] > 0 ?
                                                -board[row + 1][col + 1] :
                                                board[row + 1][col + 1];
                    }
                }
            }
        }
    }

  return found;
}

/****************************************************************************
 * Name: search_board_squares
 *
 * Description:
 *   Search for vertical/horizontal lines of same colours.
 ****************************************************************************/

int search_board_lines(void)
{
  int row;
  int col;
  int h = 0;
  int hpos = 0;
  int hfound = 0;
  int v = 0;
  int vpos = 0;
  int vfound = 0;
  int found = 0;
  int prev_sym = 0;

  /* Search horizontal lines */

  for (row = 1; row < BOARDY_SIZE + 1; row++)
    {
      prev_sym = board[row][1];

      for (col = 1; col < BOARDX_SIZE; col++)
        {
          if (board[row][col] != 0)
            {
              if (fabs(board[row][col]) == fabs(board[row][col + 1]))
                {
                  if (!hfound)
                    {
                      hpos = col;
                      hfound = 1;
                      prev_sym = board[row][col];
                    }

                  if (fabs(board[row][col]) == fabs(prev_sym))
                    {
                      h++;
                    }
                }
              else if (h < 2)
                {
                  h = 0;
                  hpos = 1;
                  hfound = 0;
                }
            }
        }

      if (h >= 2)
        {
          for (int i = hpos; i <= hpos + h; i++)
            {
              board[row][i] = board[row][i] > 0 ?
                              -board[row][i] : board[row][i];
              found = 1;
            }
        }

      h = 0;
      hpos = 1;
      hfound = 0;
    }

  /* Search vertical lines */

  for (col = 1; col < BOARDX_SIZE + 1; col++)
    {
      prev_sym = board[1][col];

      for (row = 1; row < BOARDY_SIZE; row++)
        {
          if (board[row][col] != 0)
            {
              if (fabs(board[row][col]) == fabs(board[row + 1][col]))
                {
                  if (!vfound)
                    {
                      vpos = row;
                      vfound = 1;
                      prev_sym = board[row][col];
                    }

                  if (fabs(board[row][col]) == fabs(prev_sym))
                    {
                      v++;
                    }
                }
              else if (v < 2)
                {
                  v = 0;
                  vpos = 1;
                  vfound = 0;
                }
            }
        }

      if (v >= 2)
        {
          for (int i = vpos; i <= vpos + v; i++)
            {
              board[i][col] = board[i][col] > 0 ?
                              -board[i][col] : board[i][col];
              found = 1;
            }
        }

      v = 0;
      vpos = 1;
      vfound = 0;
    }

  return found;
}

/****************************************************************************
 * Name: check_board
 *
 * Description:
 *   Verify if there are match lines/squares and remove them
 ****************************************************************************/

int check_board(void)
{
  int found = 0;
  int row;
  int col;

  found  = search_board_lines();
  found |= search_board_squares();

  /* Remove all negative values */

  for (row = 0; row < ROW; row++)
    {
      for (col = 0; col < COL; col++)
        {
          if (board[row][col] < 0)
            {
              board[row][col] = 0;
            }
        }
    }

  return found;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * shift_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *fbdev = g_default_fbdev;
  struct game_screen_s screen;
  struct input_state_s input;
  struct screen_state_s state;
  struct fb_area_s area;
  int score = 0;
  int ret;

  /* Open the framebuffer driver */

  state.fd_fb = open(fbdev, O_RDWR);
  if (state.fd_fb < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n", fbdev, errcode);
      return EXIT_FAILURE;
    }

  /* Get the characteristics of the framebuffer */

  ret = ioctl(state.fd_fb, FBIOGET_VIDEOINFO,
              (unsigned long)((uintptr_t)&state.vinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_VIDEOINFO) failed: %d\n",
              errcode);
      close(state.fd_fb);
      return EXIT_FAILURE;
    }

  printf("VideoInfo:\n");
  printf("      fmt: %u\n", state.vinfo.fmt);
  printf("     xres: %u\n", state.vinfo.xres);
  printf("     yres: %u\n", state.vinfo.yres);
  printf("  nplanes: %u\n", state.vinfo.nplanes);

  /* Setup the screen information */

  screen.xres = state.vinfo.xres > state.vinfo.yres ?
                state.vinfo.yres : state.vinfo.xres;
  screen.yres = state.vinfo.yres > state.vinfo.xres ?
                state.vinfo.xres : state.vinfo.yres;
  screen.xoff = (screen.xres % NCOLORS) / 2;
  screen.yoff = (screen.yres % NCOLORS) / 2;
  screen.steps = 2;
  screen.stepinc = 1;
  screen.blklen = (screen.xres / NCOLORS);
  screen.ncolors = NCOLORS;

  /* Get the display planeinfo */

  ret = ioctl(state.fd_fb, FBIOGET_PLANEINFO,
              (unsigned long)((uintptr_t)&state.pinfo));
  if (ret < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd_fb);
      return EXIT_FAILURE;
    }

  printf("PlaneInfo (plane 0):\n");
  printf("    fbmem: %p\n", state.pinfo.fbmem);
  printf("    fblen: %lu\n", (unsigned long)state.pinfo.fblen);
  printf("   stride: %u\n", state.pinfo.stride);
  printf("  display: %u\n", state.pinfo.display);
  printf("      bpp: %u\n", state.pinfo.bpp);

  state.fbmem = mmap(NULL, state.pinfo.fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, state.fd_fb, 0);
  if (state.fbmem == MAP_FAILED)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: ioctl(FBIOGET_PLANEINFO) failed: %d\n",
              errcode);
      close(state.fd_fb);
      return EXIT_FAILURE;
    }

  printf("Mapped FB: %p\n", state.fbmem);

  /* Fill the edge with random values */

  fill_edge();
  fill_border();

  /* Draw the empty board to user */

  draw_board(&state, &area, &screen);

  dev_input_init(&input);

  while (1)
    {
      ret = dev_read_input(&input);

      screen.dir = input.dir;

#ifdef DEBUG_SHIFT_GAME
      printf("Before moving:\n");
      print_board();
      usleep(2000000);
#endif

      /* Save a copy of the board before moving to new position */

      memcpy(prev_board, board, (sizeof(int) * ROW * COL));

      move_board(screen.dir);

      draw_board(&state, &area, &screen);

#ifdef DEBUG_SHIFT_GAME
      printf("After moving:\n");
      print_board();
      usleep(1000000);
#endif

      /* Save a copy of the board after moving the pieces */

      memcpy(prev_board, board, (sizeof(int) * ROW * COL));

      screen.dir = DIR_NONE;

      /* If we found bricks with same colors */

      if (check_board() == 1)
        {
          int i;

          /* Lets do a blinking effect */

          for (i = 0; i < 3; i++)
            {
              /* Draw the board with the pieces */

              memcpy(board, prev_board, (sizeof(int) * ROW * COL));

              draw_board(&state, &area, &screen);
              usleep(100000);

              /* Draw the board without the pieces */

              check_board();
              draw_board(&state, &area, &screen);
              usleep(100000);
            }

          usleep(500000);

          score += 100;
        }

#ifdef DEBUG_SHIFT_GAME
      printf("After checking:\n");
      print_board();
      usleep(1000000);
#endif

      usleep(1000000);

      /* Add random pieces in the not visible border */

      fill_edge();
    }

  return 0;
}
