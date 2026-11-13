/****************************************************************************
 * apps/system/gnsspps/gnsspps_main.c
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

/* gnsspps - discipline a PTP clock (/dev/ptpX) to the PPS output of a GNSS
 * receiver.
 *
 * Every PPS edge is time stamped in hardware by the PTP clock driver and
 * read from /dev/ptpX as a struct ptp_extts_event.  The fractional part of
 * that time stamp is the phase error of the clock, and a PI servo steers
 * the clock frequency with clock_adjtime(ADJ_FREQUENCY) to drive it to
 * zero.  The whole seconds come from the NMEA sentences (RMC or ZDA) that
 * the receiver sends after each pulse.  Boards that run gnsspps with their
 * own GNSS receivers then share the same UTC time scale, to within the
 * accuracy of the receivers' PPS (typically some tens of nanoseconds).
 *
 * Optionally, CLOCK_REALTIME is kept aligned with the disciplined clock.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef CONFIG_SERIAL_TERMIOS
#  include <termios.h>
#endif

#include <nuttx/clock.h>
#include <nuttx/timers/ptp_clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NSEC_PER_SEC_LL     1000000000LL
#define HALF_SEC_NS         500000000LL

#define STATE_UNLOCKED      0   /* Waiting for the first sample */
#define STATE_ACQUIRE       1   /* Have one sample, estimating frequency */
#define STATE_LOCKED        2   /* PI servo running */

#define OUTLIER_LIMIT       4   /* Re-acquire after this many outliers */
#define NMEA_CONFIRM        2   /* Consistent NMEA seconds before a step */
#define PPS_TIMEOUT_MS      2500
#define NMEA_LINE_MAX       96
#define NMEA_MAX_FIELDS     20
#define STATS_INTERVAL      16

#define RT_SYS_SAMPLES      5

#ifndef CONFIG_SYSTEM_GNSSPPS_DEVPATH
#  define CONFIG_SYSTEM_GNSSPPS_DEVPATH "/dev/ptp0"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gnsspps_s
{
  /* Configuration */

  FAR const char *ptpdev;     /* PTP clock device */
  FAR const char *nmeadev;    /* NMEA serial device, or NULL */
  long            baud;       /* NMEA baud rate, 0 to keep */
  unsigned int    channel;    /* EXTTS channel wired to PPS */
  int64_t         delay_ns;   /* PPS delay: antenna cable + receiver */
  int64_t         step_ns;    /* Larger offsets are outliers when locked */
  int64_t         rtstep_ns;  /* CLOCK_REALTIME step threshold */
  double          kp;         /* Proportional gain */
  double          ki;         /* Integral gain */
  long            count;      /* Exit after this many pulses, 0: never */
  bool            follow_rt;  /* Keep CLOCK_REALTIME in sync */
  bool            verbose;    /* Print every sample */

  /* Devices */

  int             ptpfd;
  int             nmeafd;
  clockid_t       clkid;
  long            max_adj;    /* Frequency limit of the clock (ppb) */

  /* Servo */

  int             state;
  int             outliers;
  double          freq;       /* Applied frequency offset (ppb) */
  double          integral;   /* Integral term (ppb) */
  int64_t         acq_offset; /* First sample of the acquisition */
  int64_t         acq_sec;
  int64_t         last_sec;   /* Second of the last pulse */

  /* Time of day from NMEA */

  struct timespec pulse_mono; /* When the last pulse was processed */
  bool            have_pulse;
  int64_t         pulse_sec;  /* Second of the last pulse, PTP scale */
  int64_t         nmea_last;  /* Last NMEA second, to ignore repeats */
  int64_t         nmea_err;   /* Pending whole second error */
  int             nmea_cnt;   /* Consecutive identical errors */
  bool            tod_ok;     /* Seconds confirmed by NMEA */
  char            line[NMEA_LINE_MAX];
  size_t          linelen;

  /* Statistics */

  unsigned long   pulses;
  unsigned long   nstats;
  int64_t         stat_max;
  double          stat_sq;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile bool g_gnsspps_exit;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void gnsspps_sighandler(int signo)
{
  g_gnsspps_exit = true;
}

static void gnsspps_usage(FAR const char *progname)
{
  fprintf(stderr,
          "Usage: %s [options]\n"
          "  -d <dev>   PTP clock with the PPS input (default %s)\n"
          "  -c <n>     External time stamp channel of the PPS (default 0)\n"
          "  -s <tty>   NMEA serial port of the GNSS receiver, for the\n"
          "             time of day.  Without it only the phase is "
          "locked\n"
          "  -b <baud>  Baud rate of the NMEA serial port\n"
          "  -D <ns>    PPS delay to compensate (cable, receiver), ns\n"
          "  -P <kp>    Proportional gain (default 0.7)\n"
          "  -I <ki>    Integral gain (default 0.3)\n"
          "  -S <ns>    Outlier/step threshold when locked (default "
          "20000)\n"
          "  -r         Keep CLOCK_REALTIME synchronized to the clock\n"
          "  -R <ns>    Step CLOCK_REALTIME above this error (default "
          "1000000)\n"
          "  -n <n>     Exit after n pulses\n"
          "  -v         Print every sample\n",
          progname, CONFIG_SYSTEM_GNSSPPS_DEVPATH);
}

static int64_t ts2ns(FAR const struct timespec *ts)
{
  return (int64_t)ts->tv_sec * NSEC_PER_SEC_LL + ts->tv_nsec;
}

static int64_t pct2ns(FAR const struct ptp_clock_time *t)
{
  return t->sec * NSEC_PER_SEC_LL + t->nsec;
}

static int64_t mono_ms_since(FAR const struct timespec *then)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  return (ts2ns(&now) - ts2ns(then)) / 1000000;
}

