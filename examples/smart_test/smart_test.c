/****************************************************************************
 * apps/system/smart_test/smart_test.c
 *
 *   Copyright (C) 2013, 2015 Ken Pettit. All rights reserved.
 *   Author: Ken Pettit <pettitkd@gmail.com>
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
#include <nuttx/progmem.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private data
 ****************************************************************************/

static int *g_linePos;
static int *g_lineLen;
static int g_lineCount = 2000;
static int g_seekCount = 2000;
static int g_writeCount = 2000;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: smart_create_test_file
 *
 * Description: Creates the test file with test data that we will use
 *              to conduct the test.
 *
 ****************************************************************************/

static int smart_create_test_file(char *filename)
{
  FILE     *fd;
  int       x;
  char      string[80];

  /* Try to open the file */

  printf("Creating file %s for write mode\n", filename);

  fd = fopen(filename, "w+");

  if (fd == NULL)
    {
      printf("Unable to create file %s\n", filename);
      return -ENOENT;
    }

  /* Write data to the file.  The data consists of a bunch of
   * lines, each with a line number and the offset within the
   * file where that file starts.
   */

  printf("Writing test data.  %d lines to write\n", g_lineCount);
  for (x = 0; x < g_lineCount; x++)
    {
      g_linePos[x] = ftell(fd);

      sprintf(string, "This is line %d at offset %d\n", x, g_linePos[x]);
      g_lineLen[x] = strlen(string);
      fprintf(fd, "%s", string);

      printf("\r%d", x);
      fflush(stdout);
    }

  /* Close the file */

  printf("\r\nDone.\n");

  fclose(fd);

  return OK;
}

/****************************************************************************
 * Name: smart_seek_test
 *
 * Description: Conducts a seek test on the file.
 *
 ****************************************************************************/

static int smart_seek_test(char *filename)
{
  FILE     *fd;
  char      readstring[80];
  char      cmpstring[80];
  int       index;
  int       x;
  int       ret = OK;

  fd = fopen(filename, "r");
  if (fd == NULL)
    {
      printf("Unable to open file %s\n", filename);
      return -ENOENT;
    }

  printf("Performing %d random seek tests\n", g_seekCount);

  srand(23);
  for (x = 0; x < g_seekCount; x++)
    {
      /* Get random line to seek to */

      index = rand();
      while (index >= g_lineCount)
        {
          index -= g_lineCount;
        }

      fseek(fd, g_linePos[index], SEEK_SET);
      fread(readstring, 1, g_lineLen[index], fd);
      readstring[g_lineLen[index]] = '\0';

      sprintf(cmpstring, "This is line %d at offset %d\n",
              index, g_linePos[index]);

      if (strcmp(readstring, cmpstring) != 0)
        {
          printf("\nSeek error on line %d\n", index);
          printf ("\t Expected \"%s\"\n", cmpstring);
          printf ("\t Received \"%s\"\n", readstring);
          ret = -1;
        }

      printf("\r%d", x);
      fflush(stdout);
    }

  printf("\r%d", x);
  fflush(stdout);

  fclose(fd);
  return ret;
}

/****************************************************************************
 * Name: smart_append_test
 *
 * Description: Conducts an append test on the file.
 *
 ****************************************************************************/

static int smart_append_test(char *filename)
{
  FILE     *fd;
  int       pos;
  char      readstring[80];

  fd = fopen(filename, "a+");
  if (fd == NULL)
    {
      printf("Unable to open file %s\n", filename);
      return -ENOENT;
    }

  /* Now write some data to the end of the file */

  fprintf(fd, "This is a test of the append.\n");
  pos = ftell(fd);

  /* Now seek to the end of the file and ensure that is where
   * pos is.
   */

  fseek(fd, 0, SEEK_END);
  if (ftell(fd) != pos)
    {
      printf("Error opening for append ... data not at EOF\n");
    }

  /* Now seek to that position and read the data back */

  fseek(fd, 30, SEEK_END);
  fread(readstring, 1, 30, fd);
  readstring[30] = '\0';
  if (strcmp(readstring, "This is a test of the append.\n") != 0)
    {
      printf("\nAppend test failed\n");
    }
  else
    {
      printf("\nAppend test passed\n");
    }

  fclose(fd);
  return OK;
}

/****************************************************************************
 * Name: smart_seek_with_write_test
 *
 * Description: Conducts an append test on the file.
 *
 ****************************************************************************/

