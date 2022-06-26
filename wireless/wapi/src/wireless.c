/****************************************************************************
 * apps/wireless/wapi/src/wireless.c
 *
 *   Copyright (C) 2011, 2017, 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for NuttX from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  - Redistributions of  source code must  retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <errno.h>

#include <nuttx/net/arp.h>
#include <nuttx/wireless/wireless.h>

#include "wireless/wapi.h"
#include "util.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Events & Streams */

struct wapi_event_stream_s
{
  FAR char *end;                  /* End of the stream */
  FAR char *current;              /* Current event in stream of events */
  FAR char *value;                /* Current value in event */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Frequency */

FAR const char *g_wapi_freq_flags[] =
{
  "WAPI_FREQ_AUTO",
  "WAPI_FREQ_FIXED",
  NULL
};

/* ESSID */

FAR const char *g_wapi_essid_flags[] =
{
  "WAPI_ESSID_OFF",
  "WAPI_ESSID_ON",
  NULL
};

/* Operating Mode */

FAR const char *g_wapi_modes[] =
{
  "WAPI_MODE_AUTO",
  "WAPI_MODE_ADHOC",
  "WAPI_MODE_MANAGED",
  "WAPI_MODE_MASTER",
  "WAPI_MODE_REPEAT",
  "WAPI_MODE_SECOND",
  "WAPI_MODE_MONITOR",
  "WAPI_MODE_MESH",
  NULL
};

/* Bit Rate */

FAR const char *g_wapi_bitrate_flags[] =
{
  "WAPI_BITRATE_AUTO",
  "WAPI_BITRATE_FIXED",
  NULL
};

/* Transmit Power */

FAR const char *g_wapi_txpower_flags[] =
{
  "WAPI_TXPOWER_DBM",
  "WAPI_TXPOWER_MWATT",
  "WAPI_TXPOWER_RELATIVE",
  NULL
};

/* Passphrase Algorithm */

FAR const char *g_wapi_alg_flags[] =
{
  "WPA_ALG_NONE",
  "WPA_ALG_WEP",
  "WPA_ALG_TKIP",
  "WPA_ALG_CCMP",
  NULL
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_freq2float
 *
 * Description:
 *   Converts internal representation of frequencies to a floating point.
 *
 ****************************************************************************/

static inline double wapi_freq2float(const struct iw_freq *freq)
{
  return ((double)freq->m) * pow(10, freq->e);
}

/****************************************************************************
 * Name: wapi_float2freq
 *
 * Description:
 *   Converts a floating point the our internal representation of
 *   frequencies.
 *
 ****************************************************************************/

static inline void wapi_float2freq(double floatfreq, struct iw_freq *freq)
{
  freq->e = (short)floor(log10(floatfreq));
  if (freq->e > 8)
    {
      freq->m = ((long)(floor(floatfreq / pow(10, freq->e - 6)))) * 100;
      freq->e -= 8;
    }
  else
    {
      freq->m = (long)floatfreq;
      freq->e = 0;
    }
}

/****************************************************************************
 * Name: wapi_parse_mode
 *
 * Description:
 *
 ****************************************************************************/

static int wapi_parse_mode(int iw_mode, FAR enum wapi_mode_e *wapi_mode)
{
  switch (iw_mode)
    {
    case WAPI_MODE_AUTO:
    case WAPI_MODE_ADHOC:
    case WAPI_MODE_MANAGED:
    case WAPI_MODE_MASTER:
    case WAPI_MODE_REPEAT:
    case WAPI_MODE_SECOND:
    case WAPI_MODE_MONITOR:
      *wapi_mode = iw_mode;
      return 0;

    default:
      WAPI_ERROR("ERROR: Unknown mode: %d\n", iw_mode);
      return -1;
    }
}

/****************************************************************************
 * Name: wapi_make_ether
 *
 * Description:
 *
 ****************************************************************************/

static int wapi_make_ether(FAR struct ether_addr *addr, int byte)
{
  WAPI_VALIDATE_PTR(addr);
  memset(addr, byte, sizeof(struct ether_addr));
  return 0;
}

/****************************************************************************
 * Name: wapi_event_stream_init
 *
 * Description:
 *   Initialize a stream to access the events.
 *
 ****************************************************************************/

static void wapi_event_stream_init(FAR struct wapi_event_stream_s *stream,
                                   FAR char *data, size_t len)
{
  memset(stream, 0, sizeof(struct wapi_event_stream_s));
  stream->current = data;
  stream->end = &data[len];
}

/****************************************************************************
 * Name: wapi_event_stream_extract
 *
 * Description:
 *   Extract the next event from the stream.
 *
 ****************************************************************************/

static int wapi_event_stream_extract(FAR struct wapi_event_stream_s *stream,
                                     FAR struct iw_event *iwe)
{
  int ret;
  FAR struct iw_event *iwe_stream;

  if (stream->current + offsetof(struct iw_event, u) > stream->end)
    {
      /* Nothing to process */

      return 0;
    }

  iwe_stream = (FAR struct iw_event *)stream->current;

  if (stream->current + iwe_stream->len > stream->end ||
      iwe_stream->len < offsetof(struct iw_event, u))
    {
      return -1;
    }

  ret = 1;

  switch (iwe_stream->cmd)
    {
      case SIOCGIWESSID:
      case SIOCGIWENCODE:
      case IWEVGENIE:
        iwe->cmd = iwe_stream->cmd;
        iwe->len = offsetof(struct iw_event, u) + sizeof(struct iw_point);
        iwe->u.data.flags = iwe_stream->u.data.flags;
        iwe->u.data.length = iwe_stream->u.data.length;

        iwe->u.data.pointer = (FAR void *)(stream->current +
                              offsetof(struct iw_event, u) +
                              (unsigned long)iwe_stream->u.data.pointer);
        break;

      default:
        if (iwe_stream->len > sizeof(*iwe))
          {
            WAPI_ERROR("Unhandled event size 0x%x %d\n", iwe_stream->cmd,
                                                         iwe_stream->len);
            iwe->cmd = 0;
            iwe->len = offsetof(struct iw_event, u);
            break;
          }

        memcpy(iwe, iwe_stream, iwe_stream->len);
    }

  /* Update stream to next event */

  stream->current += iwe_stream->len;
  return ret;
}

/****************************************************************************
 * Name: wapi_scan_event
 *
 * Description:
 *
 ****************************************************************************/

static int wapi_scan_event(FAR struct iw_event *event,
                           FAR struct wapi_list_s *list)
{
  FAR struct wapi_scan_info_s *info;

  /* Get current "wapi_info_t". */

  info = list->head.scan;

  /* Decode the event. */

  switch (event->cmd)
    {
    case SIOCGIWAP:
      {
        struct wapi_scan_info_s *temp;

        /* Allocate a new cell. */

        temp = malloc(sizeof(struct wapi_scan_info_s));
        if (!temp)
          {
            WAPI_STRERROR("malloc()");
            return -1;
          }

        /* Reset it. */

        bzero(temp, sizeof(struct wapi_scan_info_s));
        temp->encode = 0xffff;

        /* Save cell identifier. */

        memcpy(&temp->ap, &event->u.ap_addr.sa_data,
               sizeof(struct ether_addr));

        /* Push it to the head of the list. */

        temp->next = info;
        list->head.scan = temp;

        break;
      }

    case SIOCGIWFREQ:
      {
        info->has_freq = 1;

        if (event->u.freq.e == 0)
          {
            /* Some drivers do not report frequency, but a channel.
             * Try to map this to frequency by assuming they are using
             * IEEE 802.11b/g.  But don't overwrite a previously parsed
             * frequency if the driver sends both frequency and channel,
             * since the driver may be sending an A-band channel that we
             * don't handle here.
             */

            if (event->u.freq.m >= 1 && event->u.freq.m <= 13)
              {
                info->freq = 2407 + 5 * event->u.freq.m;
              }
            else if (event->u.freq.m == 14)
              {
                info->freq = 2484;
              }
            else if (event->u.freq.m >= 36 && event->u.freq.m <= 165)
              {
                info->freq = 5000 + 5 * event->u.freq.m;
              }
          }
        else
          {
            info->freq = wapi_freq2float(&(event->u.freq));
          }

        break;
      }

    case SIOCGIWMODE:
      {
        int ret = wapi_parse_mode(event->u.mode, &info->mode);
        if (ret >= 0)
          {
            info->has_mode = 1;
            break;
          }
        else
          {
            return ret;
          }
      }

    case SIOCGIWESSID:
      {
        info->has_essid = 1;
        info->essid_flag = (event->u.data.flags) ? WAPI_ESSID_ON
                                                 : WAPI_ESSID_OFF;
        memset(info->essid, 0, (WAPI_ESSID_MAX_SIZE + 1));
        if ((event->u.essid.pointer) && (event->u.essid.length))
          {
            memcpy(info->essid, event->u.essid.pointer,
                   event->u.essid.length);
          }

        break;
      }

    case SIOCGIWRATE:

      /* Scan may return a list of bitrates. As we have space for only a
       * single bitrate, we only keep the largest one.
       */

      if (!info->has_bitrate || event->u.bitrate.value > info->bitrate)
        {
          info->has_bitrate = 1;
          info->bitrate = event->u.bitrate.value;
        }

      break;

    case IWEVQUAL:
      {
        if (event->u.qual.updated & IW_QUAL_DBM)
          {
            info->has_rssi = 1;
            info->rssi = event->u.qual.level;

            /* Report signal levels in dBm */

            if (info->rssi >= 0x40)
              {
                info->rssi -= 0x100;
              }
          }

        break;
      }

    case SIOCGIWENCODE:
      {
        info->has_encode = 1;
        info->encode = event->u.data.flags;
        break;
      }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_get_freq
 *
 * Description:
 *   Gets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_get_freq(int sock, FAR const char *ifname, FAR double *freq,
                  FAR enum wapi_freq_flag_e *flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(freq);
  WAPI_VALIDATE_PTR(flag);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWFREQ, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWFREQ, errcode);
      ret = -errcode;
    }
  else
    {
      /* Set flag. */

      if (IW_FREQ_AUTO == wrq.u.freq.flags)
        {
          *flag = WAPI_FREQ_AUTO;
        }
      else if (IW_FREQ_FIXED == wrq.u.freq.flags)
        {
          *flag = WAPI_FREQ_FIXED;
        }
      else
        {
          WAPI_ERROR("ERROR: Unknown flag: %d\n", wrq.u.freq.flags);
          return -1;
        }

      /* Set freq. */

      *freq = wapi_freq2float(&(wrq.u.freq));
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_freq
 *
 * Description:
 *   Sets the operating frequency of the device.
 *
 ****************************************************************************/

int wapi_set_freq(int sock, FAR const char *ifname, double freq,
                  enum wapi_freq_flag_e flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  /* Set freq. */

  wapi_float2freq(freq, &(wrq.u.freq));

  /* Set flag. */

  switch (flag)
    {
    case WAPI_FREQ_AUTO:
      wrq.u.freq.flags = IW_FREQ_AUTO;
      break;

    case WAPI_FREQ_FIXED:
      wrq.u.freq.flags = IW_FREQ_FIXED;
      break;
    }

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWFREQ, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWFREQ, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_freq2chan
 *
 * Description:
 *   Finds corresponding channel for the supplied freq.
 *
 * Returned Value:
 *   0, on success; -2, if not found; otherwise, ioctl() return value.
 *
 ****************************************************************************/

int wapi_freq2chan(int sock, FAR const char *ifname, double freq,
                   FAR int *chan)
{
  struct iwreq wrq =
  {
  };

  char buf[sizeof(struct iw_range) * 2];
  int ret;

  WAPI_VALIDATE_PTR(chan);

  /* Prepare request. */

  bzero(buf, sizeof(buf));
  wrq.u.data.pointer = buf;
  wrq.u.data.length = sizeof(buf);
  wrq.u.data.flags = 0;

  /* Get range. */

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWRANGE, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      struct iw_range *range = (struct iw_range *)buf;
      int k;

      /* Compare the frequencies as double to ignore differences in encoding.
       * Slower, but safer...
       */

      for (k = 0; k < range->num_frequency; k++)
        {
          if (freq == wapi_freq2float(&(range->freq[k])))
            {
              *chan = range->freq[k].i;
              return 0;
            }
        }

      /* Oops! Nothing found. */

      ret = -2;
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWRANGE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_chan2freq
 *
 * Description:
 *   Finds corresponding frequency for the supplied chan.
 *
 * Returned Value:
 *   0, on success; -2, if not found; otherwise, ioctl() return value.
 *
 ****************************************************************************/

int wapi_chan2freq(int sock, FAR const char *ifname, int chan,
                   FAR double *freq)
{
  struct iwreq wrq =
  {
  };

  char buf[sizeof(struct iw_range) * 2];
  int ret;

  WAPI_VALIDATE_PTR(freq);

  /* Prepare request. */

  bzero(buf, sizeof(buf));
  wrq.u.data.pointer = buf;
  wrq.u.data.length = sizeof(buf);
  wrq.u.data.flags = 0;

  /* Get range. */

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWRANGE, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      struct iw_range *range = (struct iw_range *)buf;
      int k;

      for (k = 0; k < range->num_frequency; k++)
        {
          if (chan == range->freq[k].i)
            {
              *freq = wapi_freq2float(&(range->freq[k]));
              return 0;
            }
        }

      /* Oops! Nothing found. */

      ret = -2;
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWRANGE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_essid
 *
 * Description:
 *   Gets ESSID of the device.
 *
 * Input Parameters:
 *   essid - Used to store the ESSID of the device. Buffer must have
 *           enough space to store WAPI_ESSID_MAX_SIZE+1 characters.
 *
 ****************************************************************************/

int wapi_get_essid(int sock, FAR const char *ifname, FAR char *essid,
                   FAR enum wapi_essid_flag_e *flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(essid);
  WAPI_VALIDATE_PTR(flag);

  wrq.u.essid.pointer = essid;
  wrq.u.essid.length = WAPI_ESSID_MAX_SIZE + 1;
  wrq.u.essid.flags = 0;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWESSID, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWESSID, errcode);
      ret = -errcode;
    }
  else
    {
      *flag = (wrq.u.essid.flags) ? WAPI_ESSID_ON : WAPI_ESSID_OFF;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_essid
 *
 * Description:
 *    Sets ESSID of the device.
 *
 *    essid At most WAPI_ESSID_MAX_SIZE characters are read.
 *
 ****************************************************************************/

int wapi_set_essid(int sock, FAR const char *ifname, FAR const char *essid,
                   enum wapi_essid_flag_e flag)
{
  char buf[WAPI_ESSID_MAX_SIZE + 1];
  struct iwreq wrq =
  {
  };

  int ret;

  /* Prepare request. */

  wrq.u.essid.pointer = buf;
  wrq.u.essid.length =
    snprintf(buf, ((WAPI_ESSID_MAX_SIZE + 1) * sizeof(char)), "%s", essid);
  wrq.u.essid.flags = flag;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWESSID, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWESSID, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_mode
 *
 * Description:
 *   Gets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_get_mode(int sock, FAR const char *ifname,
                  FAR enum wapi_mode_e *mode)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(mode);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWMODE, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      ret = wapi_parse_mode(wrq.u.mode, mode);
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWMODE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_mode
 *
 * Description:
 *   Sets the operating mode of the device.
 *
 ****************************************************************************/

int wapi_set_mode(int sock, FAR const char *ifname, enum wapi_mode_e mode)
{
  struct iwreq wrq =
  {
  };

  int ret;

  wrq.u.mode = mode;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWMODE, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWMODE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_make_broad_ether
 *
 * Description:
 *   Creates an Ethernet broadcast address.
 *
 ****************************************************************************/

int wapi_make_broad_ether(FAR struct ether_addr *sa)
{
  return wapi_make_ether(sa, 0xff);
}

/****************************************************************************
 * Name: wapi_make_null_ether
 *
 * Description:
 *   Creates an ethernet NULL address.
 *
 ****************************************************************************/

int wapi_make_null_ether(FAR struct ether_addr *sa)
{
  return wapi_make_ether(sa, 0x00);
}

/****************************************************************************
 * Name: wapi_get_ap
 *
 * Description:
 *   Gets access point address of the device.
 *
 * Input Parameters:
 *   ap - Set the to MAC address of the device. (For "any", a broadcast
 *        ethernet address; for "off", a null ethernet address is used.)
 *
 ****************************************************************************/

int wapi_get_ap(int sock, FAR const char *ifname, FAR struct ether_addr *ap)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(ap);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWAP, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      memcpy(ap, wrq.u.ap_addr.sa_data, sizeof(struct ether_addr));
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWAP, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_ap
 *
 * Description:
 *   Sets access point address of the device.
 *
 ****************************************************************************/

int wapi_set_ap(int sock, FAR const char *ifname,
                FAR const struct ether_addr *ap)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(ap);

  wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
  memcpy(wrq.u.ap_addr.sa_data, ap, sizeof(struct ether_addr));
  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);

  ret = ioctl(sock, SIOCSIWAP, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWAP, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_bitrate
 *
 * Description:
 *   Gets bitrate of the device.
 *
 ****************************************************************************/

int wapi_get_bitrate(int sock, FAR const char *ifname,
                     FAR int *bitrate, FAR enum wapi_bitrate_flag_e *flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(bitrate);
  WAPI_VALIDATE_PTR(flag);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWRATE, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      /* Check if enabled. */

      if (wrq.u.bitrate.disabled)
        {
          WAPI_ERROR("ERROR: Bitrate is disabled\n");
          return -1;
        }

      /* Get bitrate. */

      *bitrate = wrq.u.bitrate.value;
      *flag = wrq.u.bitrate.fixed ? WAPI_BITRATE_FIXED : WAPI_BITRATE_AUTO;
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWRATE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_bitrate
 *
 * Description:
 *   Sets bitrate of the device.
 *
 ****************************************************************************/

int wapi_set_bitrate(int sock, FAR const char *ifname, int bitrate,
                     enum wapi_bitrate_flag_e flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  wrq.u.bitrate.value = bitrate;
  wrq.u.bitrate.fixed = (flag == WAPI_BITRATE_FIXED);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWRATE, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWRATE, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_dbm2mwatt
 *
 * Description:
 *   Converts a value in dBm to a value in milliWatt.
 *
 ****************************************************************************/

int wapi_dbm2mwatt(int dbm)
{
  return floor(pow(10, (((double)dbm) / 10)));
}

/****************************************************************************
 * Name: wapi_mwatt2dbm
 *
 * Description:
 *   Converts a value in milliWatt to a value in dBm.
 *
 ****************************************************************************/

int wapi_mwatt2dbm(int mwatt)
{
  return ceil(10 * log10(mwatt));
}

/****************************************************************************
 * Name: wapi_get_txpower
 *
 * Description:
 *   Gets txpower of the device.
 *
 ****************************************************************************/

int wapi_get_txpower(int sock, FAR const char *ifname, FAR int *power,
                     FAR enum wapi_txpower_flag_e *flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(power);
  WAPI_VALIDATE_PTR(flag);

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWTXPOW, (unsigned long)((uintptr_t)&wrq));
  if (ret >= 0)
    {
      /* Check if enabled. */

      if (wrq.u.txpower.disabled)
        {
          return -1;
        }

      /* Get flag. */

      if (IW_TXPOW_DBM == (wrq.u.txpower.flags & IW_TXPOW_DBM))
        {
          *flag = WAPI_TXPOWER_DBM;
        }
      else if (IW_TXPOW_MWATT == (wrq.u.txpower.flags & IW_TXPOW_MWATT))
        {
          *flag = WAPI_TXPOWER_MWATT;
        }
      else if (IW_TXPOW_RELATIVE ==
               (wrq.u.txpower.flags & IW_TXPOW_RELATIVE))
        {
          *flag = WAPI_TXPOWER_RELATIVE;
        }
      else
        {
          WAPI_ERROR("ERROR: Unknown flag: %d\n", wrq.u.txpower.flags);
          return -1;
        }

      /* Get power. */

      *power = wrq.u.txpower.value;
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWTXPOW, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_set_txpower
 *
 * Description:
 *   Sets txpower of the device.
 *
 ****************************************************************************/

int wapi_set_txpower(int sock, FAR const char *ifname, int power,
                     enum wapi_txpower_flag_e flag)
{
  struct iwreq wrq =
  {
  };

  int ret;

  /* Construct the request. */

  wrq.u.txpower.value = power;
  switch (flag)
    {
    case WAPI_TXPOWER_DBM:
      wrq.u.txpower.flags = IW_TXPOW_DBM;
      break;

    case WAPI_TXPOWER_MWATT:
      wrq.u.txpower.flags = IW_TXPOW_MWATT;
      break;

    case WAPI_TXPOWER_RELATIVE:
      wrq.u.txpower.flags = IW_TXPOW_RELATIVE;
      break;
    }

  /* Issue the set command. */

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWTXPOW, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWTXPOW, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_scan_channel_init
 *
 * Description:
 *   Starts a scan on the given interface. Root privileges are required to
 *   start a scan with specified channels.
 *
 ****************************************************************************/

int wapi_scan_channel_init(int sock, FAR const char *ifname,
                           FAR const char *essid,
                           uint8_t *channels, int num_channels)
{
  struct iw_scan_req req;
  struct iwreq wrq =
  {
  };

  size_t essid_len;
  int ret;
  int i;

  if (essid && (essid_len = strlen(essid)) > 0)
    {
      memset(&req, 0, sizeof(req));
      req.essid_len       = essid_len;
      req.bssid.sa_family = ARPHRD_ETHER;
      memset(req.bssid.sa_data, 0xff, IFHWADDRLEN);
      memcpy(req.essid, essid, essid_len);
      wrq.u.data.pointer  = (caddr_t)&req;
      wrq.u.data.length   = sizeof(req);
      wrq.u.data.flags    = IW_SCAN_THIS_ESSID;
    }

  if (channels && num_channels > 0)
    {
      req.num_channels = num_channels;
      for (i = 0; i < num_channels; i++)
        {
          req.channel_list[i].m = channels[i];
        }
    }

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWSCAN, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWSCAN, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_scan_init
 *
 * Description:
 *   Starts a scan on the given interface. Root privileges are required to
 *   start a scan.
 *
 ****************************************************************************/

int wapi_scan_init(int sock, FAR const char *ifname, FAR const char *essid)
{
  return wapi_scan_channel_init(sock, ifname, essid, NULL, 0);
}

/****************************************************************************
 * Name: wapi_scan_stat
 *
 * Description:
 *   Checks the status of the scan process.
 *
 * Returned Value:
 *   Zero, if data is ready; 1, if data is not ready; negative on failure.
 *
 ****************************************************************************/

int wapi_scan_stat(int sock, FAR const char *ifname)
{
  struct iwreq wrq =
  {
  };

  int ret;
  char buf;

  wrq.u.data.pointer = &buf;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWSCAN, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      if (errno == E2BIG)
        {
          /* Data is ready, but not enough space, which is expected. */

          return 0;
        }
      else if (errno == EAGAIN)
        {
          /* Data is not ready. */

          return 1;
        }

      printf("err[%d]: %s\n", errno, strerror(errno));
    }
  else
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWSCAN, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_scan_coll
 *
 * Description:
 *   Collects the results of a scan process.
 *
 * Input Parameters:
 *   aps - Pushes collected  struct wapi_scan_info_s into this list.
 *
 ****************************************************************************/

int wapi_scan_coll(int sock, FAR const char *ifname,
                   FAR struct wapi_list_s *aps)
{
  FAR char *buf;
  int buflen;
  struct iwreq wrq =
  {
  };

  int ret;

  WAPI_VALIDATE_PTR(aps);

  buflen = IW_SCAN_MAX_DATA;
  buf = malloc(buflen * sizeof(char));
  if (!buf)
    {
      WAPI_STRERROR("malloc()");
      return -1;
    }

alloc:

  /* Collect results. */

  wrq.u.data.pointer = buf;
  wrq.u.data.length  = buflen;
  wrq.u.data.flags   = 0;
  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);

  ret = ioctl(sock, SIOCGIWSCAN, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0 && errno == E2BIG)
    {
      FAR char *tmp;

      buflen *= 2;
      tmp = realloc(buf, buflen);
      if (!tmp)
        {
          WAPI_STRERROR("realloc()");
          free(buf);
          return -1;
        }

      buf = tmp;
      goto alloc;
    }

  /* There is still something wrong. It's either EAGAIN or some other ioctl()
   * failure. We don't bother, let the user deal with it.
   */

  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWSCAN, errcode);
      free(buf);
      return -errcode;
    }

  /* We have the results, process them. */

  if (wrq.u.data.length)
    {
      struct iw_event iwe;
      struct wapi_event_stream_s stream;

      wapi_event_stream_init(&stream, buf, wrq.u.data.length);
      do
        {
          /* Get the next event from the stream */

          ret = wapi_event_stream_extract(&stream, &iwe);
          if (ret > 0)
            {
              int eventret = wapi_scan_event(&iwe, aps);
              if (eventret < 0)
                {
                  ret = eventret;
                }
            }
          else if (ret < 0)
            {
              WAPI_ERROR("ERROR: wapi_event_stream_extract() failed!\n");
            }
        }
      while (ret > 0);
    }

  /* Free request buffer. */

  free(buf);
  return ret;
}

/****************************************************************************
 * Name: wapi_scan_coll_free
 *
 * Description:
 *   Free the scan results.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void wapi_scan_coll_free(FAR struct wapi_list_s *list)
{
  FAR struct wapi_scan_info_s *temp;
  FAR struct wapi_scan_info_s *info;

  if (list == NULL)
    {
      return;
    }

  info = list->head.scan;
  while (info)
    {
      temp = info->next;
      free(info);
      info = temp;
    }
}

/****************************************************************************
 * Name: wapi_set_country
 *
 * Description:
 *    Set the country code
 *
 ****************************************************************************/

int wapi_set_country(int sock, FAR const char *ifname,
                     FAR const char *country)
{
  struct iwreq wrq =
  {
  };

  int ret;

  /* Prepare request. */

  wrq.u.data.pointer = (FAR void *)country;
  wrq.u.data.length = 2;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCSIWCOUNTRY, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCSIWCOUNTRY, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_country
 *
 * Description:
 *    Get the country code
 *
 ****************************************************************************/

int wapi_get_country(int sock, FAR const char *ifname,
                     FAR char *country)
{
  struct iwreq wrq =
  {
  };

  int ret;

  /* Prepare request. */

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  wrq.u.data.pointer = (FAR void *)country;
  wrq.u.data.length = 2;
  ret = ioctl(sock, SIOCGIWCOUNTRY, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWSENS, errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: wapi_get_sensitivity
 *
 * Description:
 *    Get the wlan Sensitivity
 *
 ****************************************************************************/

int wapi_get_sensitivity(int sock, FAR const char *ifname, FAR int *sense)
{
  struct iwreq wrq =
  {
  };

  int ret;

  strlcpy(wrq.ifr_name, ifname, IFNAMSIZ);
  ret = ioctl(sock, SIOCGIWSENS, (unsigned long)((uintptr_t)&wrq));
  if (ret < 0)
    {
      int errcode = errno;
      WAPI_IOCTL_STRERROR(SIOCGIWSENS, errcode);
      ret = -errcode;
    }
  else
    {
      *sense = -wrq.u.sens.value;
    }

  return ret;
}