/****************************************************************************
 * Clock control
 ****************************************************************************/

static int gnsspps_setfreq(FAR struct gnsspps_s *g, double ppb)
{
  struct timex tx;

  if (ppb > g->max_adj)
    {
      ppb = g->max_adj;
    }
  else if (ppb < -g->max_adj)
    {
      ppb = -g->max_adj;
    }

  memset(&tx, 0, sizeof(tx));
  tx.modes = ADJ_FREQUENCY;
  tx.freq  = (long)(ppb * 65.536);  /* ppb to ppm with a 16 bit fraction */

  if (clock_adjtime(g->clkid, &tx) < 0)
    {
      fprintf(stderr, "gnsspps: ADJ_FREQUENCY failed: %d\n", errno);
      return -errno;
    }

  g->freq = ppb;
  return 0;
}

static int gnsspps_step(FAR struct gnsspps_s *g, int64_t delta_ns)
{
  struct timex tx;
  int64_t sec = delta_ns / NSEC_PER_SEC_LL;
  int64_t nsec = delta_ns - sec * NSEC_PER_SEC_LL;

  if (nsec < 0)
    {
      nsec += NSEC_PER_SEC_LL;
      sec--;
    }

  memset(&tx, 0, sizeof(tx));
  tx.modes        = ADJ_SETOFFSET | ADJ_NANO;
  tx.time.tv_sec  = (time_t)sec;
  tx.time.tv_usec = (long)nsec;

  if (clock_adjtime(g->clkid, &tx) < 0)
    {
      fprintf(stderr, "gnsspps: ADJ_SETOFFSET failed: %d\n", errno);
      return -errno;
    }

  return 0;
}

/* Discard the time stamps that were taken before a step */

static void gnsspps_drain(FAR struct gnsspps_s *g)
{
  struct ptp_extts_event ev[4];
  int flags = fcntl(g->ptpfd, F_GETFL);

  fcntl(g->ptpfd, F_SETFL, flags | O_NONBLOCK);
  while (read(g->ptpfd, ev, sizeof(ev)) > 0)
    {
    }

  fcntl(g->ptpfd, F_SETFL, flags);
}

/****************************************************************************
 * CLOCK_REALTIME following
 ****************************************************************************/

/* Offset of the PTP clock from CLOCK_REALTIME (ns), measured with the
 * smallest possible uncertainty.
 */

