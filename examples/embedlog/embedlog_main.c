/****************************************************************************
 * apps/examples/embedlog/embedlog_main.c
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

#include <system/embedlog.h>
#include <errno.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/****************************************************************************
 * Preprocessor macros
 ****************************************************************************/

/* Macro used by OEL* macros, so we don't have to specify &g_el every time
 * we want to print something.
 */

#define EL_OPTIONS_OBJECT &g_el

/****************************************************************************
 * Global variables
 ****************************************************************************/

/* Logger state, in flat build you need to create embedlog object and use it
 * with el_o* functions. It is so, because embedlog defines one global object
 * to make it easier to use embedlog on systems with MMU where each thread
 * has it's own global memory. In nuttx in flat build that single object is
 * shared between *all* tasks. You could probably get away with this if you
 * use embedlog only in one task, but it's usually not the case. So in nuttx
 * always create embedlog object. You can define EL_OPTIONS_OBJECT macro and
 * use OEL* macros to make it almost as easy as using embedlog with global
 * object. see el_options(3) to see learn more about it.
 */

static struct el g_el;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: el_print_options
 *
 * Description:
 *   Presents how to use options and how it's rendered. This assumes all
 *   options are enabled in compile time. If any of the options isn't enabled
 *   in compile time, setting it will have no effect and logs will vary.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void el_print_options(void)
{
  /* We will be printing logs to standard error output only */

  el_ooption(&g_el, EL_OUT, EL_OUT_STDERR);

  el_ooption(&g_el, EL_PRINT_LEVEL, 0);
  el_oprint(OELI, "We can disable information about log level");
  el_oprint(OELF, "message still will be filtered by log level");
  el_oprint(OELA, "but there is no way to tell what level message is");
  el_oprint(OELD, "like this message will not be printed");

  el_ooption(&g_el, EL_TS, EL_TS_SHORT);
  el_oprint(OELF, "As every respected logger, we also have timestamps");
  el_oprint(OELF, "which work well with time from time()");
  el_ooption(&g_el, EL_TS_TM, EL_TS_TM_MONOTONIC);
  el_oprint(OELF, "or CLOCK_MONOTONIC from POSIX");
  el_ooption(&g_el, EL_TS, EL_TS_LONG);
  el_ooption(&g_el, EL_TS_TM, EL_TS_TM_TIME);
  el_oprint(OELF, "we also have long format that works well with time()");
  el_ooption(&g_el, EL_TS_TM, EL_TS_TM_REALTIME);
  el_oprint(OELF, "if higher precision is needed we can use CLOCK_REALTIME");
  el_ooption(&g_el, EL_TS, EL_TS_SHORT);
  el_oprint(OELF, "we can also mix REALTIME with short format");
  el_ooption(&g_el, EL_TS_FRACT, EL_TS_FRACT_OFF);
  el_oprint(OELF, "and if you don't need high resolution");
  el_oprint(OELF, "you can disable fractions of seconds to save space!");
  el_ooption(&g_el, EL_TS_FRACT, EL_TS_FRACT_MS);
  el_oprint(OELF, "or enable only millisecond resolution");
  el_ooption(&g_el, EL_TS_FRACT, EL_TS_FRACT_US);
  el_oprint(OELF, "or enable only microsecond resolution");
  el_ooption(&g_el, EL_TS_FRACT, EL_TS_FRACT_NS);
  el_oprint(OELF, "or enable only nanosecond resolution "
      "(not that it makes much sense on nuttx)");
  el_ooption(&g_el, EL_TS, EL_TS_LONG);
  el_ooption(&g_el, EL_TS, EL_TS_OFF);
  el_oprint(OELF, "no time information, if your heart desire it");
  el_ooption(&g_el, EL_TS_FRACT, EL_TS_FRACT_NS);

  el_ooption(&g_el, EL_FINFO, 1);
  el_oprint(OELF, "log location is very useful for debugging");

  el_ooption(&g_el, EL_TS, EL_TS_LONG);
  el_ooption(&g_el, EL_TS_TM, EL_TS_TM_REALTIME);
  el_ooption(&g_el, EL_PRINT_LEVEL, 1);
  el_oprint(OELF, "Different scenarios need different options");
  el_oprint(OELA, "So we can mix options however we want");

  el_ooption(&g_el, EL_PRINT_NL, 0);
  el_oprint(OELF, "you can also remove printing new line ");
  el_oputs(&g_el, "to join el_oprint and el_oputs in a single ");
  el_oputs(&g_el, "long line as needed\n");
  el_ooption(&g_el, EL_PRINT_NL, 1);

  el_ooption(&g_el, EL_COLORS, 1);
  el_ooption(&g_el, EL_LEVEL, EL_DBG);
  el_oprint(OELD, "And if we have");
  el_oprint(OELI, "modern terminal");
  el_oprint(OELN, "we can enable colors");
  el_oprint(OELW, "to spot warnings");
  el_oprint(OELE, "or errors");
  el_oprint(OELC, "with a quick");
  el_oprint(OELA, "glance into");
  el_oprint(OELF, "log file");

  el_ooption(&g_el, EL_PREFIX, "embedlog: ");
  el_oprint(OELI, "you can also use prefixes");
  el_oprint(OELI, "to every message you send");

  el_ooption(&g_el, EL_PREFIX, NULL);
  el_oprint(OELI, "set prefix to null to disable it");
}

