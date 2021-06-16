/****************************************************************************
 * apps/examples/lsm330spi_test/lsm330spi_test_main.c
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
#include <nuttx/sensors/lsm330.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARRAYSIZE(x)  (sizeof((x)) / sizeof((x)[0]))
#define PASSED        0

#define SUB_PROMPT    "stst >"
#define CRED          "\x1b[31m"
#define CGREEN        "\x1b[32m"
#define CYELLOW       "\x1b[33m"
#define CBLUE         "\x1b[34m"
#define CMAGENTA      "\x1b[35m"
#define CCYAN         "\x1b[36m"
#define CRESET        "\x1b[0m"

/* Return codes */

#define RC_OPENFAILA  (-1)
#define RC_OPENRONLYA (-2)
#define RC_SEEKFAILA  (-3)
#define RC_READFAILA  (-4)
#define RC_IDFAILA    (-5)
#define RC_SEEK2FAILA (-6)
#define RC_WRITEFAILA (-7)
#define RC_READ2FAILA (-8)
#define RC_WRMFAILA   (-9)
#define RC_SEEK3FAILA (-10)
#define RC_READ3FAILA (-11)
#define RC_OPENFAILG  (-21)
#define RC_OPENRONLYG (-22)
#define RC_SEEKFAILG  (-23)
#define RC_READFAILG  (-24)
#define RC_IDFAILG    (-25)
#define RC_SEEK2FAILG (-26)
#define RC_WRITEFAILG (-27)
#define RC_READ2FAILG (-28)
#define RC_WRMFAILG   (-29)
#define RC_SEEK3FAILG (-30)
#define RC_READ3FAILG (-31)
#define RC_INVALPATH  (-41)
#define RC_INVALPARM  (-42)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE int (*test_ptr_t)(int, FAR char *);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: subtest_prompt
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
 * Name: lsm330acl_test
 *
 * Description:
 *   This function is public so that it can be called by board diagnostic
 *   programs that contain the LSM330.
 *
 ****************************************************************************/

int lsm330acl_test(int is_interactive, char *path)
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

  char bfr[32] __attribute__((aligned(2)));
  FAR struct XYZ *pxyz = (struct XYZ *)(&bfr[2]); /* Padding and Status byte 1st */
  int rc = PASSED;
  int rc_step = PASSED;

  printf("\nOBSC sensor diagnostic started...\n");
  errno = 0;
  fd = open(path, O_RDWR);
  if (fd < 0 || (errno != EROFS && errno != 0))
    {
      errcode = errno;
      printf(CRED "ERROR: Failed to open %s: %d" CRESET "\n", path, errcode);
      return RC_OPENFAILA;
    }
  else if (errno == EROFS)
    {
      printf(CRED "ERROR: Accelerometer mounted as Read Only."
             CRESET "\n", path, errno);
      rc = RC_OPENRONLYA;
    }

  printf("open errno=%d\n", errno);

  do
    {
      /* Read id register */

      printf("Reading LSM330 ACL ID_REG...\n");
      ret = lseek(fd, LSM330_ACL_IDREG, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_GYRO_IDREG, path, errcode);
          rc = RC_SEEKFAILA;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, bfr, 1);   /* read the sensor id */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to read sensor ID from %s: %d"
                 CRESET "\n", path, errcode);
          rc = RC_READFAILA;
          goto error_exit;
        }

      if (bfr[0] != LSM330_ACL_IDREG_VALUE)
        {
          printf(CRED "ERROR: Sensor ID is 0x%02X, expected 0x%02X."
                 CRESET "\n", bfr[0], LSM330_ACL_IDREG_VALUE);
          if (rc == 0)
            {
              rc = RC_IDFAILA;
            }

          if (!is_interactive)
            {
              goto error_exit;
            }
        }
      else
        {
          printf("Sensor ID 0x%02X is correct!\n", bfr[0]);
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
      /* write and read a scratch register */

      printf("Writing and reading a scratch register...\n");
      ret = lseek(fd, LSM330_ACL_SCRATCH, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_ACL_SCRATCH, path, errcode);
          rc = RC_SEEK2FAILA;
          goto error_exit;
        }

      rc_step = PASSED;
      for (i = 0; i < sizeof(tstchars); i++)
        {
          memset(bfr, 0xaa, sizeof(bfr));
          bfr[0] = (char)tstchars[i];
          ret = write(fd, bfr, 1);  /* write the scratch register */
          if (ret < 0 && rc != RC_OPENRONLYA)
            {
              errcode = errno;
              printf(CRED "ERROR: Write operation failed to %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_WRITEFAILA;
              goto error_exit;
            }

          memset(bfr, 0xaa, sizeof(bfr));
          ret = read(fd, bfr, 1);   /* read the scratch register */
          if (ret < 0)
            {
              errcode = errno;
              printf(CRED "ERROR: Read operation failed from %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_READ2FAILA;
              goto error_exit;
            }

          if (bfr[0] != (char)tstchars[i])
            {
              printf(CRED "ERROR: Wrote 0x%02X, read back 0x%02X."
                     CRESET "\n", tstchars[i], bfr[0]);
              rc_step = RC_WRMFAILA;
              if (rc == 0)
                {
                  rc = RC_WRMFAILA;
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
      ret = lseek(fd, LSM330_ACL_STATUS, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_ACL_STATUS, path, errcode);
          rc = RC_SEEK3FAILA;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, &bfr[1], 7);   /* read live accelerometer data */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Read operation failed from %s: %d" CRESET "\n",
                 path, errcode);
          rc = RC_READ3FAILA;
          goto error_exit;
        }

      printf("LSM330 ACL = (%6d, %6d, %6d), Stat=0x%02X\n",
              pxyz->d[0], pxyz->d[1], pxyz->d[2], bfr[1]);
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
      printf(CGREEN "LSM330 accelerometer diagnostic completed successfully."
             CRESET "\n");
    }

  return rc;
}