static int smart_seek_with_write_test(char *filename)
{
  FILE     *fd;
  char      temp;
  char      readstring[80];
  char      cmpstring[80];
  int       c;
  int       s1;
  int       s2;
  int       len;
  int       x;
  int       index;
  int       pass = TRUE;

  fd = fopen(filename, "r+");
  if (fd == NULL)
    {
      printf("Unable to open file %s\n", filename);
      return -ENOENT;
    }

  printf("Performing %d random seek with write tests\n",
         g_writeCount);

  index = 0;
  for (x = 0; x < g_writeCount; x++)
    {
#if 0
      /* Get a random value */

      index = rand();
      while (index >= g_lineCount)
        {
          index -= g_lineCount;
        }
#endif
      /* Read the data into the buffer */

      fseek(fd, g_linePos[index], SEEK_SET);
      fread(readstring, 1, g_lineLen[index], fd);
      readstring[g_lineLen[index]] = '\0';

      /* Scramble the data in the line */

      len = strlen(readstring);
      for (c = 0; c < 100; c++)
        {
          s1 = rand() % len;
          s2 = rand() % len;

          temp = readstring[s1];
          readstring[s1] = readstring[s2];
          readstring[s2] = temp;
        }

      /* Now write the data back to the file */

      fseek(fd, g_linePos[index], SEEK_SET);
      fwrite(readstring, 1, g_lineLen[index], fd);
      fflush(fd);

      /* Now read the data back and compare it */

      fseek(fd, g_linePos[index], SEEK_SET);
      fread(cmpstring, 1, g_lineLen[index], fd);
      cmpstring[g_lineLen[index]] = '\0';

      if (strcmp(readstring, cmpstring) != 0)
        {
          printf("\nCompare failure on line %d, offset %d\n",
                 index, g_linePos[index]);
          printf("\tExpected \"%s\"", cmpstring);
          printf("\rReceived \"%s\"", readstring);
          fseek(fd, g_linePos[index], SEEK_SET);
          fread(cmpstring, 1, g_lineLen[index], fd);
          pass = FALSE;
          break;
        }

      printf("\r%d", x);
      fflush(stdout);

      /* On to next line */

      if (++index >= g_lineCount)
        {
          index = 0;
        }
    }

  if (pass)
    {
      printf("\nPass\n");
    }
  else
    {
      printf("\nFail\n");
    }

  fclose(fd);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int smart_test_main(int argc, char *argv[])
#endif
{
  int ret, opt;
  int gTestCount = 10000;

  /* Argument given? */

  while ((opt = getopt(argc, argv, "l:s:w:")) != -1)
     {
       switch (opt)
         {
           case 'l':
             g_lineCount = atoi(optarg);
             break;

           case 's':
             g_seekCount = atoi(optarg);
             break;

           case 'w':
             g_writeCount = atoi(optarg);
             break;

           default: /* '?' */
             fprintf(stderr, "usage: smart_test [-l lineCount] [-s seekCount] [-w writeCount] smart_mounted_filename\n");
             exit(EXIT_FAILURE);
         }
   }

  if (argc < 2)
    {
      fprintf(stderr, "usage: smart_test [-l lineCount] [-s seekCount] [-w writeCount] smart_mounted_filename\n");
      return -1;
    }

  /* Allocate memory for the test */

  g_linePos = malloc(g_lineCount * sizeof(int));
  if (g_linePos == NULL)
    {
      return -1;
    }

  g_lineLen = malloc(g_lineCount * sizeof(int));
  if (g_lineLen == NULL)
    {
      free(g_linePos);
      return -1;
    }

  /* Create a test file */

  if ((ret = smart_create_test_file(argv[optind])) < 0)
    {
      goto err_out_with_mem;
    }

  /* Conduct a seek test */

  if ((ret = smart_seek_test(argv[optind])) < 0)
    {
      goto err_out_with_mem;
    }

  /* Conduct an append test */

  if ((ret = smart_append_test(argv[optind])) < 0)
    {
      goto err_out_with_mem;
    }

  /* Conduct a seek with write test */

  if ((ret = smart_seek_with_write_test(argv[optind])) < 0)
    {
      goto err_out_with_mem;
    }

err_out_with_mem:

  /* Free the memory */

  free(g_linePos);
  free(g_lineLen);
  return ret;
}
