/****************************************************************************
 * apps/games/snake/snake_main.c
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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include <nuttx/video/rgbcolors.h>

#include <nuttx/leds/ws2812.h>

#ifdef CONFIG_GAMES_SNAKE_USE_CONSOLEKEY
#include "snake_input_console.h"
#endif

#ifdef CONFIG_GAMES_SNAKE_USE_GPIO
#include "snake_input_gpio.h"
#endif

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_SNAKE_GAME
#  define DEBUG_SNAKE_GAME 1
#endif

#define BOARDX_SIZE CONFIG_GAMES_SNAKE_LED_MATRIX_ROWS
#define BOARDY_SIZE CONFIG_GAMES_SNAKE_LED_MATRIX_COLS

#define N_LEDS      BOARDX_SIZE * BOARDY_SIZE

/* ID numbers of game items */

#define BLANK_PLACE 0
#define SNAKE_TAIL  1
#define SNAKE_HEAD  2
#define FOOD        3
#define BLINK       4
#define HIGH_SCORE  5

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Game item struct to represent in board */

struct game_item
{
  int pos_x;
  int pos_y;
};

/* Snake item struct to represent in board */

struct snake_item
{
  struct game_item tail[BOARDX_SIZE * BOARDY_SIZE];
  int tail_len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_board_path[] =
  CONFIG_GAMES_SNAKE_LED_MATRIX_PATH;

struct snake_item snake =
{
  0
};

struct game_item food =
{
  0
};

/* Game board to show */

uint32_t board[BOARDX_SIZE][BOARDY_SIZE] =
{
  0
};

/* Reached highest score */

uint32_t high_score = 0;

/* Colors used in the game plus Black */

static const uint32_t palette[] =
{
  RGB24_BLACK,
  RGB24_BLUE,
  RGB24_CYAN,
  RGB24_RED,
  RGB24_WHITE,
  RGB24_GREENYELLOW,
};

/* Second block of score value to print (.e.g 1x) */

const uint64_t second_score_block[] =
{
  0x0007050505070000,
  0x0004040404040000,
  0x0007010704070000,
  0x0007040704070000,
  0x0004040705050000,
  0x0007040701070000,
  0x0007050701070000
};

/* First block of score value to print (.e.g x1) */

const uint64_t first_score_block[] =
{
  0x0070505050700000,
  0x0040404040400000,
  0x0070107040700000,
  0x0070407040700000,
  0x0040407050500000,
  0x0070407010700000,
  0x0070507010700000,
  0x0040404040700000,
  0x0070507050700000,
  0x0070407050700000
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dim_color
 *
 * Description:
 *   Dim led color to handle brightness.
 *
 * Parameters:
 *   val - RGB24 value of the led
 *   dim - Percentage of brightness
 *
 * Returned Value:
 *   Dimmed RGB24 value.
 *
 ****************************************************************************/

static uint32_t dim_color(uint32_t val, float dim)
{
  uint16_t  r = RGB24RED(val);
  uint16_t  g = RGB24GREEN(val);
  uint16_t  b = RGB24BLUE(val);

  float sat = dim;

  r *= sat;
  g *= sat;
  b *= sat;

  return RGBTO24(r, g, b);
}

/****************************************************************************
 * Name: draw_board
 *
 * Description:
 *   Draw the user board.
 *
 * Parameters:
 *   fd  - File descriptor for screen
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void draw_board(int fd)
{
  uint32_t buffer[N_LEDS] =
  {
    0
  };

  int result;
  uint32_t *bp = buffer;
  int x;
  int y;
  int rgb_val;
  int tmp;

  for (x = 0; x < BOARDX_SIZE; x++)
    {
      for (y = 0; y < BOARDY_SIZE; y++)
        {
          rgb_val = palette[board[x][y]];
          tmp = dim_color(rgb_val, 0.15);
          *bp++ = ws2812_gamma_correct(tmp);
        }
    }

  lseek(fd, 0, SEEK_SET);

  result = write(fd, buffer, 4 * N_LEDS);
  if (result != 4 * N_LEDS)
    {
      fprintf(stderr,
              "ws2812_main: write failed: %d  %d\n",
              result,
              errno);
    }
}

/****************************************************************************
 * Name: print_board
 *
 * Description:
 *   Draw the board for debugging.
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#ifdef DEBUG_SNAKE_GAME
static void print_board(void)
{
  const char board_icons[] =
    {
      ' ',
      'o',
      'O',
      'X'
  };

  int row;
  int col;
  int tmp;
  int tmp_idx;

  /* Clear screen */

  printf("\e[1;1H\e[2J");

  /* Print board */

  for (row = 0; row < BOARDY_SIZE; row++)
    {
      for (tmp = 0; tmp < BOARDX_SIZE; tmp++)
        {
          printf("+--");
        }

      printf("-+\n");
      printf("|");
      for (col = 0; col < BOARDX_SIZE; col++)
        {
          tmp_idx = board[row][col];
          printf(" %c ", board_icons[tmp_idx]);
        }

      printf("|\n");
    }

  for (tmp = 0; tmp < BOARDX_SIZE; tmp++)
    {
      printf("+--");
    }

  printf("-+\n\n\n");
}
#endif

/****************************************************************************
 * Name: move_snake
 *
 * Description:
 *   Move the snake to 'dir' direction (L,R,U,D).
 *
 * Parameters:
 *   dir - Direction to move
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void move_snake(int dir)
{
  int prev_x = snake.tail[0].pos_x;
  int prev_y = snake.tail[0].pos_y;
  int prev_2x;
  int prev_2y;
  int i;

  board[prev_x][prev_y] = SNAKE_TAIL;
  for (i = 1; i < snake.tail_len; i++)
    {
      prev_2x = snake.tail[i].pos_x;
      prev_2y = snake.tail[i].pos_y;
      snake.tail[i].pos_x = prev_x;
      snake.tail[i].pos_y = prev_y;
      prev_x = prev_2x;
      prev_y = prev_2y;
    }

  board[prev_x][prev_y] = 0;

  if (dir == DIR_LEFT)
    {
      snake.tail[0].pos_y--;
    }

  if (dir == DIR_RIGHT)
    {
      snake.tail[0].pos_y++;
    }

  if (dir == DIR_UP)
    {
      snake.tail[0].pos_x--;
    }

  if (dir == DIR_DOWN)
    {
      snake.tail[0].pos_x++;
    }

  /* Wrap around screen borders */