static int gnsspps_rtoffset(FAR struct gnsspps_s *g, FAR int64_t *offset)
{
  struct ptp_sys_offset_precise prec;
  struct ptp_sys_offset sys;
  int64_t best = INT64_MAX;
  int i;

  if (ioctl(g->ptpfd, PTP_SYS_OFFSET_PRECISE,
            (unsigned long)(uintptr_t)&prec) == 0)
    {
      *offset = pct2ns(&prec.device) - pct2ns(&prec.sys_realtime);
      return 0;
    }

  memset(&sys, 0, sizeof(sys));
  sys.n_samples = RT_SYS_SAMPLES;
  if (ioctl(g->ptpfd, PTP_SYS_OFFSET,
            (unsigned long)(uintptr_t)&sys) < 0)
    {
      return -errno;
    }

  for (i = 0; i < RT_SYS_SAMPLES; i++)
    {
      int64_t t1 = pct2ns(&sys.ts[2 * i]);
      int64_t tp = pct2ns(&sys.ts[2 * i + 1]);
      int64_t t2 = pct2ns(&sys.ts[2 * i + 2]);

      if (t2 - t1 < best)
        {
          best    = t2 - t1;
          *offset = tp - (t1 + (t2 - t1) / 2);
        }
    }

  return 0;
}

static void gnsspps_follow_rt(FAR struct gnsspps_s *g)
{
  int64_t offset = 0;
  int64_t absoff;

  if (gnsspps_rtoffset(g, &offset) < 0)
    {
      return;
    }

  if (g->verbose)
    {
      printf("gnsspps: CLOCK_REALTIME offset %+" PRId64 " ns\n", -offset);
    }

  absoff = offset < 0 ? -offset : offset;
  if (absoff > g->rtstep_ns)
    {
      struct timespec now;
      int64_t ns;

      clock_gettime(CLOCK_REALTIME, &now);
      ns = ts2ns(&now) + offset;
      now.tv_sec  = (time_t)(ns / NSEC_PER_SEC_LL);
      now.tv_nsec = (long)(ns % NSEC_PER_SEC_LL);
      clock_settime(CLOCK_REALTIME, &now);

      if (g->verbose)
        {
          printf("gnsspps: CLOCK_REALTIME stepped by %+" PRId64 " ns\n",
                 offset);
        }
    }
#ifdef CONFIG_CLOCK_ADJTIME
  else
    {
      struct timeval delta;

      delta.tv_sec  = (time_t)(offset / 1000000000);
      delta.tv_usec = (long)((offset % 1000000000) / 1000);
      adjtime(&delta, NULL);
    }
#endif
}

/****************************************************************************
 * Servo
 ****************************************************************************/

static void gnsspps_stats(FAR struct gnsspps_s *g, int64_t offset)
{
  int64_t absoff = offset < 0 ? -offset : offset;

  if (absoff > g->stat_max)
    {
      g->stat_max = absoff;
    }

  g->stat_sq += (double)offset * (double)offset;
  if (++g->nstats >= STATS_INTERVAL)
    {
      double rms = g->stat_sq / g->nstats;
      double x = rms;
      int i;

      /* Square root by Newton iterations, avoids libm */

      for (i = 0; i < 30 && x > 0; i++)
        {
          x = 0.5 * (x + rms / x);
        }

      printf("gnsspps: rms %" PRId64 " ns max %" PRId64
             " ns freq %+" PRId64 " ppb%s\n",
             (int64_t)x, g->stat_max, (int64_t)g->freq,
             g->nmeafd >= 0 && !g->tod_ok ? " (time of day unknown)" : "");

      g->nstats   = 0;
      g->stat_max = 0;
      g->stat_sq  = 0;
    }
}

/* Process one PPS time stamp.  offset > 0 means that the clock is ahead */

