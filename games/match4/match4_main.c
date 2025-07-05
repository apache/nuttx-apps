/****************************************************************************
 * apps/games/match4/match4_main.c
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

#ifdef CONFIG_GAMES_MATCH4_USE_CONSOLEKEY
#include "match4_input_console.h"
#endif

#ifdef CONFIG_GAMES_MATCH4_USE_GPIO
#include "match4_input_gpio.h"
#endif

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

#ifdef CONFIG_MATCH4_GAME_DEBUG
#  define DEBUG_MATCH4_GAME 1
#endif

#define BOARDX_SIZE CONFIG_GAMES_MATCH4_LED_MATRIX_ROWS
#define BOARDY_SIZE CONFIG_GAMES_MATCH4_LED_MATRIX_COLS

#define N_LEDS      BOARDX_SIZE * BOARDY_SIZE

#define P1_COLOUR        RGB24_YELLOW
#define P1_SELECT_COLOUR RGB24_GOLD
#define P2_COLOUR        RGB24_BLUE
#define P2_SELECT_COLOUR RGB24_CYAN
#define BLUE_COLOR       RGB24_BLUE
#define RED_COLOR        RGB24_RED
#define WHITE_COLOR      RGB24_WHITE

/* ID numbers of game items */

#define BLANK_PLACE 0
#define P1_PREVIEW  1
#define P1_SELECT   2
#define P2_PREVIEW  3
#define P2_SELECT   4
#define BLUE_INDEX  5
#define RED_INDEX   6
#define WHITE_INDEX 7
#define PALETTE_MAX 8

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_board_path[] =
  CONFIG_GAMES_MATCH4_LED_MATRIX_PATH;

/* Game board to show */

uint32_t board[BOARDX_SIZE][BOARDY_SIZE] =
{
  0
};

/* Player colours */

uint32_t players[2] =
{
  P1_SELECT,
  P2_SELECT,
};

/* Player move preview colour */

uint32_t players_select[2] =
{
  P1_PREVIEW,
  P2_PREVIEW,
};

/* Colors used in the game */

static const uint32_t palette[] =
{
  RGB24_BLACK,
  P1_COLOUR,
  P1_SELECT_COLOUR,
  P2_COLOUR,
  P2_SELECT_COLOUR,
  BLUE_COLOR,
  RED_COLOR,
  WHITE_COLOR,
};

/* Game entrance screen that shows one player mode */

uint32_t one_player_scr[BOARDX_SIZE][BOARDY_SIZE] =
{
  {
    BLUE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX,
    BLUE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, BLUE_INDEX, WHITE_INDEX, BLUE_INDEX,
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, BLUE_INDEX, WHITE_INDEX,
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX,
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  }
};

/* Game entrance screen that shows two players mode */

