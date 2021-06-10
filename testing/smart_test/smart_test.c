/****************************************************************************
 * apps/testing/smart_test/smart_test.c
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

static int *g_line_pos;
static int *g_line_len;
static int g_seek_count = 0;
static int g_write_count = 0;
static int g_circ_count = 0;

static int g_line_count = 2000;
static int g_record_len = 64;
static int g_erase_count = 32;
static int g_totalz_records = 40000;

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

  printf("Writing test data.  %d lines to write\n", g_line_count);
  for (x = 0; x < g_line_count; x++)
    {
      g_line_pos[x] = ftell(fd);

      sprintf(string, "This is line %d at offset %d\n", x, g_line_pos[x]);
      g_line_len[x] = strlen(string);
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

  printf("Performing %d random seek tests\n", g_seek_count);

  srand(23);
  for (x = 0; x < g_seek_count; x++)
    {
      /* Get random line to seek to */

      index = rand();
      while (index >= g_line_count)
        {
          index -= g_line_count;
        }

      fseek(fd, g_line_pos[index], SEEK_SET);
      fread(readstring, 1, g_line_len[index], fd);
      readstring[g_line_len[index]] = '\0';

      sprintf(cmpstring, "This is line %d at offset %d\n",
              index, g_line_pos[index]);

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
         g_write_count);

  index = 0;
  for (x = 0; x < g_write_count; x++)
    {
#if 0
      /* Get a random value */

      index = rand();
      while (index >= g_line_count)
        {
          index -= g_line_count;
        }
#endif

      /* Read the data into the buffer */

      fseek(fd, g_line_pos[index], SEEK_SET);
      fread(readstring, 1, g_line_len[index], fd);
      readstring[g_line_len[index]] = '\0';

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

      fseek(fd, g_line_pos[index], SEEK_SET);
      fwrite(readstring, 1, g_line_len[index], fd);
      fflush(fd);

      /* Now read the data back and compare it */

      fseek(fd, g_line_pos[index], SEEK_SET);
      fread(cmpstring, 1, g_line_len[index], fd);
      cmpstring[g_line_len[index]] = '\0';

      if (strcmp(readstring, cmpstring) != 0)
        {
          printf("\nCompare failure on line %d, offset %d\n",
                 index, g_line_pos[index]);
          printf("\tExpected \"%s\"", cmpstring);
          printf("\rReceived \"%s\"", readstring);
          fseek(fd, g_line_pos[index], SEEK_SET);
          fread(cmpstring, 1, g_line_len[index], fd);
          pass = FALSE;
          break;
        }

      printf("\r%d", x);
      fflush(stdout);

      /* On to next line */

      if (++index >= g_line_count)
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
 * Name: smart_circular_log_test
 *
 * Description: Conducts a record based circular log seek / update test
 *
 * NOTE!!!
 *    In the test below, we use open / write, not fopen / fwrite.  If the
 *    fopen / fwrite routines were used instead, they would provide us with
 *    stream buffer management which we DO NOT WANT.  This will throw off
 *    the SMARTFS write sizes which will not generate the optimized wear
 *    operations.
 *
 ****************************************************************************/

static int smart_circular_log_test(char *filename)
{
  int       fd;
  char      *buffer;
  char      *cmpbuf;
  int       s1;
  int       x;
  int       record_no;
  int       buf_size;
  int       pass = TRUE;

  /* Calculate the size of our granular "erase record" writes */

  buf_size = g_record_len * g_erase_count;
  if (buf_size == 0)
    {
      printf("Invalid record parameters\n");
      return -EINVAL;
    }

  /* Allocate memory for the record */

  buffer = malloc(buf_size);
  if (buffer == NULL)
    {
      printf("Unable to allocate memory for record storage\n");
      return -ENOMEM;
    }

  /* Allocate memory for the compare buffer */

  cmpbuf = malloc(g_record_len);
  if (cmpbuf == NULL)
    {
      printf("Unable to allocate memory for record storage\n");
      free(buffer);
      return -ENOMEM;
    }

  /* Open the circular log file */

  fd = open(filename, O_RDWR | O_CREAT);
  if (fd == -1)
    {
      printf("Unable to create file %s\n", filename);
      free(buffer);
      free(cmpbuf);
      return -ENOENT;
    }

  /* Now fill the circular log with dummy 0xFF data */

  printf("Creating circular log with %d records\n", g_totalz_records);
  memset(buffer, 0xff, g_record_len);
  for (x = 0; x < g_totalz_records; x++)
    {
      write(fd, buffer, g_record_len);
    }

  close(fd);

  /* Now reopen the file for read/write mode */

  fd = open(filename, O_RDWR);
  if (fd == -1)
    {
      printf("Unable to open file %s\n", filename);
      free(buffer);
      free(cmpbuf);
      return -ENOENT;
    }

  printf("Performing %d circular log record update tests\n",
         g_circ_count);

  /* Start at record number zero and start updating log entries */

  record_no = 0;
  for (x = 0; x < g_circ_count; x++)
    {
      /* Fill a new record with random data */

      for (s1 = 0; s1 < g_record_len; s1++)
        {
          buffer[s1] = rand() & 0xff;
        }

      /* Set the first byte of the record (flag byte) to 0xFF */

      buffer[0] = 0xff;

      /* Seek to the record location in the file */

      lseek(fd, g_record_len * record_no, SEEK_SET);

      /* Write the new record to the file */

      if ((record_no & (g_erase_count - 1)) == 0)
        {
          /* Every g_erase_count records we will write a larger
           * buffer with our record and padded with 0xFF to
           * the end of our larger buffer.
           */

          memset(&buffer[g_record_len], 0xff, buf_size - g_record_len);
          write(fd, buffer, buf_size);
        }
      else
        {
          /* Just write a single record */

          write(fd, buffer, g_record_len);
        }

      /* Now perform a couple of simulated flag updates */

      lseek(fd, g_record_len * record_no, SEEK_SET);
      buffer[0] = 0xfe;
      write(fd, buffer, 1);
      lseek(fd, g_record_len * record_no, SEEK_SET);
      buffer[0] = 0xfc;
      write(fd, buffer, 1);

      /* Now read the data back and compare it */

      lseek(fd, g_record_len * record_no, SEEK_SET);
      read(fd, cmpbuf, g_record_len);

      for (s1 = 0; s1 < g_record_len; s1++)
        {
          if (buffer[s1] != cmpbuf[s1])
            {
              printf("\nCompare failure in record %d, offset %d\n",
                     record_no, record_no * g_record_len + s1);
              printf("\tExpected \"%02x\"", cmpbuf[s1]);
              printf("\rReceived \"%02x\"", buffer[s1]);
              pass = FALSE;
              break;
            }
        }

      printf("\r%d", x);
      fflush(stdout);

      /* Increment to the next record */

      if (++record_no >= g_totalz_records)
        {
          record_no = 0;
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

  close(fd);
  free(buffer);
  free(cmpbuf);
  return OK;
}

/****************************************************************************
 * Name: smart_usage
 *
 * Description: Displays usage information for the command.
 *
 ****************************************************************************/

static void smart_usage(void)
{
  fprintf(stderr, "usage: smart_test "
    "[-c COUNT] [-s SEEKCOUNT] [-w WRITECOUNT] smart_mounted_filename\n\n");

  fprintf(stderr, "DESCRIPTION\n");
  fprintf(stderr,
    "    Conducts various stress tests to validate SMARTFS operation.\n");
  fprintf(stderr,
    "    Please choose one or more of -c, -s, or -w to conduct tests.\n\n");

  fprintf(stderr, "OPTIONS\n");
  fprintf(stderr, "    -c COUNT\n");
  fprintf(stderr, "          "
    "Performs a circular log style test where a fixed number of fixed\n");
  fprintf(stderr, "          "
    "length records are written and then overwritten with new data.\n");
  fprintf(stderr, "          "
    "Uses the -r, -e and -t options to specify the parameters of the \n");
  fprintf(stderr, "          "
    "record geometry and update operation.  The COUNT parameter sets\n");
  fprintf(stderr, "          "
    "the number of record updates to perform.\n\n");

  fprintf(stderr, "    -s SEEKCOUNT\n");
  fprintf(stderr, "          "
    "Performs a simple seek test where to validate the SMARTFS seek\n");
  fprintf(stderr, "          "
    "operation.  Uses the -l option to specify the number of test\n");
  fprintf(stderr, "          "
    "lines to write to the test file.  The SEEKCOUNT parameter sets\n");
  fprintf(stderr, "          "
    "the number of seek/read operations to perform.\n\n");

  fprintf(stderr, "    -w WRITECOUNT\n");
  fprintf(stderr, "          "
    "Performs a seek/write/seek/read test where to validate the SMARTFS\n");
  fprintf(stderr, "          "
    "seek/write operation.  Uses the -l option to specify the number of\n");
  fprintf(stderr, "          test lines "
    "to write to the test file.  The WRITECOUNT parameter sets\n");
  fprintf(stderr, "          "
    "the number of seek/write operations to perform.\n\n");

  fprintf(stderr, "    -l LINECOUNT\n");
  fprintf(stderr, "          "
    "Sets the number of lines of test data to write to the test file\n");
  fprintf(stderr, "          "
    "during seek and seek/write tests.\n\n");

  fprintf(stderr, "    -r RECORDLEN\n");
  fprintf(stderr, "          "
    "Sets the length of each log record during circular log tests.\n\n");

  fprintf(stderr, "    -e ERASECOUNT\n");
  fprintf(stderr, "          Sets the erase granularity"
    " for overwriting old circular log entries.\n");
  fprintf(stderr, "          Setting this value to 16, "
    "for instance, would cause every 16th record\n");
  fprintf(stderr, "          update "
    "to write a single record followed by 15 records with all 0xFF\n");
  fprintf(stderr, "          content.  "
    "This helps SMARTFS perform better wear leveling and reduces\n");
  fprintf(stderr, "          "
    "the number of FLASH block erases significantly.\n\n");

  fprintf(stderr, "    -t TOTALRECORDS\n");
  fprintf(stderr, "          "
    "Sets the total number of records in the circular log test file.\n\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;
  int opt;

  /* Argument given? */

  while ((opt = getopt(argc, argv, "c:e:l:r:s:t:w:")) != -1)
    {
      switch (opt)
        {
          case 'c':
            g_circ_count = atoi(optarg);
            break;

          case 'e':
            g_erase_count = atoi(optarg);
            break;

          case 'l':
            g_line_count = atoi(optarg);
            break;

          case 'r':
            g_record_len = atoi(optarg);
            break;

          case 's':
            g_seek_count = atoi(optarg);
            break;

          case 't':
            g_totalz_records = atoi(optarg);
            break;

          case 'w':
            g_write_count = atoi(optarg);
            break;

          default: /* '?' */
            smart_usage();
            exit(EXIT_FAILURE);
      }
   }

  if (argc < 2 || (g_seek_count + g_write_count + g_circ_count == 0))
    {
      smart_usage();
      return -1;
    }

  /* Allocate memory for the test */

  g_line_pos = malloc(g_line_count * sizeof(int));
  if (g_line_pos == NULL)
    {
      return -1;
    }

  g_line_len = malloc(g_line_count * sizeof(int));
  if (g_line_len == NULL)
    {
      free(g_line_pos);
      return -1;
    }

  /* Test if performing seek test or write test */

  if (g_seek_count > 0 || g_write_count > 0)
    {
      /* Create a test file */

      if ((ret = smart_create_test_file(argv[optind])) < 0)
        {
          goto err_out_with_mem;
        }

      /* Conduct a seek test? */

      if (g_seek_count > 0)
        {
          if ((ret = smart_seek_test(argv[optind])) < 0)
            {
              goto err_out_with_mem;
            }
        }

      /* Conduct an append test */

      if ((ret = smart_append_test(argv[optind])) < 0)
        {
          goto err_out_with_mem;
        }

      /* Conduct a seek with write test? */

      if (g_write_count > 0)
        {
          if ((ret = smart_seek_with_write_test(argv[optind])) < 0)
            {
              goto err_out_with_mem;
            }
        }
    }

  /* Perform a "circular log" test */

  if ((ret = smart_circular_log_test(argv[optind])) < 0)
    {
      goto err_out_with_mem;
    }

err_out_with_mem:

  /* Free the memory */

  free(g_line_pos);
  free(g_line_len);
  return ret;
}
