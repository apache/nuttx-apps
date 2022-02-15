/****************************************************************************
 * apps/examples/adxl372_test/adxl372_test_main.c
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <nuttx/spi/spi.h>
#include <arch/board/board.h>
#include <nuttx/fs/fs.h>
#include <nuttx/sensors/adxl372.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define PASSED        0

#define SUB_PROMPT   "stst >"
#define CRED         "\x1b[31m"
#define CGREEN       "\x1b[32m"
#define CYELLOW      "\x1b[33m"
#define CBLUE        "\x1b[34m"
#define CMAGENTA     "\x1b[35m"
#define CCYAN        "\x1b[36m"
#define CRESET       "\x1b[0m"

/* Return codes */

#define RC_OPENFAIL  (-1)
#define RC_OPENRONLY (-2)
#define RC_SEEKFAIL  (-3)
#define RC_READFAIL  (-4)
#define RC_IDFAIL    (-5)
#define RC_SEEK2FAIL (-6)
#define RC_WRITEFAIL (-7)
#define RC_READ2FAIL (-8)
#define RC_WRMFAIL   (-9)
#define RC_SEEK3FAIL (-10)
#define RC_READ3FAIL (-11)
#define RC_INVALPATH (-41)
#define RC_INVALPARM (-42)

/****************************************************************************
 * Private types
 ****************************************************************************/

typedef CODE int (*test_ptr_t)(int, FAR char *);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: subtest_prompt()
 *
 * Description:
 *   1. Display the prompt.
 *   2. Receive a character from the console port.
 *   3. Filter the input character. Invalid characters convert to either
 *     '!' (error) of '*' (invalid character).
 *   4. Echo the character.
 *
 ****************************************************************************/