uint32_t two_player_scr[BOARDX_SIZE][BOARDY_SIZE] =
{
  {
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, BLUE_INDEX,
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, BLUE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX,
    BLUE_INDEX, WHITE_INDEX, BLUE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX,
    WHITE_INDEX, BLUE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX,
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, WHITE_INDEX, WHITE_INDEX, WHITE_INDEX
  },
  {
    WHITE_INDEX, WHITE_INDEX, RED_INDEX, WHITE_INDEX,
    RED_INDEX, RED_INDEX, RED_INDEX, WHITE_INDEX
  }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef DEBUG_MATCH4_GAME
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

static void print_board(void)
{
  const char board_icons[] =
    {
      ' ', 'x', 'X', 'o', 'O', '-', '#', ' ', ' '
    };

  int row;
  int col;
  int tmp;

  /* Clear screen */

  printf("\e[1;1H\e[2J");

  /* Print board */

  for (int i = 0; i < BOARDY_SIZE; i++)
    {
      printf(" %d", i);
    }

  printf("\n");

  for (row = 0; row < BOARDY_SIZE; row++)
    {
      for (col = 0; col < BOARDX_SIZE; col++)
        {
          printf("|%c", board_icons[board[row][col]]);
        }

      printf("|\n");
    }

  for (tmp = 0; tmp < BOARDX_SIZE * 2 + 1; tmp++)
    {
      printf("-");
    }

  printf("\n");
}
#endif /* DEBUG_MATCH4_GAME */

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
 *   fd - File descriptor for screen
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

#ifdef DEBUG_MATCH4_GAME
  print_board();
#endif

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

  usleep(200);
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
 * Name: put_mark
 *
 * Description:
 *   Drop a piece on the selected column
 *
 * Parameters:
 *   col   - Column number
 *   piece - Piece to drop
 *
 * Returned Value:
 *   OK on success, ERROR if not.
 *
 ****************************************************************************/

int put_mark(int col, uint32_t piece)
{
  int i = 0;
  if (col < 0 || col >= BOARDY_SIZE)
    {
      return OK;
    }

  for (i = BOARDX_SIZE - 1; i >= 0; i--)
    {
      if (board[i][col] == BLANK_PLACE)
        {
          board[i][col] = piece;
          return OK;
        }
    }

  return ERROR;
}

/****************************************************************************
 * Name: check_winner
 *
 * Description:
 *   Check for 4 in a row in any direction
 *
 * Parameters:
 *   piece - Mark number depends on turn
 *   blink - Flag to blink
 *
 * Returned Value:
 *   OK on success, ERROR if not.
 *
 ****************************************************************************/

int check_winner(uint32_t piece, bool blink, int fd)
{
  int i = 0;
  int j = 0;
  int k = 0;

  /* Horizontal 4 piece check */

  for (i = 0; i < BOARDX_SIZE; i++)
    {
      for (j = 0; j <= BOARDY_SIZE - 4; j++)
        {
          if (board[i][j] == piece &&
              board[i][j + 1] == piece &&
              board[i][j + 2] == piece &&
              board[i][j + 3] == piece)
            {
              if (blink)
                {
                  for (k = 0; k < 3; k++)
                    {
                      board[i][j] = WHITE_INDEX;
                      board[i][j + 1] = WHITE_INDEX;
                      board[i][j + 2] = WHITE_INDEX;
                      board[i][j + 3] = WHITE_INDEX;
                      draw_board(fd);
                      usleep(300000);

                      board[i][j] = piece;
                      board[i][j + 1] = piece;
                      board[i][j + 2] = piece;
                      board[i][j + 3] = piece;
                      draw_board(fd);
                      usleep(300000);
                    }
                }

              return OK;
            }
        }
    }

  /* Vertical 4 piece check */

  for (i = 0; i <= BOARDX_SIZE - 4; i++)
    {
      for (j = 0; j < BOARDY_SIZE; j++)
        {
          if (board[i][j] == piece &&
              board[i + 1][j] == piece &&
              board[i + 2][j] == piece &&
              board[i + 3][j] == piece)
            {
              if (blink)
                {
                  for (k = 0; k < 3; k++)
                    {
                      board[i][j] = WHITE_INDEX;
                      board[i + 1][j] = WHITE_INDEX;
                      board[i + 2][j] = WHITE_INDEX;
                      board[i + 3][j] = WHITE_INDEX;
                      draw_board(fd);
                      usleep(300000);

                      board[i][j] = piece;
                      board[i + 1][j] = piece;
                      board[i + 2][j] = piece;
                      board[i + 3][j] = piece;
                      draw_board(fd);
                      usleep(300000);
                    }
                }

              return OK;
            }
        }
    }

  /* Diagonal down-right 4 piece check */

  for (i = 0; i <= BOARDX_SIZE - 4; i++)
    {
      for (j = 0; j <= BOARDY_SIZE - 4; j++)
        {
          if (board[i][j] == piece &&
              board[i + 1][j + 1] == piece &&
              board[i + 2][j + 2] == piece &&
              board[i + 3][j + 3] == piece)
            {
              if (blink)
                {
                  for (k = 0; k < 3; k++)
                    {
                      board[i][j] = WHITE_INDEX;
                      board[i + 1][j + 1] = WHITE_INDEX;
                      board[i + 2][j + 2] = WHITE_INDEX;
                      board[i + 3][j + 3] = WHITE_INDEX;
                      draw_board(fd);
                      usleep(300000);

                      board[i][j] = piece;
                      board[i + 1][j + 1] = piece;
                      board[i + 2][j + 2] = piece;
                      board[i + 3][j + 3] = piece;
                      draw_board(fd);
                      usleep(300000);
                    }
                }

              return OK;
            }
        }
    }

  /* Diagonal up-right 4 piece check */

  for (i = 3; i < BOARDX_SIZE; i++)
    {
      for (j = 0; j <= BOARDY_SIZE - 4; j++)
        {
          if (board[i][j] == piece &&
              board[i - 1][j + 1] == piece &&
              board[i - 2][j + 2] == piece &&
              board[i - 3][j + 3] == piece)
            {
              if (blink)
                {
                  for (k = 0; k < 3; k++)
                    {
                      board[i][j] = WHITE_INDEX;
                      board[i - 1][j + 1] = WHITE_INDEX;
                      board[i - 2][j + 2] = WHITE_INDEX;
                      board[i - 3][j + 3] = WHITE_INDEX;
                      draw_board(fd);
                      usleep(300000);

                      board[i][j] = piece;
                      board[i - 1][j + 1] = piece;
                      board[i - 2][j + 2] = piece;
                      board[i - 3][j + 3] = piece;
                      draw_board(fd);
                      usleep(300000);
                    }
                }

              return OK;
            }
        }
    }

  return ERROR;
}

/****************************************************************************
 * Name: is_board_empty
 *
 * Description:
 *   Check if the board is full to decide there is a draw
 *
 * Parameters:
 *   None.
 *
 * Returned Value:
 *   OK if there is a BLANK_PLACE space, ERROR if not.
 *
 ****************************************************************************/

static int is_board_empty(void)
{
  for (int j = 0; j < BOARDY_SIZE; j++)
    {
      if (board[0][j] == BLANK_PLACE)
        {
          return OK;
        }
    }

  return ERROR;
}

/****************************************************************************
 * Name: ai_move
 *
 * Description:
 *   Finds first empty column to fill
 *
 * Parameters:
 *   piece          - AI (CPU) piece value
 *   opponent_piece - Opponent piece value
 *
 * Returned Value:
 *   Column number if there is valid move, ERROR if not.
 *
 ****************************************************************************/

int ai_move(uint32_t piece, uint32_t opponent_piece)
{
  int row = 0;
  int col = 0;
  int valid_boardy_size[BOARDY_SIZE];
  int count = 0;

  /* Trying to find winner move */

  for (col = 0; col < BOARDY_SIZE; col++)
    {
      for (row = BOARDX_SIZE - 1; row >= 0; row--)
        {
          if (board[row][col] == BLANK_PLACE)
            {
              board[row][col] = piece;
              if (check_winner(piece, false, 0) == OK)
                {
                  board[row][col] = 0;
                  return col;
                }

              board[row][col] = 0;
              break;
            }
        }
    }

  /* Trying to block opponent let to win */

  for (col = 0; col < BOARDY_SIZE; col++)
    {
      for (row = BOARDX_SIZE - 1; row >= 0; row--)
        {
          if (board[row][col] == BLANK_PLACE)
            {
              board[row][col] = opponent_piece;
              if (check_winner(opponent_piece, false, 0) == OK)
                {
                  board[row][col] = 0;
                  return col;
                }

              board[row][col] = 0;
              break;
            }
        }
    }

  /* Choose random column */

  for (col = 0; col < BOARDY_SIZE; col++)
    {
      if (board[0][col] == BLANK_PLACE)
        {
          valid_boardy_size[count++] = col;
        }
    }

  if (count > 0)
    {
      return valid_boardy_size[rand() % count];
    }

  /* Return ERROR if no valid move */

  return ERROR;
}

/****************************************************************************
 * Name: find_empty_col
 *
 * Description:
 *   Finds first empty column to fill
 *
 * Parameters:
 *   col - Column number to start
 *   dir - Direction from that column
 *
 * Returned Value:
 *   First empty column to fill.
 *
 ****************************************************************************/

static int find_empty_col(int col, int dir)
{
  int  i = 0;
  while (i < BOARDY_SIZE)
    {
      if ((col == 7) && (dir == 1))
        {
          col = 0;
        }
      else if ((col == BLANK_PLACE) && (dir == -1))
        {
          col = 7;
        }
      else
        {
          col = col + dir;
        }

      if (board[0][col] == BLANK_PLACE)
        {
          break;
        }

      i++;
    }

  return col;
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
  fill_board(BLANK_PLACE);
}

/****************************************************************************
 * Name: get_col_input
 *
 * Description:
 *   Gets column number to put mark into board.
 *
 * Parameters:
 *   input_st - Input struct pointer to get input
 *   turn     - Turn number
 *   col      - Column number pointer to return
 *   fd       - File descriptor to print
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void get_col_input(struct input_state_s *input_st,
                          int turn, int *col, int fd)
{
  int ret = ERROR;
  uint8_t column = find_empty_col(-1, 1);
  bool refresh_board = true;

  printf("Player %d, choose column (0-%d): \n", turn + 1, BOARDX_SIZE - 1);
  while (input_st->dir != DIR_DOWN)
    {
      if (refresh_board)
        {
          refresh_board = false;
          put_mark(column, players_select[turn]);
          draw_board(fd);
          int j = 0;
          while (board[j][column] != players_select[turn])
            {
              j++;
            }

          board[j][column] = 0;
        }

      while (ret == ERROR || input_st->dir == DIR_NONE)
        {
          ret = dev_read_input(input_st);
        }

      if (input_st->dir == DIR_RIGHT)
        {
          column = find_empty_col(column, 1);
          refresh_board = true;
          input_st->dir = DIR_NONE;
          printf("right button pressed: %d\n", column);
          usleep(7000);
        }
      else if (input_st->dir == DIR_LEFT)
        {
          column = find_empty_col(column, -1);
          refresh_board = true;
          input_st->dir = DIR_NONE;
          printf("left button pressed: %d\n", column);
          usleep(7000);
        }

      usleep(15000);
    }

  usleep(15000);
  input_st->dir = DIR_NONE;
  *col = column;
}

/****************************************************************************
 * Name: draw_led_matrix
 *
 * Description:
 *   Fills board premade picture
 *
 * Parameters:
 *   img  - Image to show
 *   fd   - File descriptor to print
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void draw_led_matrix(uint32_t img[][BOARDY_SIZE], int fd)
{
  int i = 0;
  int j = 0;
  for (i = 0; i < BOARDX_SIZE; i++)
    {
      for (j = 0; j < BOARDX_SIZE; j++)
        {
          board[i][j] = img[i][j];
        }
    }

  draw_board(fd);
}

/****************************************************************************
 * Name: game_entrance
 *
 * Description:
 *   Screen that handles entrance screen to select game mode (1-2 player)
 *
 * Parameters:
 *   input_st - Input struct pointer to get input
 *   fd       - File descriptor to print
 *
 * Returned Value:
 *   True if one player mode, false if two player mode.
 *
 ****************************************************************************/

static bool game_entrance(struct input_state_s *input_st, int fd)
{
  int ret = ERROR;
  bool game_mode_one = true;

  draw_led_matrix(one_player_scr, fd);
  while (input_st->dir != DIR_DOWN)
    {
      while (ret == ERROR || input_st->dir == DIR_NONE)
        {
          ret = dev_read_input(input_st);
        }

      if (input_st->dir == DIR_LEFT || input_st->dir == DIR_RIGHT)
        {
          game_mode_one = !game_mode_one;
          if (game_mode_one)
            {
              draw_led_matrix(one_player_scr, fd);
            }
          else
            {
              draw_led_matrix(two_player_scr, fd);
            }

          input_st->dir = DIR_NONE;
        }

      usleep(15000);
    }

  usleep(15000);
  input_st->dir = DIR_NONE;
  return game_mode_one;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * match4_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int col = 0;
  int turn = 0;
  bool one_player_mode = true;
  int ret = ERROR;
  int board_fd = -1;
  bool game_over = false;
  struct input_state_s input_st;
  char player_symbol[2] =
    {
      'X', 'O'
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

restart:
  ret = 0;
  turn = 0;
  col = 0;
  one_player_mode = true;
  input_st.dir = DIR_NONE;
  game_over = false;

  one_player_mode = game_entrance(&input_st, board_fd);
  init_game();
  while (!game_over)
    {
      draw_board(board_fd);
      if ((one_player_mode && turn == 0) || !one_player_mode)
        {
          get_col_input(&input_st, turn, &col, board_fd);
        }
      else
        {
          col = ai_move(players[turn], players[(turn + 1) % 2]);
          printf("AI chooses column %d\n", col);
        }

      ret = put_mark(col, players[turn]);
      if (ret != 0)
        {
          printf("Column full or invalid. Try again.\n");
          continue;
        }

      if (check_winner(players[turn], true, board_fd) == OK)
        {
          if ((one_player_mode && turn == 0) || !one_player_mode)
            {
              printf("\nPlayer %d (%c) wins!\n",
                     turn + 1, player_symbol[turn]);
            }
          else
            {
              printf("\nAI (%c) wins!\n", player_symbol[turn]);
            }

          fill_board(players[turn]);
          draw_board(board_fd);
          usleep(85000);
          break;
        }

      if (is_board_empty() != OK)
        {
          printf("It's a draw!\n");
          break;
        }

      turn = (turn + 1) % 2;
      usleep(85000);
    }

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

  input_st.dir = DIR_NONE;
  dev_input_deinit();
  return 0;
}
