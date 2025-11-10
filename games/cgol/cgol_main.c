/****************************************************************************
 * apps/games/cgol/cgol_main.c
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
 * Author's note:
 *
 * There are several approaches that can be taken to make a CGOL
 * implementation efficient for a resource-constrained device, such as the
 * ones that Nuttx (impressively) can run on.
 *
 * I can make each of the cells a single bit, 1 for alive and 0 for dead, and
 * pack a large game state into a single array of bit fields. I can take
 * advantage of the game's "sparse" nature and only store the living cells as
 * x, y coordinates.
 *
 * In terms of memory usage:
 * A decently sized map of 100 x 100 cells with a single bit per cell would
 * require a little over 1KB of memory. On the other hand, each living cell
 * could be represented with 2 bytes (x, y), which allows 625 living cells in
 * the same space (much less).
 *
 * As it turns out, people smarter than me have determined in the "still-Life
 * Conjecture" that a still life cannot have a density greater than 1/2. [1]
 * Although this doesn't tell me anything about the maximum living cells at
 * any moment in time, it would be fair to say that on _average_ a 100 x 100
 * map is upper-bounded by some 5000 living cells. That is a lot more than
 * the 625 which can be implemented using 1250 bytes of memory.
 *
 * On the other hand, we also want to balance CPU usage. Nobody should be
 * using this alongside important tasks, as it is just a curiosity/demo.
 * However, lower CPU usage allows us to render frames more quickly and get
 * a nicer video output.
 *
 * This implementation will restrict the size of the CGOL world to be less
 * than or equal to the size of the frame buffer (i.e. minimum resolution of
 * one pixel per cell). With this cap on the world size, it will likely
 * always be more memory efficient to use full world map with 1 bit per cell
 * than to represent living cells as (x, y) pairs.
 *
 * The implementation in [2] gives a fast method for computing CGOL with
 * bit-fields in `uint64_t` types. However, that won't work as efficiently
 * on machines with smaller word sizes. I want to use a bit-field that works
 * well on all machines.
 *
 * In order to keep calculations of next states fast, this program enforces
 * the map width to be a multiple of the machine word size.
 *
 * [1] Elkies, Noam D, “The still-Life density problem and its
 * generalizations,” arXiv.org, 1999. https://arxiv.org/abs/math/9905194
 * (accessed Nov. 10, 2025).
 *
 * [2] https://binary-banter.github.io/game-of-life/
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <nuttx/video/fb.h>
#include <nuttx/video/rgbcolors.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Foreground and background colours for all pixel sizes */

#define ARGB_FULL_ALPHA (0xff << 24)

#define BG32 (RGB24_BLACK | ARGB_FULL_ALPHA)
#define FG32 (RGB24_WHITE | ARGB_FULL_ALPHA)

#define BG24 RGB24_BLACK
#define FG24 RGB24_WHITE

#define BG16 RGB16_BLACK
#define FG16 RGB16_WHITE

#define BG8 RGB8_BLACK
#define FG8 RGB8_WHITE

/* Total cells in the map */

#define TOTAL_CELLS (CONFIG_GAMES_CGOL_MAPWIDTH * CONFIG_GAMES_CGOL_MAPHEIGHT)

/* The number of bits in one "natural" word on our machine */

#ifdef UINT_WIDTH
#define WORD_BITS UINT_WIDTH
#else
#define WORD_BITS ((uint8_t)(sizeof(unsigned int) * 8))
#endif

/* The number of words required to represent the map. This will always be a
 * multiple of the word size since that is a restriction we put on MAPWIDTH.
 */

#define WORD_COUNT (TOTAL_CELLS / WORD_BITS)

/* Map width in words */

#define MAPWIDTH_WORDS (CONFIG_GAMES_CGOL_MAPWIDTH / WORD_BITS)

/* Last bit of a word mask. Important for calculating next state */

#define LAST_BIT_MASK (1 << (WORD_BITS - 1))