static void gnsspps_sample(FAR struct gnsspps_s *g, int64_t pulse_ns)
{
  int64_t sec = pulse_ns / NSEC_PER_SEC_LL;
  int64_t frac = pulse_ns - sec * NSEC_PER_SEC_LL;
  int64_t offset;
  int64_t absoff;
  int64_t interval;
  double ppb;

  /* The pulse marks a whole second: the phase error is the distance to
   * the nearest second, in (-0.5 s, +0.5 s].
   */

  if (frac < 0)
    {
      frac += NSEC_PER_SEC_LL;
      sec--;
    }

  if (frac > HALF_SEC_NS)
    {
      offset = frac - NSEC_PER_SEC_LL;
      sec++;
    }
  else
    {
      offset = frac;
    }

  absoff = offset < 0 ? -offset : offset;
  interval = sec - g->last_sec;
  g->last_sec = sec;

  clock_gettime(CLOCK_MONOTONIC, &g->pulse_mono);
  g->pulse_sec  = sec;
  g->have_pulse = true;
  g->pulses++;

  switch (g->state)
    {
      case STATE_UNLOCKED:
        g->acq_offset = offset;
        g->acq_sec    = sec;
        g->state      = STATE_ACQUIRE;
        if (g->verbose)
          {
            printf("gnsspps: offset %+" PRId64 " ns, acquiring\n", offset);
          }
        break;

      case STATE_ACQUIRE:
        interval = sec - g->acq_sec;
        if (interval < 1 || interval > 4)
          {
            g->acq_offset = offset;
            g->acq_sec    = sec;
            break;
          }

        /* Frequency error of the clock: phase change per second */

        ppb = g->freq - (double)(offset - g->acq_offset) / interval;
        gnsspps_setfreq(g, ppb);
        g->integral = g->freq;

        /* Remove the phase error with one step */

        gnsspps_step(g, -offset);
        gnsspps_drain(g);
        g->state    = STATE_LOCKED;
        g->outliers = 0;

        printf("gnsspps: locked, offset %+" PRId64 " ns stepped, "
               "freq %+" PRId64 " ppb\n", offset, (int64_t)g->freq);
        break;

      case STATE_LOCKED:
        if (absoff > g->step_ns)
          {
            if (++g->outliers >= OUTLIER_LIMIT)
              {
                printf("gnsspps: offset %+" PRId64 " ns, lost lock\n",
                       offset);
                g->state = STATE_UNLOCKED;
              }
            else if (g->verbose)
              {
                printf("gnsspps: outlier %+" PRId64 " ns ignored\n",
                       offset);
              }

            break;
          }

        if (interval < 1 || interval > 16)
          {
            interval = 1;
          }

        g->outliers  = 0;
        ppb          = g->integral - (g->kp + g->ki * interval) * offset;
        g->integral -= g->ki * interval * offset;

        if (g->integral > g->max_adj)
          {
            g->integral = g->max_adj;
          }
        else if (g->integral < -g->max_adj)
          {
            g->integral = -g->max_adj;
          }

        gnsspps_setfreq(g, ppb);

        if (g->verbose)
          {
            printf("gnsspps: offset %+6" PRId64 " ns freq %+" PRId64
                   " ppb\n", offset, (int64_t)g->freq);
          }

        gnsspps_stats(g, offset);

        if (g->follow_rt)
          {
            gnsspps_follow_rt(g);
          }
        break;
    }
}

/****************************************************************************
 * NMEA time of day
 ****************************************************************************/

static int gnsspps_hex(char c)
{
  if (c >= '0' && c <= '9')
    {
      return c - '0';
    }

  if (c >= 'A' && c <= 'F')
    {
      return c - 'A' + 10;
    }

  if (c >= 'a' && c <= 'f')
    {
      return c - 'a' + 10;
    }

  return -1;
}

static int gnsspps_num(FAR const char *s, int n)
{
  int v = 0;

  while (n-- > 0)
    {
      if (*s < '0' || *s > '9')
        {
          return -1;
        }

      v = v * 10 + (*s++ - '0');
    }

  return v;
}

/* Parse an RMC or ZDA sentence into UTC seconds.  Returns false if the
 * sentence carries no valid time.
 */