  if (snake.tail[0].pos_x >= BOARDX_SIZE)
    {
      snake.tail[0].pos_x = 0;
    }

  if (snake.tail[0].pos_x < 0)
    {
      snake.tail[0].pos_x = BOARDX_SIZE - 1;
    }

  if (snake.tail[0].pos_y >= BOARDY_SIZE)
    {
      snake.tail[0].pos_y = 0;
    }

  if (snake.tail[0].pos_y < 0)
    {
      snake.tail[0].pos_y = BOARDY_SIZE - 1;
    }

  board[snake.tail[0].pos_x][snake.tail[0].pos_y] = SNAKE_HEAD;
}

/****************************************************************************
 * Name: fill_board_with_score
 *
 * Description:
 *   Fills board with score value to print
 *
 * Parameters:
 *   val - Value that includes with tiles should light up
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void fill_board_with_score(uint64_t val)
{
  int i = 0;
  int j = 0;
  for (i = 0; i < BOARDX_SIZE; i++)
    {
      for (j = 0; j < BOARDX_SIZE; j++)
        {
          if (val % 2 == 0)
            {
              board[i][j] = BLANK_PLACE;
            }
          else
            {
              board[i][j] = FOOD;
            }

            val = val >> 1;
        }
    }
}

/****************************************************************************
 * Name: print_score
 *
 * Description:
 *   Prints score to the screen
 *
 * Parameters:
 *   score - Score that reached in game
 *   fd    - File descriptor for screen
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void print_score(int score, int fd)
{
  int second_idx = (int) score / 10;
  int first_idx = score % 10;
  uint64_t val = second_score_block[second_idx] |
    first_score_block[first_idx];
  fill_board_with_score(val);
  draw_board(fd);
}

/****************************************************************************
 * Name: fill_board
 *
 * Description:
 *   Fill board with given value
 *
 * Parameters:
 *   val - Value to fill the board
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void fill_board(int val)
{
  int i;
  int j;

  for (i = 0; i < BOARDX_SIZE; i++)
    {
      for (j = 0; j < BOARDY_SIZE; j++)
        {
          board[i][j] = val;
        }
    }
}

/****************************************************************************
 * Name: init_game
 *
 * Description:
 *   Initializes game to start properly
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void init_game(void)
{
  snake.tail_len = 1;
  snake.tail[0].pos_x = BOARDX_SIZE / 2;
  snake.tail[0].pos_y = BOARDY_SIZE / 2;
  food.pos_x = rand() % BOARDX_SIZE;
  food.pos_y = rand() % BOARDY_SIZE;
  fill_board(BLANK_PLACE);

  board[BOARDX_SIZE / 2][BOARDY_SIZE / 2] = SNAKE_HEAD;
}

/****************************************************************************
 * Name: check_food_snake_collision
 *
 * Description:
 *   Check if there is any collision between food or snake
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   True(1) if there is collision, False(0) if not.
 *
 ****************************************************************************/