/* Returns 1 if bit `n` is set, 0 if not. */

#define BIT_N_ISSET(word, n) (((word) & (1 << (n))) >> (n))

/* The map width must be a multiple of the word size in bits, otherwise
 * the calculation of next states becomes much slower (not good on
 * low-resource devices).
 *
 * We can only compile-time check this if the compiler includes a macro for
 * the size of an unsigned int. Otherwise, we have to perform a run-time
 * check.
 */

#ifdef UINT_WIDTH
#if CONFIG_GAMES_CGOL_MAPWIDTH % WORD_BITS != 0
#error "Please choose a map width which is a multiple of sizeof(unsigned int)"
#endif
#endif

/* Short-hand for accessing the render buffer */

#ifdef CONFIG_GAMES_CGOL_DBLBUF
#define render_buf(state) ((state)->rambuf)
#else
#define render_buf(state) ((state)->fb)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fb_state_s
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  unsigned int scale;
  int fd;
  void *fb; /* Real frame buffer */
#ifdef CONFIG_GAMES_CGOL_DBLBUF
  void *rambuf; /* RAM double buffer */
#endif
};

/* Function which renders a single cell at `x`, `y` */

typedef void (*cell_render_f)(const struct fb_state_s *, uint32_t x,
                              uint32_t y);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cgol_init
 *
 * Description:
 *   Initializes the CGOL game with some random living cells. These are
 *   chosen using `rand()` and are approximately uniformly distributed.
 *   Overlap can happen and is not specially handled.
 *
 * Parameters:
 *   map - The map to initialize with random living cells.
 *   density - The map is initialized with `(1 / density) * TOTAL_CELLS` live
 *             cells, not accounting for overlap.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_init(unsigned int *map, unsigned int density)
{
  unsigned int idx;
  uint8_t offset;

  /* Wipe the map to all dead initially */

  memset(map, 0, WORD_COUNT * sizeof(unsigned int));

  /* Approximately half the total cells should start alive. */

  for (unsigned int i = 0; i < TOTAL_CELLS / density; i++)
    {
      /* Choose a random word, and within that word, choose a random bit to
       * start as a live cell.
       */

      idx = rand() % WORD_COUNT;
      offset = rand() % WORD_BITS;
      map[idx] |= (1 << offset);
    }

  return;
}

/****************************************************************************
 * Name: cell_lives
 *
 * Description:
 *   Calculates if a cell lives or dies in the next generation according to
 *   the rules of CGOL.
 *
 *   Rules of cells in CGOL:
 *   - Any live cell with < 2 neighbour dies by underpopulation
 *   - Any live cell with 2-3 live neighbours lives
 *   - Any live cell with > 3 live neighbours dies by overpopulation
 *   - Any dead cell with exactly 3 live neighbours becomes alive
 *
 * Parameters:
 *   alive - True if the cell is alive, false if the cell is dead
 *   count - The cell's neighbour count
 *
 * Returned Value:
 *   Returns 1 if the cell lives, 0 if the cell doesn't.
 *
 ****************************************************************************/

static unsigned int cell_lives(unsigned int alive, uint8_t count)
{
  if (alive)
    {
      if (count < 2)
        {
          return 0;
        }
      else if (count > 3)
        {
          return 0;
        }
      else
        {
          return 1;
        }
    }

  /* Cell is dead */

  return count == 3 ? 1 : 0;
}

/****************************************************************************
 * Name: cgol_advance
 *
 * Description:
 *   Advances the state of the game to the next time step. The next state
 *   overwrites the current one in `map`.
 *
 *   NOTE: This function expects both maps to have the same dimensions.
 *
 * Parameters:
 *   map - A bit-field map representing all of the cells in the map
 *
 * Returned Value:
 *   The pointer of the most updated map.
 *
 ****************************************************************************/