static bool gnsspps_nmea_time(FAR char *line, FAR time_t *utc)
{
  FAR char *field[NMEA_MAX_FIELDS];
  FAR char *star;
  FAR char *p;
  uint8_t sum = 0;
  int nfields = 0;
  struct tm tm;
  int hi;
  int lo;

  if (line[0] != '$' || (star = strchr(line, '*')) == NULL)
    {
      return false;
    }

  for (p = line + 1; p < star; p++)
    {
      sum ^= (uint8_t)*p;
    }

  hi = gnsspps_hex(star[1]);
  lo = gnsspps_hex(star[2]);
  if (hi < 0 || lo < 0 || sum != (uint8_t)((hi << 4) | lo))
    {
      return false;
    }

  *star = '\0';

  /* Split in fields, keeping the empty ones */

  for (p = line; p != NULL && nfields < NMEA_MAX_FIELDS; )
    {
      field[nfields++] = p;
      p = strchr(p, ',');
      if (p != NULL)
        {
          *p++ = '\0';
        }
    }

  if (strlen(field[0]) != 6 || strlen(field[1]) < 6)
    {
      return false;
    }

  memset(&tm, 0, sizeof(tm));
  tm.tm_hour = gnsspps_num(field[1], 2);
  tm.tm_min  = gnsspps_num(field[1] + 2, 2);
  tm.tm_sec  = gnsspps_num(field[1] + 4, 2);

  if (strcmp(field[0] + 3, "RMC") == 0)
    {
      /* $xxRMC,hhmmss.ss,A,lat,N,lon,E,spd,cog,ddmmyy,... */

      if (nfields < 10 || field[2][0] != 'A' || strlen(field[9]) != 6)
        {
          return false;
        }

      tm.tm_mday = gnsspps_num(field[9], 2);
      tm.tm_mon  = gnsspps_num(field[9] + 2, 2) - 1;
      tm.tm_year = gnsspps_num(field[9] + 4, 2) + 100;
    }
  else if (strcmp(field[0] + 3, "ZDA") == 0)
    {
      /* $xxZDA,hhmmss.ss,dd,mm,yyyy,zh,zm */

      if (nfields < 5 || strlen(field[4]) != 4)
        {
          return false;
        }

      tm.tm_mday = gnsspps_num(field[2], 2);
      tm.tm_mon  = gnsspps_num(field[3], 2) - 1;
      tm.tm_year = gnsspps_num(field[4], 4) - 1900;
    }
  else
    {
      return false;
    }

  if (tm.tm_hour < 0 || tm.tm_min < 0 || tm.tm_sec < 0 ||
      tm.tm_mday < 1 || tm.tm_mon < 0 || tm.tm_year < 100)
    {
      return false;
    }

  *utc = timegm(&tm);
  return *utc != (time_t)-1;
}

/* The receiver sends the time of a pulse shortly after the pulse.  Compare
 * it with the second the clock gave to that pulse and fix whole seconds.
 */

static void gnsspps_nmea_line(FAR struct gnsspps_s *g)
{
  time_t utc;
  int64_t err;

  if (!gnsspps_nmea_time(g->line, &utc) || (int64_t)utc == g->nmea_last)
    {
      return;
    }

  g->nmea_last = utc;

  if (!g->have_pulse || mono_ms_since(&g->pulse_mono) >= 1000)
    {
      return;
    }

  err = (int64_t)utc - g->pulse_sec;
  if (err == 0)
    {
      if (!g->tod_ok)
        {
          printf("gnsspps: time of day confirmed by NMEA\n");
        }

      g->tod_ok   = true;
      g->nmea_cnt = 0;
      return;
    }

  if (err == g->nmea_err)
    {
      g->nmea_cnt++;
    }
  else
    {
      g->nmea_err = err;
      g->nmea_cnt = 1;
    }

  if (g->nmea_cnt >= NMEA_CONFIRM)
    {
      printf("gnsspps: stepping %+" PRId64 " s to NMEA time\n", err);
      if (gnsspps_step(g, err * NSEC_PER_SEC_LL) == 0)
        {
          gnsspps_drain(g);
          g->pulse_sec += err;
          g->last_sec  += err;
          g->acq_sec   += err;
          g->tod_ok     = true;
        }

      g->nmea_cnt = 0;
    }
}