/****************************************************************************
 * Name: lsm330gyro_test
 *
 * Description:
 *   This function is public so that it can be called by board diagnostic
 *  programs that contain the LSM330.
 *
 ****************************************************************************/

int lsm330gyro_test(int is_interactive, FAR char *path)
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

  char bfr[32] __attribute__((aligned(2)));
  FAR struct XYZ *pxyz = (struct XYZ *)(&bfr[2]); /* Temp and Status byte 1st */
  int rc = PASSED;
  int rc_step = PASSED;

  errno = 0;
  printf("\nLSL330 gyroscope diagnostic started...\n");
  fd = open (path, O_RDWR);
  if (fd < 0 || (errno != EROFS && errno != 0))
    {
      errcode = errno;
      printf(CRED "ERROR: Failed to open %s: %d" CRESET "\n", path, errcode);
      return RC_OPENFAILG;
    }
  else if (errno == EROFS)
    {
      printf(CRED "ERROR: Gyroscope mounted as Read Only." CRESET "\n", path,
             errno);
      rc = RC_OPENRONLYG;
    }

  printf("open errno=%d\n", errno);

  do
    {
      /* Read id register */

      printf("Reading LSM330 GYRO ID_REG...\n");
      ret = lseek(fd, LSM330_GYRO_IDREG, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_GYRO_IDREG, path, errcode);
          rc = RC_SEEKFAILG;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, bfr, 1);   /* read the sensor id */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to read sensor ID from %s: %d"
                 CRESET "\n", path, errcode);
          rc = RC_READFAILG;
          goto error_exit;
        }

      if (bfr[0] != LSM330_GYRO_IDREG_VALUE)
        {
          printf(CRED "ERROR: Sensor ID is 0x%02X, expected 0x%02X."
                 CRESET "\n", bfr[0], LSM330_GYRO_IDREG_VALUE);
          if (rc == 0)
            {
              rc = RC_IDFAILG;
            }

          if (!is_interactive)
            {
              goto error_exit;
            }
        }
      else
        {
          printf("Sensor ID 0x%02X is correct!\n", bfr[0]);
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
      ret = lseek(fd, LSM330_GYRO_SCRATCH, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_GYRO_SCRATCH, path, errcode);
          rc = RC_SEEK2FAILG;
          goto error_exit;
        }

      rc_step = PASSED;
      for (i = 0; i < sizeof(tstchars); i++)
        {
          memset(bfr, 0xaa, sizeof(bfr));
          bfr[0] = (char)tstchars[i];
          ret = write(fd, bfr, 1);  /* write the scratch register */
          if (ret < 0 && rc != RC_OPENRONLYG)
            {
              errcode = errno;
              printf(CRED "ERROR: Write operation failed to %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_WRITEFAILG;
              goto error_exit;
            }

          memset(bfr, 0xaa, sizeof(bfr));
          ret = read(fd, bfr, 1);   /* read the scratch register */
          if (ret < 0)
            {
              errcode = errno;
              printf(CRED "ERROR: Read operation failed from %s: %d"
                     CRESET "\n", path, errcode);
              rc = RC_READ2FAILG;
              goto error_exit;
            }

          if (bfr[0] != (char)tstchars[i])
            {
              printf(CRED "ERROR: Wrote 0x%02X, read back 0x%02X."
                     CRESET "\n", tstchars[i], bfr[0]);
              rc_step = RC_WRMFAILG;
              if (rc == 0)
                {
                  rc = RC_WRMFAILG;
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
      ret = lseek(fd, LSM330_GYRO_OUT_TEMP, SEEK_SET);
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Failed to seek to reg 0x%02X in  %s: %d"
                 CRESET "\n", LSM330_GYRO_OUT_TEMP, path, errcode);
          rc = RC_SEEK3FAILG;
          goto error_exit;
        }

      memset(bfr, 0xaa, sizeof(bfr));
      ret = read(fd, bfr, 8);   /* read live accelerometer data */
      if (ret < 0)
        {
          errcode = errno;
          printf(CRED "ERROR: Read operation failed from %s: %d" CRESET "\n",
                 path, errcode);
          rc = RC_READ3FAILG;
          goto error_exit;
        }

      printf("LSM330 GYRO = (%6d, %6d, %6d), Temp=%d, Stat=0x%02X\n",
             pxyz->d[0], pxyz->d[1], pxyz->d[2], bfr[0] & 0xff, bfr[1]);

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
      printf(CGREEN "LSM330 gyroscope diagnostic completed successfully."
             CRESET "\n");
    }

  return rc;
}