static char subtest_prompt(FAR char *prompt)
{
  char c;

  printf("%s", prompt);
  fflush(stdout);
  c = getchar();
  if (c < 1)
    {
      c = '!';
    }
  else if ((c < 0x20 || c > 0x7e) && c != '\r')
    {
      c = '*';
    }

  if (c != '\r')
    {
      putchar(c);
    }

  printf("\n");
  return c;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adxl372_test
 *
 * Description:
 *   This function is public so that it can be called by board diagnostic
 *   programs that contain the ADXL372.
 *
 ****************************************************************************/

static int adxl372_test(int is_interactive, FAR char *path)
{
  int fd;
  int ret;
  int errcode;
  int i;
  char ch1 = 0;
  struct XYZ
  {
    int16_t d[3];
  };

  unsigned char tstchars[] =
  {
    0x5e, 0xc5, 0x00
  };

  char bfr[32] __attribute__((aligned(2)));  /* REVISIT: GCC dependent attribute */
  FAR struct XYZ *pxyz = (FAR struct XYZ *) bfr;
  int rc = PASSED;
  int rc_step = PASSED;

  printf("\nADXL372 accelerometer diagnostic started...\n");
  fd = open (path, O_RDWR);
  if (fd < 0 || (errno != EROFS && errno != 0))
    {
      errcode = errno;
      printf(CRED "ERROR: Failed to open %s: %d" CRESET "\n", path, errcode);
      return RC_OPENFAIL;
    }
  else if (errno == EROFS)
    {
      printf(CRED "ERROR: Accelerometer mounted as Read Only." CRESET "\n",
             path, errno);
      rc = RC_OPENRONLY;
    }

  printf("open errno=%d\n", errno);

  do
    {
      /* read id register */

      printf("Reading ADXL372 ID_REG...\n");
      ret = lseek(fd, ADXL372_DEVID_AD, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", ADXL372_DEVID_AD, path, errcode);
          rc = RC_SEEKFAIL;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, bfr, 4);   /* read the sensor id regs */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to read sensor ID from %s: %d"
                 CRESET "\n", path, errcode);
          rc = RC_READFAIL;
          goto error_exit;
        }

      if (bfr[0] != ADXL372_DEVID_AD_VALUE &&
          bfr[1] != ADXL372_DEVID_MST_VALUE &&
          bfr[2] != ADXL372_PARTID_VALUE)
        {
          printf(CRED "ERROR: Sensor ID is 0x%02X%02X%02X%02X, "
                "expected 0x%02X%02X%02Xxx." CRESET "\n",
                 bfr[0], bfr[1], bfr[2], bfr[3],
                 ADXL372_DEVID_AD_VALUE, ADXL372_DEVID_MST_VALUE,
                 ADXL372_PARTID_VALUE);

          if (rc == 0)
            {
              rc = RC_IDFAIL;
            }

          if (!is_interactive)
            {
              goto error_exit;
            }
        }
      else
        {
          printf("Sensor ID 0x%02X%02X%02X%02X is correct!\n",
            bfr[0], bfr[1], bfr[2], bfr[3]);
        }

      if (is_interactive)
        {
          ch1 = subtest_prompt("Press 'l' to repeat or enter to continue.\n"
                               SUB_PROMPT);
        }

      if (ch1 == 'x')
        {
          goto quick_exit;
        }
    }
  while (ch1 == 'l');

  do
    {
      /* Write and read a scratch register */

      printf("Writing and reading a scratch register...\n");
      ret = lseek(fd, ADXL372_SCRATCH, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", ADXL372_SCRATCH, path, errcode);
          rc = RC_SEEK2FAIL;
          goto error_exit;
        }

      rc_step = PASSED;
      for (i = 0; i < sizeof(tstchars); i++)
        {
          memset(bfr, 0xaa, sizeof(bfr));
          bfr[0] = (char)tstchars[i];
          ret = write(fd, bfr, 1);  /* write the scratch register */
          if (ret < 0 && rc != RC_OPENRONLY)
            {
              errcode = errno;
              printf(CRED "ERROR: Write operation failed to %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_WRITEFAIL;
              goto error_exit;
            }

          memset(bfr, 0xaa, sizeof(bfr));
          ret = read(fd, bfr, 1);   /* read the scratch register */
          if (ret < 0)
            {
              errcode = errno;
              printf(CRED "ERROR: Read operation failed from %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_READ2FAIL;
              goto error_exit;
            }

          if (bfr[0] != (char)tstchars[i])
            {
              printf(CRED "ERROR: Wrote 0x%02X, read back 0x%02X."
                     CRESET "\n", tstchars[i], bfr[0]);
              rc_step = RC_WRMFAIL;
              if (rc == 0)
                {
                  rc = RC_WRMFAIL;
                }

              if (!is_interactive)
                {
                  goto error_exit;
                }
            }
        }

      if (rc_step == PASSED)
        {
          printf("Read/write test passed!\n");
        }

      if (is_interactive)
        {
          ch1 = subtest_prompt("Press 'l' to repeat or enter to continue.\n"
                               SUB_PROMPT);
        }

      if (ch1 == 'x')
        {
          goto quick_exit;
        }
    }
  while (ch1 == 'l');

  do
    {
      /* Read live data */

      printf("Reading Live Data...\n");
      ret = lseek(fd, 0, SEEK_END);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", ADXL372_XDATA_H, path, errcode);
          rc = RC_SEEK3FAIL;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, bfr, 6);   /* read live accelerometer data */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Read operation failed from %s: %d" CRESET "\n",
                 path, errcode);
          rc = RC_READ3FAIL;
          goto error_exit;
        }

      printf("ADXL372 = ( %6d, %6d, %6d))\n",
             pxyz->d[0], pxyz->d[1], pxyz->d[2]);
      printf("ADXL372 = ( 0x%04X, 0x%04X, 0x%04X)\n",
             pxyz->d[0], pxyz->d[1], pxyz->d[2]);
      printf("ADXL372 raw = ( 0x%02X%02X, 0x%02X%02X, 0x%02X%02X)\n",
             bfr[1], bfr[0], bfr[3], bfr[2], bfr[5], bfr[4]);

      if (is_interactive)
        {
          ch1 = subtest_prompt("Press 'l' to repeat or enter to continue.\n"
                               SUB_PROMPT);
        }

      if (ch1 == 'x')
        {
          goto quick_exit;
        }
    }
  while (ch1 == 'l');

error_exit:
quick_exit:
  close(fd);
  if (rc == PASSED)
    {
      printf(CGREEN
            "ADXL372 accelerometer diagnostic completed successfully."
             CRESET "\n");
    }

  return rc;
}