static void cgol_advance(unsigned int *map)
{
  unsigned int *above;
  unsigned int *cur;
  unsigned int *below;
  unsigned int prev_idx;
  unsigned int next_idx;
  unsigned int zerobuf[MAPWIDTH_WORDS];
  uint8_t count;

  unsigned int buffer[2][MAPWIDTH_WORDS];
  unsigned int *readybuf = buffer[0];
  unsigned int *workbuf = buffer[1];
  unsigned int *temp;

  /* We start with `cur` as row 1 of the map. Any part of our row scanner
   * (which is three rows in height) that exceeds the map's y limits
   * is made to point to a row of dead cells. This will prevent us from
   * having to handle more special cases in the logic below.
   */

  memset(zerobuf, 0, sizeof(zerobuf));

  above = zerobuf;
  cur = map;
  below = map + MAPWIDTH_WORDS;

  /* For each row of the map (i.e. while the `cur` pointer hasn't looped back
   * to the start of the map):
   */

  do
    {
      /* Working buffer starts zeroed so we don't need to explicitly do bit
       * operations on dead cells.
       */

      memset(workbuf, 0, MAPWIDTH_WORDS * sizeof(unsigned int));

      /* Iterate through each word in the rows. */

      for (unsigned int i = 0; i < MAPWIDTH_WORDS; i++)
        {
          /* Get the previous and next indexes (considering wrap-around),
           * since they will be useful later
           */

          if (i == 0)
            {
              prev_idx = MAPWIDTH_WORDS - 1;
            }
          else
            {
              prev_idx = i - 1;
            }

          if (i + 1 == MAPWIDTH_WORDS)
            {
              next_idx = 0;
            }
          else
            {
              next_idx = i + 1;
            }

          /* Iterate through each cell (bit) in the word */

          for (uint8_t b = 0; b < WORD_BITS; b++)
            {
              count = 0;

              /* Count the three '1' bits above and below our cell in the
               * neighbourhood.
               */

              if (b == 0)
                {
                  /* Bit 0 also needs to check the last bit of the next word
                   * above, below and immediately left.
                   */

                  count += popcount(above[i] & 0x3);
                  count += popcount(below[i] & 0x3);
                  count += BIT_N_ISSET(above[prev_idx], WORD_BITS - 1);
                  count += BIT_N_ISSET(below[prev_idx], WORD_BITS - 1);
                  count += BIT_N_ISSET(cur[prev_idx], WORD_BITS - 1);
                }
              else
                {
                  /* For all bits that aren't bit 0, we need to check the
                   * left-hand side of the neighbourhood. We check top-left
                   * and bottom-left for free already with above and below
                   * words, so just check current left explicitly.
                   */

                  count += popcount(above[i] & (0x7 << (b - 1)));
                  count += popcount(below[i] & (0x7 << (b - 1)));
                  count += BIT_N_ISSET(cur[i], b - 1);
                }

              /* For all cases except the last bit, we can easily check the
               * right-most neighbours.
               */

              if (b == WORD_BITS - 1)
                {
                  /* We need to fetch the next words and check if their
                   * first bits are set.
                   */

                  count += BIT_N_ISSET(cur[next_idx], 0);
                  count += BIT_N_ISSET(above[next_idx], 0);
                  count += BIT_N_ISSET(below[next_idx], 0);
                }
              else
                {
                  /* We already checked top-right and bottom-right before,
                   * check the current right only.
                   */

                  count += BIT_N_ISSET(cur[i], b + 1);
                }

              /* Now, we should have the complete neighbourhood count for the
               * cell at word `i` and bit `b`.
               *
               * We always put the next state of neighbours into the last row
               * of the buffer. Then, we shift up the rows by one index. Once
               * a row has bubbled to the top of the buffer, it is used to
               * overwrite the corresponding row of the map.
               */

              DEBUGASSERT(count <= 8); /* Only 8 neighbours to this cell */
              workbuf[i] |= (cell_lives(cur[i] & (1 << b), count) << b);
            }
        }

      if (cur - map >= MAPWIDTH_WORDS)
        {
          /* We can copy the portion of the next state that is ready to the
           * `above` row. Once we hit the next iteration, the `above` row
           * isn't used for calculation ever again. Since it will no longer
           * have any impact on the following neighbourhoods, it's safe to
           * overwrite in the map.
           *
           * NOTE: We only does this when `cur` is row 1 onward, since
           * otherwise `readybuf` is uninitialized.
           *
           * NOTE: This is the clever trick that allows us to have only 2
           * rows of buffer for the next state calculation, regardless of
           * the map size. This is very advantageous for embedded contexts.
           * Row buffers are even better because each word in the buffer is
           * `WORD_BITS` cells, opposed to column buffering which would take
           * as many words as the map height (a factor of `WORD_BITS` worse
           * than row buffering).
           */

          memcpy(above, readybuf, MAPWIDTH_WORDS * sizeof(unsigned int));
        }

      /* Swap work buffer and temporary buffer */

      temp = workbuf;
      workbuf = readybuf;
      readybuf = temp;

      /* Now all the updating and whatnot is done for this row. Increment the
       * row pointers to the next rows and continue!
       */

      above = cur;
      cur = below;
      below += MAPWIDTH_WORDS;

      /* If we exceed the bottom of the map, the 'below' pointer gets to be
       * the zero buffer.
       */

      if (below >= map + WORD_COUNT)
        {
          below = zerobuf;
        }
    }

  /* Since we move up the pointers for `above`, `cur` and `below`, the `cur`
   * pointer will be moved to the `zerobuf` once we hit the bottom of the
   * map. This is the reason for this condition.
   */

  while (cur != zerobuf);

  /* We perform one last copy from the `readybuf` at this stage, since the
   * `above` row will never reach the bottom row during the regular loop
   * iterations.
   */

  memcpy(above, readybuf, MAPWIDTH_WORDS * sizeof(unsigned int));
}