static void gnsspps_nmea_read(FAR struct gnsspps_s *g)
{
  char buf[64];
  ssize_t n;
  ssize_t i;

  n = read(g->nmeafd, buf, sizeof(buf));
  for (i = 0; i < n; i++)
    {
      char c = buf[i];

      if (c == '$')
        {
          g->linelen = 0;
        }

      if (c == '\r' || c == '\n')
        {
          if (g->linelen > 0)
            {
              g->line[g->linelen] = '\0';
              gnsspps_nmea_line(g);
              g->linelen = 0;
            }
        }
      else if (g->linelen < sizeof(g->line) - 1)
        {
          g->line[g->linelen++] = c;
        }
      else
        {
          g->linelen = 0;
        }
    }
}

static int gnsspps_nmea_open(FAR struct gnsspps_s *g)
{
  g->nmeafd = open(g->nmeadev, O_RDONLY | O_NONBLOCK | O_NOCTTY);
  if (g->nmeafd < 0)
    {
      fprintf(stderr, "gnsspps: cannot open %s: %d\n", g->nmeadev, errno);
      return -errno;
    }

#ifdef CONFIG_SERIAL_TERMIOS
  if (g->baud > 0)
    {
      struct termios tio;

      if (tcgetattr(g->nmeafd, &tio) == 0)
        {
          cfmakeraw(&tio);
          cfsetspeed(&tio, g->baud);
          tcsetattr(g->nmeafd, TCSANOW, &tio);
        }
    }
#endif

  return 0;
}

/****************************************************************************
 * Setup
 ****************************************************************************/

static int gnsspps_open(FAR struct gnsspps_s *g)
{
  struct ptp_extts_request req;
  struct ptp_clock_caps caps;
  struct timespec ts;

  g->ptpfd = open(g->ptpdev, O_RDWR | O_CLOEXEC);
  if (g->ptpfd < 0)
    {
      fprintf(stderr, "gnsspps: cannot open %s: %d\n", g->ptpdev, errno);
      return -errno;
    }

  g->clkid = (g->ptpfd << CLOCK_SHIFT) | CLOCK_FD;

  memset(&caps, 0, sizeof(caps));
  if (ioctl(g->ptpfd, PTP_CLOCK_GETCAPS,
            (unsigned long)(uintptr_t)&caps) < 0)
    {
      fprintf(stderr, "gnsspps: PTP_CLOCK_GETCAPS failed: %d\n", errno);
      return -errno;
    }

  if (g->channel >= (unsigned int)caps.n_ext_ts)
    {
      fprintf(stderr, "gnsspps: %s has %d external time stamp inputs\n",
              g->ptpdev, caps.n_ext_ts);
      return -EINVAL;
    }

  g->max_adj = caps.max_adj > 0 ? caps.max_adj : 500000;

  /* A clock that was never set starts near zero.  Start from the system
   * time; NMEA will fix the seconds later if it is available.
   */

  if (clock_gettime(g->clkid, &ts) == 0 && ts.tv_sec < 365 * 86400)
    {
      clock_gettime(CLOCK_REALTIME, &ts);
      clock_settime(g->clkid, &ts);
    }

  gnsspps_setfreq(g, 0);

  memset(&req, 0, sizeof(req));
  req.index = g->channel;
  req.flags = PTP_ENABLE_FEATURE | PTP_RISING_EDGE;
  if (ioctl(g->ptpfd, PTP_EXTTS_REQUEST,
            (unsigned long)(uintptr_t)&req) < 0)
    {
      fprintf(stderr, "gnsspps: PTP_EXTTS_REQUEST failed: %d\n", errno);
      return -errno;
    }

  return 0;
}

