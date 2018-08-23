/****************************************************************************
 * apps/system/sudoku/sudoku.c
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
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "system/readline.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NHISTORY 16

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sudoku_s
{
  uint16_t canbe;
  uint16_t is;
};
typedef struct sudoku_s sudoku_t[81];

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sudoku_t history[NHISTORY];
static sudoku_t working;
static int first;
static int last;
static int current;

static const char *r1[8] =
  {"   ", "1  ", " 2 ", "12 ", "  3", "1 3", " 23", "123" };
static const char *r2[8] =
  {"   ", "4  ", " 5 ", "45 ", "  6", "4 6", " 56", "456" };
static const char *r3[8] =
  {"   ", "7  ", " 8 ", "78 ", "  9", "7 9", " 89", "789" };
static char line[256];
static int nis = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void init_array(void)
{
  int i;

  first = 0;
  last = 0;
  current = 0;

  for (i = 0; i < 81; i++)
    {
      working[i].canbe = 0x1ff;
      working[i].is    = 0x000;
    }
}

static inline void push(void)
{
  int next = current + 1;
  if (next >= NHISTORY) next = 0;

  if (current == last)
    {
      last = next;
    }

  if (first == next)
  {
    if (++first >= NHISTORY) first = 0;
  }

  memcpy (history[current], working, sizeof(sudoku_t));
  current = next;
}

static inline void undo(void)
{
  if (current == first)
    {
      printf("Cannot undo\n");
    }
  else
    {
      if (--current < 0) current = NHISTORY-1;
      memcpy (working, history[current], sizeof(sudoku_t));
    }
}

static inline void redo(void)
{
  if (current == last)
    {
      printf("Cannot redo\n");
    }
  else
    {
      if (++current >= NHISTORY) current = 0;
      memcpy(working, history[current], sizeof(sudoku_t));
    }
}

static inline void show_row(int row)
{
  printf("    |%3s|%3s|%3s| |%3s|%3s|%3s| |%3s|%3s|%3s|\n",
         r1[working[9*row + 0].canbe & 0x007],
         r1[working[9*row + 1].canbe & 0x007],
         r1[working[9*row + 2].canbe & 0x007],
         r1[working[9*row + 3].canbe & 0x007],
         r1[working[9*row + 4].canbe & 0x007],
         r1[working[9*row + 5].canbe & 0x007],
         r1[working[9*row + 6].canbe & 0x007],
         r1[working[9*row + 7].canbe & 0x007],
         r1[working[9*row + 8].canbe & 0x007]);
  printf(" %2d |%3s|%3s|%3s| |%3s|%3s|%3s| |%3s|%3s|%3s|\n", row + 1,
         r2[(working[9*row + 0].canbe >> 3) & 0x007],
         r2[(working[9*row + 1].canbe >> 3) & 0x007],
         r2[(working[9*row + 2].canbe >> 3) & 0x007],
         r2[(working[9*row + 3].canbe >> 3) & 0x007],
         r2[(working[9*row + 4].canbe >> 3) & 0x007],
         r2[(working[9*row + 5].canbe >> 3) & 0x007],
         r2[(working[9*row + 6].canbe >> 3) & 0x007],
         r2[(working[9*row + 7].canbe >> 3) & 0x007],
         r2[(working[9*row + 8].canbe >> 3) & 0x007]);
  printf("    |%3s|%3s|%3s| |%3s|%3s|%3s| |%3s|%3s|%3s|\n",
         r3[(working[9*row + 0].canbe >> 6) & 0x007],
         r3[(working[9*row + 1].canbe >> 6) & 0x007],
         r3[(working[9*row + 2].canbe >> 6) & 0x007],
         r3[(working[9*row + 3].canbe >> 6) & 0x007],
         r3[(working[9*row + 4].canbe >> 6) & 0x007],
         r3[(working[9*row + 5].canbe >> 6) & 0x007],
         r3[(working[9*row + 6].canbe >> 6) & 0x007],
         r3[(working[9*row + 7].canbe >> 6) & 0x007],
         r3[(working[9*row + 8].canbe >> 6) & 0x007]);
}

static inline void show_array(void)
{
  int row;
  int i;

  printf("    +===+===+===+ +===+===+===+ +===+===+===+\n");
  printf("    | 1 | 2 | 3 | | 4 | 5 | 6 | | 7 | 8 | 9 |\n");
  printf("    +===+===+===+ +===+===+===+ +===+===+===+\n");

  row = 0;
  for (i = 0; i < 3; i++)
    {
      show_row(row++);
      printf("    +---+---+---+ +---+---+---+ +---+---+---+\n");
      show_row(row++);
      printf("    +---+---+---+ +---+---+---+ +---+---+---+\n");
      show_row(row++);
      printf("    +===+===+===+ +===+===+===+ +===+===+===+\n");
    }
}

static inline int is_unique(unsigned int val)
{
  switch (val)
    {
    case 0x001:
    case 0x002:
    case 0x004:
    case 0x008:
    case 0x010:
    case 0x020:
    case 0x040:
    case 0x080:
    case 0x100:
      return 1;
    default:
      return 0;
    }
}

static inline int impossible(void)
{
  int nchanges = 0;
  unsigned int val;
  int row1, col1;
  int row2, col2;
  int i, j;
  int ndx1, ndx2;

  for (row1 = 0; row1 < 9; row1++)
    {
      row2 = row1 / 3;
      for (col1 = 0; col1 < 9; col1++)
        {
          col2 = col1 / 3;
          ndx1 = row1*9 + col1;
          val = working[ndx1].canbe;

          for (i = 0; i < 9; i++)
            {
              ndx2 = i*9 + col1;
              if (ndx1 != ndx2)
                {
                  val &= ~working[ndx2].is;
                }
            }

          for (j = 0; j < 9; j++)
            {
              ndx2 = row1*9 + j;
              if (ndx1 != ndx2)
                {
                  val &= ~working[ndx2].is;
                }
            }

          for (i = 0; i < 3; i++)
            {
              for (j = 0; j < 3; j++)
                {
                  ndx2 = (row2*3 + i)*9 + (col2*3 + j);
                  if (ndx1 != ndx2)
                    {
                      val &= ~working[ndx2].is;
                    }
                }
            }

          if (val == 0)
            {
              return -1;
            }

          if (val != working[ndx1].canbe)
            {
              nchanges++;
              working[ndx1].canbe = val;

              if (is_unique(val))
                {
                  working[ndx1].is = val;
                }
            }
        }
    }

  return nchanges;
}

static inline int unique(void)
{
  int noccurences[9];
  int lastndx[9];
  int nchanges = 0;
  unsigned int mask;
  unsigned int val;
  int row1, col1;
  int row2, col2;
  int ndx;
  int i;

  for (row1 = 0; row1 < 9; row1++)
    {
      for (i = 0; i < 9; i++)
        {
          noccurences[i] = 0;
          lastndx[i] = -1;
        }

      for (i = 0; i < 9; i++)
        {
          mask = 1 << i;
          for (col1 = 0; col1 < 9; col1++)
            {
              ndx = row1 * 9 + col1;
              if (working[ndx].canbe & mask)
                {
                  noccurences[i]++;
                  lastndx[i] = ndx;
                }
            }

          for (i = 0; i < 9; i++)
            {
              if (noccurences[i] == 1)
                {
                  val = 1 << i;
                  ndx = lastndx[i];
                  if (working[ndx].is != val)
                    {
                      working[ndx].is = val;
                      working[ndx].canbe = val;
                      nchanges++;
                    }
                }
            }
        }
    }

  for (col1 = 0; col1 < 9; col1++)
    {
      for (i = 0; i < 9; i++)
        {
          noccurences[i] = 0;
          lastndx[i] = -1;
        }

      for (i = 0; i < 9; i++)
        {
          mask = 1 << i;
          for (row1 = 0; row1 < 9; row1++)
            {
              ndx = row1 * 9 + col1;
              if (working[ndx].canbe & mask)
                {
                  noccurences[i]++;
                  lastndx[i] = ndx;
                }
            }

          for (i = 0; i < 9; i++)
            {
              if (noccurences[i] == 1)
                {
                  val = 1 << i;
                  ndx = lastndx[i];
                  if (working[ndx].is != val)
                    {
                      working[ndx].is = val;
                      working[ndx].canbe = val;
                      nchanges++;
                    }
                }
            }
        }
    }

  for (row2 = 0; row2 < 3; row2++)
    {
      for (col2 = 0; col2 < 3; col2++)
        {
          for (i = 0; i < 9; i++)
            {
              noccurences[i] = 0;
              lastndx[i] = -1;
            }

          for (i = 0; i < 9; i++)
            {
              mask = 1 << i;
              for ( row1 = 0; row1 < 3; row1++)
                {
                  for ( col1 = 0; col1 < 3; col1++)
                    {
                      ndx = (row2*3 + row1)*9 + (col2*3 + col1);
                      if (working[ndx].canbe & mask)
                        {
                          noccurences[i]++;
                          lastndx[i] = ndx;
                        }
                    }
                }
            }

          for (i = 0; i < 9; i++)
            {
              if (noccurences[i] == 1)
                {
                  val = 1 << i;
                  ndx = lastndx[i];
                  if (working[ndx].is != val)
                    {
                      working[ndx].is = val;
                      working[ndx].canbe = val;
                      nchanges++;
                    }
                }
            }
        }
    }

  return nchanges;
}

static inline int getcell(int *pndx, int *pval)
{
  int row, col, ndx;
  unsigned int val;
  int i = 0;

  while ((line[i] == ' ' || line[i] == '\t') && (line[i] != '\0') && (line[i] != '\n')) i++;

  if (line[i] < '1' || line[i] > '9')
    {
      printf("Bad row: %c\n", line[i]);
      return -1;
    }
  row = line[i++] -'0' - 1;

  while ((line[i] == ' ' || line[i] == '\t') && (line[i] != '\0') && (line[i] != '\n')) i++;

  if (line[i] < '1' || line[i] > '9')
    {
      printf("Bad col: %c\n", line[i]);
      return -1;
    }
  col = line[i++] -'0' - 1;
  ndx = row*9 + col;

  while ((line[i] == ' ' || line[i] == '\t') && (line[i] != '\0') && (line[i] != '\n')) i++;

  if (line[i] < '1' || line[i] > '9')
    {
      printf("Bad value: %c\n", line[i]);
      return -1;
    }

  val = 1 << ((line[i] - '0') - 1);
  if (( val & working[ndx].canbe) == 0)
    {
      printf("Impossible value: %c\n", line[i]);
      return -1;
    }

  *pndx = ndx;
  *pval = val;
  return 0;
}

static inline int get_command(void)
{
  printf("%d of 81> ", nis);
  fflush(stdout);
  (void)readline(line, 256, stdin, stdout);

  int i = 0;

  while ((line[i] == ' ' || line[i] == '\t') && (line[i] != '\0') && (line[i] != '\n')) i++;

  return line[i];
}

static inline void show_usage(void)
{
  printf("ABC - Set cell[A,B] to C where A,B, and C are in {1..9}\n");
  printf("U   - Undo last cell assignment\n");
  printf("R   - Redo last cell assignment\n");
  printf("H|? - Show this message\n");
  printf("Q   - Quit\n");
}

static inline void count_cells(void)
{
  int i;

  nis = 0;
  for (i = 0; i < 81; i++) if (working[i].is > 0) nis++;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_LOADABLE
int main(int argc, FAR char *argv[])
#else
int sudoku_main(int argc, char **argv, char **envp)
#endif
{
  int cmd;
  int nchanged;
  int val;
  int ndx;

  init_array();
  for (;;)
    {
      count_cells();
      show_array();
      if (nis == 81)
        {
          printf("GAME OVER: 81 of 81 assigned\n");
          return 0;
        }
      else
        {
          cmd = get_command();
          switch (cmd)
            {
            case 'U':
            case 'u':
              undo();
              break;

            case 'R':
            case 'r':
              redo();
              break;

            case 'H':
            case 'h':
            case '?':
              show_usage();
              break;

            case 'Q':
            case 'q':
              return 0;

            default:
              if (getcell(&ndx, &val) == 0)
                {
                  push();
                  working[ndx].is    = val;
                  working[ndx].canbe = val;
                  for (;;)
                    {
                      nchanged = impossible();
                      if (nchanged < 0)
                        {
                          printf("This move is not possible\n");
                          undo();
                          break;
                        }
                      else if (nchanged == 0)
                        {
                          nchanged = unique();
                          if (nchanged == 0)
                            {
                              break;
                            }
                        }
                    }
                }
            }
        }
    }
}