/****************************************************************************
 * Name: cgol_render_update
 *
 * Description:
 *   Updates the frame buffer with the current render.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_update(const struct fb_state_s *state)
{
#ifdef CONFIG_FB_UPDATE
  struct fb_area_s *full_screen;

  /* Create an area with the dimensions of the full screen for updating the
   * frame buffer after render operations are complete.
   */

  full_screen.x = 0;
  full_screen.y = 0;
  full_screen.w = fb_state.vinfo.xres;
  full_screen.h = fb_state.vinfo.yres;
#endif

  /* If double buffering, copy the RAM buffer to the frame buffer */

#ifdef CONFIG_GAMES_CGOL_DBLBUF
  memcpy(state->fb, state->rambuf, state->pinfo.fblen);
#endif

  /* If the frame buffer on this device needs explicit updates, do that */

#ifdef CONFIG_FB_UPDATE
  err = ioctl(fb_state.fd, FBIO_UPDATE, (uintptr_t)&full_screen);
  if (err < 0)
    {
      fprintf(stderr, "Couldn't update screen: %d\n", errno);
    }
#endif
}

/****************************************************************************
 * Name: cgol_render_clear
 *
 * Description:
 *   Wipes the frame buffer clean to the background colour.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_clear(const struct fb_state_s *state)
{
  /*  TODO: we can do this more efficiently if we just blot out the pixels
   * used for live cells last frame.
   */

  switch (state->pinfo.bpp)
    {
    case 32:
      for (uint32_t y = 0; y < state->pinfo.yres_virtual; y++)
        {
          uint8_t *row = render_buf(state) + state->pinfo.stride * y;
          for (uint32_t x = 0; x < state->pinfo.xres_virtual; x++)
            {
              ((uint32_t *)(row))[x] = BG32;
            }
        }
      break;

    case 24:
      for (uint32_t y = 0; y < state->pinfo.yres_virtual; y++)
        {
          uint8_t *row = render_buf(state) + state->pinfo.stride * y;
          for (uint32_t x = 0; x < state->pinfo.xres_virtual; x++)
            {
              *row++ = RGB24BLUE(BG24);
              *row++ = RGB24GREEN(BG24);
              *row++ = RGB24RED(BG24);
            }
        }
      break;

    case 16:
      {
        for (uint32_t y = 0; y < state->pinfo.yres_virtual; y++)
          {
            uint8_t *row = render_buf(state) + state->pinfo.stride * y;
            for (uint32_t x = 0; x < state->pinfo.xres_virtual; x++)
              {
                ((uint16_t *)(row))[x] = BG16;
              }
          }
      }
      break;

    case 8:
      memset(render_buf(state), BG8, state->pinfo.fblen);
      break;
    }
}