/****************************************************************************
 * Name: adxl372_test_main
 *
 * Description:
 *   This program defaults to testing in interactive mode.
 *   To run in batch mode use a first parameter of b. E.g. adxl372_test -b
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int rc = 0;
  int step_rc = 0;
  uint8_t ch;
  int is_interactive = 1; /* Program defaults to interactive */
  int flag_present = 0;   /* Default to no flag coded */
  uint32_t ui;
  struct stat sbuf;

  /* The two arrays below must be synchronized */

  test_ptr_t test_ptr_array[] = /* Array of test programs */
    {
      adxl372_test,   /* ADXL372 accelerometer tests */
    };

  FAR char *test_path[ARRAYSIZE(test_ptr_array)];

  if (argc < 1 || *argv[1] == 0 || *(argv[1] + 1) == 0)
    {
      goto print_help;
    }

  /* We have at least 1 parameter which has at least two characters. */

  if (*argv[1] == '-')
    {
      flag_present = 1;
      if (*(argv[1] + 1) == 'b')
        {
          is_interactive = 0;
        }
      else if (*(argv[1] + 1) == 'i')
        {
          is_interactive = 1;
        }
      else
        {
          goto print_help;
        }
    }

  if (flag_present && argc < 2)
    {
      goto print_help;
    }

  test_path[0] = argv[1 + flag_present];

  printf("path=%s\n", test_path[0]);

  if (stat(test_path[0], &sbuf))
    {
      printf(CRED "Error: Accelerometer path not found. Path=%s" CRESET "\n",
             test_path[0]);
      rc = RC_INVALPATH;
    }

  if (rc)
    {
      return rc;
    }

  if (is_interactive)
    {
      printf("LSM330 diagnostic started in interactive mode...\n");
      ch = 0;
      while (ch != 'x')
        {
          printf("Enter test section...\n");
          printf("  1 = SPI ADXL Tests.\n");
          printf("  i = Toggle interactive mode.\n");
          printf("  x = Exit. (at anytime).\n");
          printf("stst> ");
          fflush(stdout);

          ch = getchar();
          if (ch < 1)
            {
              ch = '!';
            }
          else if ((ch < 0x20 || ch > 0x7e) && ch != '\r')
            {
              ch = '*';
            }

          if (ch != '\r')
            {
              putchar(ch);
            }

          printf("\n");
          if (ch == 'x')
            {
              return 0;
            }

          ui = ch - 0x31; /* make index zero origin */

          if (ch == 'i')
            {
              if (is_interactive == 0)
                {
                  is_interactive = 1;
                  printf("Set to interactive mode.\n");
                }
              else
                {
                  is_interactive = 0;
                  printf("Set to batch mode.\n");
                }
            }
          else if (ui >= ARRAYSIZE(test_ptr_array))
            {
              printf("Huh?\n");
            }
          else
            {
              if (test_ptr_array[ui] == 0)
                {
                  printf ("Test not supported yet.\n");
                }
              else
                {
                  test_ptr_array[ui](is_interactive, test_path[ui]);
                }
            }
        }
    }
  else /* not interactive mode */
    {
      printf("ADXL372 sensor diagnostic started in batch mode...\n");

      for (ui = 0; ui < ARRAYSIZE(test_ptr_array); ui++)
        {
          step_rc = 0;
          if (test_ptr_array[ui] != 0)
            {
              /* call the next test routine */

              step_rc = test_ptr_array[ui](is_interactive, test_path[ui]);
            }

          if (step_rc != 0 && rc == 0)
            {
              rc = step_rc;
            }

          sleep(1);
        }
    }

  printf("ADXL372 sensor diagnostic completed.\n");
  return rc;

print_help:
  printf("Usage...\n");
  printf("adxl372_test [-b | -i] <dev_path>\n");
  printf("    -b = batch mode execution\n");
  printf("    -i = interactive mode execution (default)\n");
  printf("    <dev_path> = Device path for the ADXL372 accelerometer\n");
  printf(" Example:\n");
  printf("   adxl372_test -b /dev/adxl372_0\n");
  return RC_INVALPARM;
}
