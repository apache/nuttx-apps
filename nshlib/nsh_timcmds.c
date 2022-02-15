/****************************************************************************
 * apps/nshlib/nsh_timcmds.c
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

#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_TIME_STRING 80

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DATE
static FAR const char * const g_datemontab[] =
{
  "jan", "feb", "mar", "apr", "may", "jun",
  "jul", "aug", "sep", "oct", "nov", "dec"
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: date_month
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DATE
static inline int date_month(FAR const char *abbrev)
{
  int i;

  for (i = 0; i < 12; i++)
    {
      if (strncasecmp(g_datemontab[i], abbrev, 3) == 0)
        {
          return i;
        }
    }

  return ERROR;
}
#endif

/****************************************************************************
 * Name: date_gettime
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DATE
static inline int date_showtime(FAR struct nsh_vtbl_s *vtbl,
                                FAR const char *name, bool utc)
{
  struct timespec ts;
  struct tm tm;
  char timbuf[MAX_TIME_STRING];
  int ret;

  /* Get the current time */

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, name, "clock_gettime", NSH_ERRNO);
      return ERROR;
    }

  /* Break the current time up into the format needed by strftime */

  if (utc)
    {
      if (gmtime_r((FAR const time_t *)&ts.tv_sec, &tm) == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, name, "gmtime_r", NSH_ERRNO);
          return ERROR;
        }
    }
  else
    {
      if (localtime_r((FAR const time_t *)&ts.tv_sec, &tm) == NULL)
        {
          nsh_error(vtbl, g_fmtcmdfailed, name, "localtime_r", NSH_ERRNO);
          return ERROR;
        }
    }

  /* Show the current time in the requested format */

  ret = strftime(timbuf, MAX_TIME_STRING, "%a, %b %d %H:%M:%S %Y", &tm);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, name, "strftime", NSH_ERRNO);
      return ERROR;
    }

  nsh_output(vtbl, "%s\n", timbuf);
  return OK;
}
#endif

/****************************************************************************
 * Name: date_settime
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DATE
static inline int date_settime(FAR struct nsh_vtbl_s *vtbl,
                               FAR const char *name, bool utc,
                               FAR char *newtime)
{
  struct timespec ts;
  struct tm tm;
  FAR char *token;
  FAR char *saveptr;
  long result;
  int ret;

  memset(&tm, 0, sizeof(tm));

  /* Only this date format is supported: MMM DD HH:MM:SS YYYY */

  /* Get the month abbreviation */

  token = strtok_r(newtime, " \t", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  tm.tm_mon = date_month(token);
  if (tm.tm_mon < 0)
    {
      goto errout_bad_parm;
    }

  /* Get the day of the month.
   * NOTE: Accepts day-of-month up to 31 for all months
   */

  token = strtok_r(NULL, " \t", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  result = strtol(token, NULL, 10);
  if (result < 1 || result > 31)
    {
      goto errout_bad_parm;
    }

  tm.tm_mday = (int)result;

  /* Get the hours */

  token = strtok_r(NULL, " \t:", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  result = strtol(token, NULL, 10);
  if (result < 0 || result > 23)
    {
      goto errout_bad_parm;
    }

  tm.tm_hour = (int)result;

  /* Get the minutes */

  token = strtok_r(NULL, " \t:", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  result = strtol(token, NULL, 10);
  if (result < 0 || result > 59)
    {
      goto errout_bad_parm;
    }

  tm.tm_min = (int)result;

  /* Get the seconds */

  token = strtok_r(NULL, " \t:", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  result = strtol(token, NULL, 10);
  if (result < 0 || result > 61)
    {
      goto errout_bad_parm;
    }

  tm.tm_sec = (int)result;

  /* And finally the year */

  token = strtok_r(NULL, " \t", &saveptr);
  if (token == NULL)
    {
      goto errout_bad_parm;
    }

  result = strtol(token, NULL, 10);
  if (result < 1900 || result > 2100)
    {
      goto errout_bad_parm;
    }

  tm.tm_year = (int)result - 1900;

  /* Convert this to the right form, then set the timer */

  ts.tv_sec  = utc ? timegm(&tm): mktime(&tm);
  ts.tv_nsec = 0;

  ret = clock_settime(CLOCK_REALTIME, &ts);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, name, "clock_settime", NSH_ERRNO);
      return ERROR;
    }

  return OK;

errout_bad_parm:
  nsh_error(vtbl, g_fmtarginvalid, name);
  return ERROR;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_time
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_TIME
int cmd_time(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct timespec start;
#ifndef CONFIG_NSH_DISABLEBG
  bool bgsave;
#endif
#ifdef CONFIG_FILE_STREAM
  bool redirsave;
#endif
  int ret;

  /* Get the current time */

  ret = clock_gettime(CLOCK_MONOTONIC, &start);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "clock_gettime", NSH_ERRNO);
      return ERROR;
    }

  /* Save state */

#ifndef CONFIG_NSH_DISABLEBG
  bgsave    = vtbl->np.np_bg;
#endif
#ifdef CONFIG_FILE_STREAM
  redirsave = vtbl->np.np_redirect;
#endif

  /* Execute the command */

  ret = nsh_parse(vtbl, argv[1]);
  if (ret >= 0)
    {
      struct timespec end;
      struct timespec diff;

      /* Get and print the elapsed time */

      ret = clock_gettime(CLOCK_MONOTONIC, &end);
      if (ret < 0)
        {
           nsh_error(vtbl, g_fmtcmdfailed,
                     argv[0], "clock_gettime", NSH_ERRNO);
           ret = ERROR;
        }
      else
        {
          diff.tv_sec = end.tv_sec - start.tv_sec;
          if (start.tv_nsec > end.tv_nsec)
            {
              diff.tv_sec--;
              end.tv_nsec += 1000000000;
            }

          diff.tv_nsec = end.tv_nsec - start.tv_nsec;
          nsh_output(vtbl, "\n%lu.%04lu sec\n", (unsigned long)diff.tv_sec,
                     (unsigned long)diff.tv_nsec / 100000);
        }
    }

  /* Restore state */

#ifndef CONFIG_NSH_DISABLEBG
  vtbl->np.np_bg       = bgsave;
#endif
#ifdef CONFIG_FILE_STREAM
  vtbl->np.np_redirect = redirsave;
#endif

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_date
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DATE
int cmd_date(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *newtime = NULL;
  FAR const char *errfmt;
  bool utc = false;
  int option;
  int ret;

  /* Get the date options:  date [-s time] [+FORMAT] */

  while ((option = getopt(argc, argv, "s:u")) != ERROR)
    {
      if (option == 's')
        {
          /* We will be setting the time */

          newtime = optarg;
        }
      else if (option == 'u')
        {
          /* We will use the UTC time */

          utc = true;
        }
      else /* option = '?' */
        {
          errfmt = g_fmtarginvalid;
          goto errout;
        }
    }

  /* optind < argc-1 means that there are additional, unexpected arguments on
   * th command-line
   */

  if (optind < argc)
    {
      errfmt = g_fmttoomanyargs;
      goto errout;
    }

  /* Display or set the time */

  if (newtime)
    {
      ret = date_settime(vtbl, argv[0], utc, newtime);
    }
  else
    {
      ret = date_showtime(vtbl, argv[0], utc);
    }

  return ret;

errout:
  nsh_error(vtbl, errfmt, argv[0]);
  return ERROR;
}
#endif