/****************************************************************************
 * Name: cgol_render_cell8
 *
 * Description:
 *   Renders living cell with 8bpp.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_cell8(const struct fb_state_s *state, uint32_t x,
                              uint32_t y)
{
  uint8_t *row;

  /* Scale the (x, y) coordinates */

  x *= state->scale;
  y *= state->scale;

  /* Starting at the (x, y) pair, we draw `scale` cells in each direction */

  for (uint8_t yy = 0; yy < state->scale; yy++)
    {
      row = ((uint8_t *)render_buf(state)) + state->pinfo.stride * y;
      for (uint8_t xx = 0; xx < state->scale; xx++)
        {
          row[x + xx] = FG8;
        }

      y++;
    }
}

/****************************************************************************
 * Name: cgol_render_cell16
 *
 * Description:
 *   Renders living cell with 16bpp.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_cell16(const struct fb_state_s *state, uint32_t x,
                               uint32_t y)
{
  uint8_t *row;

  /* Scale the (x, y) coordinates */

  x *= state->scale;
  y *= state->scale;

  /* Starting at the (x, y) pair, we draw `scale` cells in each direction */

  for (uint8_t yy = 0; yy < state->scale; yy++)
    {
      row = ((uint8_t *)render_buf(state)) + state->pinfo.stride * y;
      for (uint8_t xx = 0; xx < state->scale; xx++)
        {
          ((uint16_t *)(row))[x + xx] = FG16;
        }

      y++;
    }
}

/****************************************************************************
 * Name: cgol_render_cell24
 *
 * Description:
 *   Renders living cell with 24bpp.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_cell24(const struct fb_state_s *state, uint32_t x,
                               uint32_t y)
{
  uint8_t *row;

  /* Scale the (x, y) coordinates */

  x *= state->scale;
  y *= state->scale;

  /* Starting at the (x, y) pair, we draw `scale` cells in each direction */

  for (uint8_t yy = 0; yy < state->scale; yy++)
    {
      row = ((uint8_t *)render_buf(state)) + state->pinfo.stride * y;
      for (uint8_t xx = 0; xx < state->scale; xx++)
        {
          row[x + xx + 0] = RGB24BLUE(BG24);
          row[x + xx + 1] = RGB24GREEN(BG24);
          row[x + xx + 2] = RGB24RED(BG24);
        }

      y++;
    }
}

/****************************************************************************
 * Name: cgol_render_cell32
 *
 * Description:
 *   Renders living cell with 32bpp.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_cell32(const struct fb_state_s *state, uint32_t x,
                               uint32_t y)
{
  uint8_t *row;

  /* Scale the (x, y) coordinates */

  x *= state->scale;
  y *= state->scale;

  /* Starting at the (x, y) pair, we draw `scale` cells in each direction */

  for (uint8_t yy = 0; yy < state->scale; yy++)
    {
      row = ((uint8_t *)render_buf(state)) + state->pinfo.stride * y;
      for (uint8_t xx = 0; xx < state->scale; xx++)
        {
          ((uint32_t *)(row))[x + xx] = FG32;
        }

      y++;
    }
}