/****************************************************************************
 * Name: lsm330spi_test_main
 *
 * Description:
 *   This program defaults to testing in interactive mode.
 *   To run in batch mode use a first parameter of b. E.g. lsm330spi_test -b
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int rc = 0;
  int step_rc = 0;
  uint8_t ch;
  int is_interactive = 1; /* program defaults to interactive */
  int flag_present = 0;   /* default to no flag coded */
  uint32_t ui;
  struct stat sbuf;

  /* The two arrays below must be synchronized */

  test_ptr_t test_ptr_array[] =
    {
      lsm330acl_test,   /* LSM330 accelerometer tests */
      lsm330gyro_test,  /* LSM330 gyroscope tests */
    };

  FAR char *test_path[ARRAYSIZE(test_ptr_array)];

  if (argc < 2 || *argv[1] == 0 || *(argv[1] + 1) == 0)
    {
      goto print_help;
    }

  /* We have at least 2 parameters, the first has at least two characters. */

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

  if (flag_present && argc < 3)
    {
      goto print_help;
    }

  test_path[0] = argv[1 + flag_present];
  test_path[1] = argv[2 + flag_present];

  printf("path_acl=%s\n", test_path[0]);
  printf("path_gyro=%s\n", test_path[1]);

  if (stat(test_path[0], &sbuf))
    {
      printf(CRED "Error: Accelerometer path not found. Path=%s" CRESET "\n",
             test_path[0]);
      rc = RC_INVALPATH;
    }

  if (stat(test_path[1], &sbuf))
    {
      printf(CRED "Error: Gyroscope path not found. Path=%s" CRESET "\n",
             test_path[1]);
      rc = RC_INVALPATH;
    }

  if (rc)
    {
      return rc;
    }

  if (is_interactive)
    {
      printf("LSM330 diagnostic started in interactive mode... \n");
      ch = 0;
      while (ch != 'x')
        {
          printf("Enter test section...\n");
          printf("  1 = SPI2 LSM330 ACL Tests.\n");
          printf("  2 = SPI2 LSM330 GYRO Tests.\n");
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
      printf("LSM330 sensor diagnostic started in batch mode...\n");
      for (ui = 0; ui < ARRAYSIZE(test_ptr_array); ui++)
        {
          step_rc = 0;
          if (test_ptr_array[ui] != 0)
            {
              /* Call the next test routine */

              step_rc = test_ptr_array[ui](is_interactive, test_path[ui]);
            }

          if (step_rc != 0 && rc == 0)
            {
              rc = step_rc;
            }

          sleep(1);
        }
    }

  printf("LSM330 sensor diagnostic completed.\n");
  return rc;

print_help:
  printf("Usage...\n");
  printf("lsm330spi_test [-b | -i] <dev_path_acl> <dev_path_gyro>\n");
  printf("    -b = batch mode execution\n");
  printf("    -i = interactive mode execution (default)\n");
  printf("    <dev_path_acl> = Device path for the LSM330 accelerometer\n");
  printf("    <dev_path_gyro> = Device path for the LSM330 gyroscope\n");
  printf(" Example:\n");
  printf("   lsm330spi_test -b /dev/lsm330acl0 /dev/lsm330gyro0\n");
  return RC_INVALPARM;
}