static bool check_food_snake_collision(void)
{
  int i = 0;

  for (i = 0; i < snake.tail_len; i++)
    {
      if (snake.tail[i].pos_x == food.pos_x &&
          snake.tail[i].pos_y == food.pos_y)
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: check_collisions
 *
 * Description:
 *   Check if there is any collision between food or snake itself
 *
 * Parameters:
 *   None
 *
 * Returned Value:
 *   True(1) if there is collision itself, False(0) if not.
 *
 ****************************************************************************/

static int check_collisions(void)
{
  int i = 0;
  bool end_game = false;
  bool food_col = false;

  /* Collision with itself (game over) */

  for (i = 1; i < snake.tail_len; i++)
    {
      if (snake.tail[i].pos_x == snake.tail[0].pos_x &&
          snake.tail[i].pos_y == snake.tail[0].pos_y)
        {
          end_game = true;
        }
    }

  /* Collision with food (eating food) */

  food_col = check_food_snake_collision();
  if (food_col)
    {
      snake.tail_len++;

      while (food_col)
        {
          food.pos_x = rand() % BOARDX_SIZE;
          food.pos_y = rand() % BOARDY_SIZE;
          food_col = check_food_snake_collision();
        }
    }

  board[food.pos_x][food.pos_y] = FOOD;
  return end_game;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * snake_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  bool game_over;
  struct input_state_s input_st;
  int board_fd = -1;
  int last_dir = DIR_NONE;
  int ret = ERROR;
  int i;
  int idx;
  int board_x;
  int board_y;
  struct snake_item prev_snake =
  {
    0
  };

  srand(time(NULL));

  /* Open the output device driver */

  board_fd = open(g_board_path, O_RDWR);
  if (board_fd < 0)
    {
      int errcode = errno;
      fprintf(stderr, "ERROR: Failed to open %s: %d\n",
              g_board_path, errcode);
      return EXIT_FAILURE;
    }

  dev_input_init(&input_st);

  /* Draw the empty board to user */

restart:
  init_game();
  draw_board(board_fd);

  input_st.dir = DIR_NONE;
  game_over = false;

  while (!game_over)
    {
#ifdef DEBUG_SNAKE_GAME
      print_board();
#endif
      draw_board(board_fd);

      last_dir = input_st.dir;
      ret = ERROR;

      while (ret == ERROR)
        {
          ret = dev_read_input(&input_st);
        }

      /* Check if input creates collision */

      if ((last_dir == DIR_LEFT && input_st.dir == DIR_RIGHT) ||
          (last_dir == DIR_RIGHT && input_st.dir == DIR_LEFT) ||
          (last_dir == DIR_UP && input_st.dir == DIR_DOWN) ||
          (last_dir == DIR_DOWN && input_st.dir == DIR_UP))
        {
          input_st.dir = last_dir;
        }

      if (input_st.dir == DIR_NONE)
        {
          input_st.dir = last_dir;
        }

      memcpy(&prev_snake, &snake, sizeof(struct snake_item));

      move_snake(input_st.dir);

      game_over = check_collisions();

      if (game_over)
        {
          /* Lets do a blinking effect */

          for (i = 0; i < 3; i++)
            {
              /* Draw the board with the pieces */

              for (idx = 0; idx < prev_snake.tail_len; idx++)
                {
                  board_x = prev_snake.tail[idx].pos_x;
                  board_y = prev_snake.tail[idx].pos_y;
                  board[board_x][board_y] = BLINK;
                }

              draw_board(board_fd);
              usleep(100000);

              /* Draw the board without the pieces */

              board[prev_snake.tail[0].pos_x][prev_snake.tail[0].pos_y] =
                SNAKE_HEAD;
              for (idx = 1; idx < snake.tail_len; idx++)
                {
                  board_x = prev_snake.tail[idx].pos_x;
                  board_y = prev_snake.tail[idx].pos_y;
                  board[board_x][board_y] = SNAKE_TAIL;
                }

              draw_board(board_fd);
              usleep(100000);
            }

          usleep(500000);
        }

      usleep(85000);
    }

  printf("Game Over! Score: %d\n", snake.tail_len);
  if (high_score < snake.tail_len)
    {
      printf("Congrats! New High Score\n");
      high_score = snake.tail_len;
      for (i = 0; i < 3; i++)
        {
          fill_board(HIGH_SCORE);
          draw_board(board_fd);
          usleep(200000);

          fill_board(BLANK_PLACE);
          draw_board(board_fd);
          usleep(200000);
        }
    }

  print_score(snake.tail_len, board_fd);

  printf("Please press left key to exit or other keys to restart\n");
  usleep(2000000);

  input_st.dir = DIR_NONE;
  ret = ERROR;
  while (ret == ERROR && input_st.dir == DIR_NONE)
    {
      ret = dev_read_input(&input_st);
    }

  if (input_st.dir != DIR_LEFT)
    {
      goto restart;
    }

  return 0;
}