/****************************************************************************
 * Name: el_print_memory
 *
 * Description:
 *   Presents how to print blob of memory.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void el_print_memory(void)
{
  char s[] = "some message\0that contains\0null characters";
  char ascii[128];
  int i;

  for (i = 0; i != 128; ++i)
    {
      ascii[i] = (char)i;
    }

  /* We will be printing logs to standard error output only */

  el_ooption(&g_el, EL_OUT, EL_OUT_STDERR);

  el_oprint(OELI, "print whole ASCII table");
  el_opmemory(OELI, ascii, sizeof(ascii));

  el_oprint(OELI, "print memory region that contains string with NULL "
                  "chars");
  el_opmemory(OELI, s, sizeof(s));

  el_oprint(OELI, "print the same region but this time with nice ascii "
                  "table");
  el_opmemory_table(OELI, s, sizeof(s));
}

/****************************************************************************
 * Name: el_print_file
 *
 * Description:
 *   Presents how to print data to file with log rotation
 *
 * Input Parameters:
 *   workdir - directory where logs will be stored.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void el_print_file(const char *workdir)
{
  char log_path[PATH_MAX];

  /* create user specified directory if it doesn't exist */

  if (mkdir(workdir, 0755) != 0 && errno != EEXIST)
    {
      fprintf(stderr, "Couldn't create %s, error %s\n",
              workdir, strerror(errno));
      return;
    }

  /* Create full path to log file embedlog will use */

  sprintf(log_path, "%s/log-rotate", workdir);

  /* Enable file rotation, maximum 5 files will be created, none of the log
   * files size shall exceed 512 bytes. Rotate size is low to present how
   * rotation works, in real life this should be much higher, unless you
   * really know what you are doing and understand the consequences of low
   * rotate size.
   */

  el_ooption(&g_el, EL_FROTATE_NUMBER, 5);
  el_ooption(&g_el, EL_FROTATE_SIZE, 512);

  /* Tell embedlog to sync all buffers and metadata regarding log file.
   * There are cases, when data (a lot of data) goes into block device,
   * but metadata of the file are not updated when power loss occurs. This
   * leads to data loss - even though they landed physically on block device
   * like sdcard. To prevent losing too much data, you can define how often
   * you want embedlog to call sync() functions. It basically translates to
   * "how much data am I ready to loose?". In this example, we sync() once
   * every 4096 bytes are stored into file.
   */

  el_ooption(&g_el, EL_FILE_SYNC_EVERY, 4096);

  /* Setting above value to too small value, will cause sync() to be called
   * very often (depending on how much data you log) and that may criple
   * performance greatly. And there are situations when you are willing to
   * loose some info or debug data (up to configured sync every value), but
   * when critical error shows up, you want data to be synced immediately to
   * minimize chance of losing information about critical error. For that
   * you can define which log level will be synced *every* time regardless
   * of sync every value. Here we will instruct embedlog to log prints that
   * have severity higher or equal to critical (so critial, alert and fatal)
   */

  el_ooption(&g_el, EL_FILE_SYNC_LEVEL, EL_CRIT);

  /* Print logs to both file and standard error */

  el_ooption(&g_el, EL_OUT, EL_OUT_FILE | EL_OUT_STDERR);

  /* Register path to log file in embedlog. This will cause logger to look
   * for next valid log file (when log rotate in enabled) and will try to
   * open file for reading.
   */

  if (el_ooption(&g_el, EL_FPATH, log_path) != 0)
    {
      if (errno == ENAMETOOLONG || errno == EINVAL)
        {
          /* These are the only unrecoverable errors embedlog may return,
           * any other error it actually a warning, telling user that file
           * could not have been opened now, but every el_print function with
           * output to file enabled, will try to reopen file. This of course
           * apply to situation when problem is temporary - like directory
           * does not exist but will be created after warning, or file has
           * no write permission but it gets fixed later.
           */

            fprintf(stderr, "log file name too long");
            return;
        }

      fprintf(stderr, "WARNING: couldn't open file: %s\n", strerror(errno));
      return;
    }

  /* Now we can print messages */

  el_oprint(OELI, "Now we enabled log rotation");
  el_oprint(OELI, "If log cannot fit in current file");
  el_oprint(OELI, "it will be stored in new file and");
  el_oprint(OELI, "if library creates EL_FROTATE_NUMBER files oldest");
  el_oprint(OELI, "file will be deleted and new file will be created");
  el_oprint(OELI, "run this program multiple times and see how it works");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * embedlog_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc > 2 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
      fprintf(stderr, "usage: %s [<log-directory>]\n", argv[0]);
      return 1;
    }

  /* Initialize embedlog object to sane and known values */

  el_oinit(&g_el);

  el_oprint(OELI, "Right after init, embedlog will print to stderr with "
      "just log level information - these are default settings.");

  el_print_options();
  el_print_memory();

  if (argc == 2)
    {
      el_print_file(argv[1]);
    }

  /* This will cleanup whatever has been allocated by el_oinit(), like opened
   * file descriptors
   */

  el_ocleanup(&g_el);

  return 0;
}