/****************************************************************************
 * Name: cgol_render_alive
 *
 * Description:
 *   Renders living cells to the frame buffer.
 *
 * Parameters:
 *   state - The frame buffer state information to use for rendering
 *   map - The bit-field map where 1 represents living cells, 0 for dead
 *         cells
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void cgol_render_alive(const struct fb_state_s *state,
                              const unsigned int *map)
{
  cell_render_f render_cell = NULL;
  uint32_t x;
  uint32_t y;
  uint32_t bit_index;
  unsigned int word;
  const unsigned int *start;
  const unsigned int *end;

  switch (state->pinfo.bpp)
    {
    case 32:
      render_cell = cgol_render_cell32;
      break;
    case 24:
      render_cell = cgol_render_cell24;
      break;
    case 16:
      render_cell = cgol_render_cell16;
      break;
    case 8:
      render_cell = cgol_render_cell8;
      break;
    }

  DEBUGASSERT(render_cell != NULL);

  /* Render only the living cells. */

  start = map;
  end = &map[WORD_COUNT];

  for (; map < end; map++)
    {
      word = *map; /* Current word */

      /* Determine where we are in the map in terms of (x, y) coords.
       */

      bit_index = (map - start) * WORD_BITS;
      y = (bit_index) / CONFIG_GAMES_CGOL_MAPWIDTH;
      x = (bit_index) % CONFIG_GAMES_CGOL_MAPWIDTH;

      /* Stop trying to render cells if there are no more living cells in
       * this word.
       */

      while (word)
        {
          /* Get the index of the first bit in the word.
           *
           * NOTE: reusing the bit_index variable here since it's no longer
           * needed.
           */

          bit_index = ffs(word);
          DEBUGASSERT(bit_index > 0);

          /* Advance the word past the set bit so that next iteration we'll
           * be looking for a new bit.
           *
           * NOTE: if the bit index is the same as the word size in bits,
           * the C standard says that left-shift behaviour is undefined.
           * For this case, we just set the word to 0 manually.
           */

          if (bit_index == WORD_BITS)
            {
              word = 0;
            }
          else
            {
              word = word >> bit_index;
            }

          /* Calculate the x, y coordinates of the bit we found that was set.
           *
           * NOTE: We can safely perform `- 1` since there is no way we would
           * be here if there isn't a '1' bit in the word.
           *
           * NOTE: Since we're operating within a single word, there is no
           * way that the x index can ever exceed the end of the word, and
           * therefore no way that it can ever cross to another row.
           */

          x += (bit_index - 1);
          DEBUGASSERT(x < CONFIG_GAMES_CGOL_MAPWIDTH);

          /* Since we've rounded the map length to the nearest word, we
           * may have up to WORD_BITS extra bits at the end of our map.
           * These bits have invalid (x, y) coordinates. Since the end of
           * our map always lands on the end of a row, this will manifest
           * with a `y` coordinate that is greater than or equal to the
           * map height. In this case, we skip rendering the cell. We've
           * also reached the end of the map so it's safe to break out of
           * the rendering loop.
           */

          if (y >= CONFIG_GAMES_CGOL_MAPHEIGHT)
            {
              break;
            }

          /* Render the cell at x, y */

          render_cell(state, x, y);

          /* Since our bit-index increase to x (`x += (bit_index - 1)`)
           * doesn't account for the additional left-shift of the word, we
           * have to increase x by one here.
           */

          x++;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cgol_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int err;
  char *fbdev = CONFIG_GAMES_CGOL_FBDEV;
  struct fb_state_s fb_state;
  unsigned int map[WORD_COUNT];
  unsigned int yscale;
  unsigned int xscale;

  if (argc == 2)
    {
      fbdev = argv[1];
    }

  /* Access the frame buffer */

  fb_state.fd = open(fbdev, O_RDWR);
  if (fb_state.fd < 0)
    {
      fprintf(stderr, "Failed to open %s: %d\n", fbdev, errno);
      return EXIT_FAILURE;
    }

  /* Get information about the frame buffer for rendering */

  err = ioctl(fb_state.fd, FBIOGET_VIDEOINFO, (uintptr_t)&fb_state.vinfo);
  if (err < 0)
    {
      fprintf(stderr, "Couldn't get frame buffer video information: %d\n",
              errno);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }

  err = ioctl(fb_state.fd, FBIOGET_PLANEINFO, (uintptr_t)&fb_state.pinfo);
  if (err < 0)
    {
      fprintf(stderr, "Couldn't get frame buffer plane information: %d\n",
              errno);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }

  /* If the frame buffer resolution is too small to support our game at its
   * lowest resolution (one pixel per cell), we can't play :(
   */

  if (fb_state.vinfo.xres < CONFIG_GAMES_CGOL_MAPWIDTH ||
      fb_state.vinfo.yres < CONFIG_GAMES_CGOL_MAPHEIGHT)
    {
      fprintf(stderr,
              "Needed at least %u x %u px resolution, but got %u x %u",
              CONFIG_GAMES_CGOL_MAPWIDTH, CONFIG_GAMES_CGOL_MAPHEIGHT,
              fb_state.vinfo.xres, fb_state.vinfo.yres);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }

#ifndef UINT_WIDTH

  /* If we haven't performed a compile-time check to guarantee that the map
   * width is a multiple of the word size, we need to run-time check it.
   */

  if (CONFIG_GAMES_CGOL_MAPWIDTH % WORD_BITS != 0)
    {
      fprintf(
          stderr,
          "Map width of %u cells is not a multiple of %u bits (word size)\n",
          CONFIG_GAMES_CGOL_MAPWIDTH, WORD_BITS);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }
#endif

  /* Get access to the frame buffer memory */

  fb_state.fb = mmap(NULL, fb_state.pinfo.fblen, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_FILE, fb_state.fd, 0);

  if (fb_state.fb == MAP_FAILED)
    {
      fprintf(stderr, "Failed to map frame buffer memory: %d\n", errno);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }

#ifdef CONFIG_GAMES_CGOL_DBLBUF

  /* If double buffering, allocate the RAM buffer */

  fb_state.rambuf = malloc(fb_state.pinfo.fblen);
  if (fb_state.rambuf == NULL)
    {
      fprintf(stderr, "Couldn't allocate double buffer: %d\n", errno);
      close(fb_state.fd);
      return EXIT_FAILURE;
    }
#endif

  /* Determine the ratio of the map size to the frame buffer size. If we can,
   * use a scale factor greater than 1 so that the image is rendered larger.
   *
   * This is selected by picking the minimum of the scale options.
   */

  xscale = fb_state.pinfo.xres_virtual / CONFIG_GAMES_CGOL_MAPWIDTH;
  yscale = fb_state.pinfo.yres_virtual / CONFIG_GAMES_CGOL_MAPHEIGHT;
  fb_state.scale = xscale < yscale ? xscale : yscale;

  /* Now we can seed the game with some random starting cells */

  cgol_init(map, CONFIG_GAMES_CGOL_DENSITY);

  /* Initially clear the game backdrop */

  cgol_render_clear(&fb_state);

  /* Loop the game forever */

  for (; ; )
    {
      /* Render the freshly calculated cells */

      cgol_render_alive(&fb_state, map);

#if CONFIG_GAMES_CGOL_FRAMEDELAY > 0
      usleep(CONFIG_GAMES_CGOL_FRAMEDELAY);
#endif

      /* Update the render with the new image */

      cgol_render_update(&fb_state);

      /* Calculate next state of the cells */

      cgol_advance(map);

      /* Clear the render in preparation for new cells to be rendered */

      cgol_render_clear(&fb_state);
    }

#ifdef CONFIG_GAMES_CGOL_DBLBUF
  free(fb_state.rambuf);
#endif
  close(fb_state.fd);
  return EXIT_SUCCESS;
}