static void gnsspps_close(FAR struct gnsspps_s *g)
{
  if (g->ptpfd >= 0)
    {
      struct ptp_extts_request req;

      memset(&req, 0, sizeof(req));
      req.index = g->channel;
      ioctl(g->ptpfd, PTP_EXTTS_REQUEST, (unsigned long)(uintptr_t)&req);
      close(g->ptpfd);
    }

  if (g->nmeafd >= 0)
    {
      close(g->nmeafd);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct gnsspps_s g;
  struct pollfd pfd[2];
  struct timespec last_pps;
  int nfds;
  int opt;
  int ret;

  memset(&g, 0, sizeof(g));
  g.ptpdev    = CONFIG_SYSTEM_GNSSPPS_DEVPATH;
  g.ptpfd     = -1;
  g.nmeafd    = -1;
  g.kp        = 0.7;
  g.ki        = 0.3;
  g.step_ns   = 20000;
  g.rtstep_ns = 1000000;
  g.nmea_last = -1;

  while ((opt = getopt(argc, argv, "d:c:s:b:D:P:I:S:rR:n:vh")) != -1)
    {
      switch (opt)
        {
          case 'd':
            g.ptpdev = optarg;
            break;

          case 'c':
            g.channel = (unsigned int)strtoul(optarg, NULL, 0);
            break;

          case 's':
            g.nmeadev = optarg;
            break;

          case 'b':
            g.baud = strtol(optarg, NULL, 0);
            break;

          case 'D':
            g.delay_ns = strtoll(optarg, NULL, 0);
            break;

          case 'P':
            g.kp = strtod(optarg, NULL);
            break;

          case 'I':
            g.ki = strtod(optarg, NULL);
            break;

          case 'S':
            g.step_ns = strtoll(optarg, NULL, 0);
            break;

          case 'r':
            g.follow_rt = true;
            break;

          case 'R':
            g.rtstep_ns = strtoll(optarg, NULL, 0);
            break;

          case 'n':
            g.count = strtol(optarg, NULL, 0);
            break;

          case 'v':
            g.verbose = true;
            break;

          default:
            gnsspps_usage(argv[0]);
            return opt == 'h' ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

  g_gnsspps_exit = false;
  signal(SIGINT, gnsspps_sighandler);
  signal(SIGTERM, gnsspps_sighandler);

  ret = gnsspps_open(&g);
  if (ret == 0 && g.nmeadev != NULL)
    {
      ret = gnsspps_nmea_open(&g);
    }

  if (ret < 0)
    {
      gnsspps_close(&g);
      return EXIT_FAILURE;
    }

  printf("gnsspps: %s channel %u%s%s\n", g.ptpdev, g.channel,
         g.nmeadev ? ", NMEA from " : ", no time of day source",
         g.nmeadev ? g.nmeadev : "");

  clock_gettime(CLOCK_MONOTONIC, &last_pps);

  while (!g_gnsspps_exit && (g.count == 0 || (long)g.pulses < g.count))
    {
      pfd[0].fd      = g.ptpfd;
      pfd[0].events  = POLLIN;
      pfd[0].revents = 0;
      nfds           = 1;

      if (g.nmeafd >= 0)
        {
          pfd[1].fd      = g.nmeafd;
          pfd[1].events  = POLLIN;
          pfd[1].revents = 0;
          nfds           = 2;
        }

      ret = poll(pfd, nfds, PPS_TIMEOUT_MS);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          fprintf(stderr, "gnsspps: poll failed: %d\n", errno);
          break;
        }

      if (pfd[0].revents & POLLIN)
        {
          struct ptp_extts_event ev[4];
          ssize_t n;
          int i;

          n = read(g.ptpfd, ev, sizeof(ev));
          for (i = 0; i < n / (ssize_t)sizeof(ev[0]); i++)
            {
              if (ev[i].index == g.channel)
                {
                  /* A delayed pulse is captured late: remove the delay */

                  gnsspps_sample(&g, pct2ns(&ev[i].t) - g.delay_ns);
                  clock_gettime(CLOCK_MONOTONIC, &last_pps);
                }
            }
        }

      if (nfds > 1 && (pfd[1].revents & POLLIN))
        {
          gnsspps_nmea_read(&g);
        }

      if (mono_ms_since(&last_pps) > PPS_TIMEOUT_MS)
        {
          /* Holdover: keep the last frequency until the PPS returns */

          if (g.state != STATE_UNLOCKED)
            {
              printf("gnsspps: no PPS, holding frequency %+" PRId64
                     " ppb\n", (int64_t)g.freq);
            }

          g.state = g.state == STATE_LOCKED ? STATE_UNLOCKED : g.state;
          clock_gettime(CLOCK_MONOTONIC, &last_pps);
        }
    }

  gnsspps_close(&g);
  return EXIT_SUCCESS;
}
