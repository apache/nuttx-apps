/****************************************************************************
 * apps/include/gpsutils/minmea.h
 *
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 *
 * Released under the NuttX BSD license with permission from the author:
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

#ifndef __APPS_INCLUDE_GPSUTILS_MINMEA_H
#define __APPS_INCLUDE_GPSUTILS_MINMEA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <math.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MINMEA_MAX_LENGTH 80

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum minmea_sentence_id
{
  MINMEA_INVALID = -1,
  MINMEA_UNKNOWN = 0,
  MINMEA_SENTENCE_RMC,
  MINMEA_SENTENCE_GGA,
  MINMEA_SENTENCE_GSA,
  MINMEA_SENTENCE_GLL,
  MINMEA_SENTENCE_GST,
  MINMEA_SENTENCE_GSV,
};

struct minmea_float
{
  int_least32_t value;
  int_least32_t scale;
};

struct minmea_date
{
  int day;
  int month;
  int year;
};

struct minmea_time
{
  int hours;
  int minutes;
  int seconds;
  int microseconds;
};

struct minmea_sentence_rmc
{
  struct minmea_time   time;
  bool                 valid;
  struct minmea_float  latitude;
  struct minmea_float  longitude;
  struct minmea_float  speed;
  struct minmea_float  course;
  struct minmea_date   date;
  struct minmea_float  variation;
};

struct minmea_sentence_gga
{
  struct minmea_time   time;
  struct minmea_float  latitude;
  struct minmea_float  longitude;
  int                  fix_quality;
  int                  satellites_tracked;
  struct minmea_float  hdop;
  struct minmea_float  altitude;
  char                 altitude_units;
  struct minmea_float  height;
  char                 height_units;
  int                  dgps_age;
};

enum minmea_gll_status
{
  MINMEA_GLL_STATUS_DATA_VALID     = 'A',
  MINMEA_GLL_STATUS_DATA_NOT_VALID = 'V',
};

enum minmea_gll_mode
{
  MINMEA_GLL_MODE_AUTONOMOUS       = 'A',
  MINMEA_GLL_MODE_DPGS             = 'D',
  MINMEA_GLL_MODE_DR               = 'E',
};

struct minmea_sentence_gll
{
  struct minmea_float  latitude;
  struct minmea_float  longitude;
  struct minmea_time   time;
  char                 status;
  char                 mode;
};

struct minmea_sentence_gst
{
  struct minmea_time   time;
  struct minmea_float  rms_deviation;
  struct minmea_float  semi_major_deviation;
  struct minmea_float  semi_minor_deviation;
  struct minmea_float  semi_major_orientation;
  struct minmea_float  latitude_error_deviation;
  struct minmea_float  longitude_error_deviation;
  struct minmea_float  altitude_error_deviation;
};

enum minmea_gsa_mode
{
  MINMEA_GPGSA_MODE_AUTO           = 'A',
  MINMEA_GPGSA_MODE_FORCED         = 'M',
};

enum minmea_gsa_fix_type
{
  MINMEA_GPGSA_FIX_NONE            = 1,
  MINMEA_GPGSA_FIX_2D              = 2,
  MINMEA_GPGSA_FIX_3D              = 3,
};

struct minmea_sentence_gsa
{
  char                 mode;
  int                  fix_type;
  int                  sats[12];
  struct minmea_float  pdop;
  struct minmea_float  hdop;
  struct minmea_float  vdop;
};

struct minmea_sat_info
{
  int nr;
  int elevation;
  int azimuth;
  int snr;
};

struct minmea_sentence_gsv
{
  int                     total_msgs;
  int                     msg_nr;
  int                     total_sats;
  struct minmea_sat_info  sats[4];
};

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Calculate raw sentence checksum. Does not check sentence integrity. */

uint8_t minmea_checksum(FAR const char *sentence);

/* Check sentence validity and checksum. Returns true for valid sentences. */

bool minmea_check(FAR const char *sentence, bool strict);

/* Determine talker identifier. */

bool minmea_talker_id(char talker[3], FAR const char *sentence);

/* Determine sentence identifier. */

enum minmea_sentence_id minmea_sentence_id(FAR const char *sentence,
                                           bool strict);

/* Scanf-like processor for NMEA sentences. Supports the following formats:
 * c - single character (char *)
 * d - direction, returned as 1/-1, default 0 (int *)
 * f - fractional, returned as value + scale (int *, int *)
 * i - decimal, default zero (int *)
 * s - string (char *)
 * t - talker identifier and type (char *)
 * T - date/time stamp (int *, int *, int *)
 * Returns true on success. See library source code for details.
 */

bool minmea_scan(const char *sentence, const char *format, ...);

/* Parse a specific type of sentence. Return true on success. */

bool minmea_parse_rmc(struct minmea_sentence_rmc *frame, const char *sentence);
bool minmea_parse_gga(struct minmea_sentence_gga *frame, const char *sentence);
bool minmea_parse_gsa(struct minmea_sentence_gsa *frame, const char *sentence);
bool minmea_parse_gll(struct minmea_sentence_gll *frame, const char *sentence);
bool minmea_parse_gst(struct minmea_sentence_gst *frame, const char *sentence);
bool minmea_parse_gsv(struct minmea_sentence_gsv *frame, const char *sentence);

/* Convert GPS UTC date/time representation to a UNIX timestamp. */

int minmea_gettime(FAR struct timespec *ts,
                   FAR const struct minmea_date *date,
                   FAR const struct minmea_time *time_);

/* Rescale a fixed-point value to a different scale. Rounds towards zero. */

static inline int_least32_t minmea_rescale(FAR struct minmea_float *f,
                                           int_least32_t new_scale)
{
  if (f->scale == 0)
    {
      return 0;
    }

  if (f->scale == new_scale)
    {
      return f->value;
    }

  if (f->scale > new_scale)
    {
      return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale /
              new_scale / 2) / (f->scale / new_scale);
    }
  else
    {
    return f->value * (new_scale / f->scale);
    }
}

/* Convert a fixed-point value to a floating-point value.
 * Returns NaN for "unknown" values.
 */

static inline float minmea_tofloat(FAR struct minmea_float *f)
{
  if (f->scale == 0)
    {
      return NAN;
    }

  return (float) f->value / (float) f->scale;
}

/* Convert a raw coordinate to a floating point DD.DDD... value.
 * Returns NaN for "unknown" values.
 */

static inline float minmea_tocoord(FAR struct minmea_float *f)
{
  if (f->scale == 0)
    {
      return NAN;
    }

  int_least32_t degrees = f->value / (f->scale * 100);
  int_least32_t minutes = f->value % (f->scale * 100);

  return (float) degrees + (float) minutes / (60 * f->scale);
}

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_GPSUTILS_MINMEA_H */
