/****************************************************************************
 * apps/logging/nxscope/nxscope_chan.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <debug.h>
#include <endian.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <logging/nxscope/nxscope.h>

#include "nxscope_internals.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ENDIAN_BIG
#  error Big endian not tested
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

int g_type_size[] =
{
  0,                            /* NXSCOPE_TYPE_UNDEF */
  0,                            /* NXSCOPE_TYPE_NONE */
  sizeof(uint8_t),              /* NXSCOPE_TYPE_UINT8 */
  sizeof(int8_t),               /* NXSCOPE_TYPE_INT8 */
  sizeof(uint16_t),             /* NXSCOPE_TYPE_UINT16 */
  sizeof(int16_t),              /* NXSCOPE_TYPE_INT16 */
  sizeof(uint32_t),             /* NXSCOPE_TYPE_UINT32 */
  sizeof(int32_t),              /* NXSCOPE_TYPE_INT32 */
  sizeof(uint64_t),             /* NXSCOPE_TYPE_UINT64 */
  sizeof(int64_t),              /* NXSCOPE_TYPE_INT64 */
  sizeof(float),                /* NXSCOPE_TYPE_FLOAT */
  sizeof(double),               /* NXSCOPE_TYPE_DOUBLE */
  sizeof(ub8_t),                /* NXSCOPE_TYPE_UB8 */
  sizeof(b8_t),                 /* NXSCOPE_TYPE_B8 */
  sizeof(ub16_t),               /* NXSCOPE_TYPE_UB16 */
  sizeof(b16_t),                /* NXSCOPE_TYPE_B16 */
  sizeof(ub32_t),               /* NXSCOPE_TYPE_UB32 */
  sizeof(b32_t),                /* NXSCOPE_TYPE_B32 */
  sizeof(char),                 /* NXSCOPE_TYPE_CHAR */
#if 0
  /* Reserved for future use */

  sizeof(wchar_t),              /* NXSCOPE_TYPE_WCHAR */
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_USERTYPES
  /* User type always last element and always equal to  1B */

  sizeof(uint8_t),              /* NXSCOPE_TYPE_USER */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_stream_overflow
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static void nxscope_stream_overflow(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  s->streambuf[s->proto_stream->hdrlen] |= NXSCOPE_STREAM_FLAGS_OVERFLOW;
}

/****************************************************************************
 * Name: nxscope_ch_validate
 ****************************************************************************/

static int nxscope_ch_validate(FAR struct nxscope_s *s, uint8_t ch,
                               uint8_t type, uint8_t d, uint8_t mlen)
{
  union nxscope_chinfo_type_u utype;
  size_t                      next_i    = 0;
  int                         ret       = OK;
  size_t                      type_size = 0;

  DEBUGASSERT(s);

  /* Do nothing if stream not started */

  if (!s->start)
    {
      ret = -EAGAIN;
      goto errout;
    }

  /* Do nothing if channel not enabled */

  if (s->chinfo[ch].enable != 1)
    {
      ret = -EAGAIN;
      goto errout;
    }

  /* Some additional checks if debug features enabled */

#ifdef CONFIG_DEBUG_FEATURES
  /* Validate channel */

  if (ch > s->cmninfo.chmax)
    {
      _err("ERROR: invalid channel %d\n", ch);
      ret = -EINVAL;
      goto errout;
    }

  /* Validate channel type */

  if (s->chinfo[ch].type.s.dtype != type)
    {
      _err("ERROR: invalid channel type %d != %d\n",
           s->chinfo[ch].type.u8, type);
      ret = -EINVAL;
      goto errout;
    }

  /* Validate channel vdim */

  if (s->chinfo[ch].vdim != d)
    {
      _err("ERROR: invalid channel dim %d\n", d);
      ret = -EINVAL;
      goto errout;
    }

  /* Validate channel metadata size */

  if (s->chinfo[ch].mlen != mlen)
    {
      _err("ERROR: invalid channel mlen %d\n", mlen);
      ret = -EINVAL;
      goto errout;
    }
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  /* Handle sample rate divider */

  s->cntr[ch] += 1;
  if (s->cntr[ch] % (s->chinfo[ch].div + 1) != 0)
    {
      ret = -EAGAIN;
      goto errout;
    }
#endif

  /* Get utype */

  utype.u8 = type;

  /* Check buffer size */

#ifdef CONFIG_LOGGING_NXSCOPE_USERTYPES
  if (type >= NXSCOPE_TYPE_USER)
    {
      type_size = 1;
    }
  else
#endif
    {
      type_size = g_type_size[utype.s.dtype];
    }

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  if (utype.s.cri)
    {
#  ifdef CONFIG_DEBUG_FEATURES
      next_i = (s->proto_stream->hdrlen + 1 + type_size * d + mlen +
                s->proto_stream->footlen);

      /* Verify the size of the critical channels buffer  */

      if (s->cribuf_len < next_i)
        {
          _err("ERROR: no space in cribuf %zu < %zu\n", s->cribuf_len,
               next_i);
          ret = -ENOBUFS;
          goto errout;
        }
      else
#  endif
        {
          /* No more checks needed for critical channel */

          ret = OK;
          goto errout;
        }
    }
#endif

  next_i = (s->stream_i + 1 + type_size * d + mlen +
            s->proto_stream->footlen);

  if (next_i > s->streambuf_len)
    {
      _err("ERROR: no space for data %zu\n", s->stream_i);
      nxscope_stream_overflow(s);
      ret = -ENOBUFS;
      goto errout;
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_put_vector
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       stream buffer
 *
 * IMPORTANT: Data stored always as little-endian !
 *
 ****************************************************************************/

static int nxscope_put_vector(FAR uint8_t *buff, uint8_t type, FAR void *val,
                              uint8_t d)
{
  int i = 0;
  int j = 0;

  DEBUGASSERT(buff);

  /* Pack data */

  switch (type)
    {
      case NXSCOPE_TYPE_NONE:
        {
          /* Nothing to do here */

          break;
        }

      case NXSCOPE_TYPE_UINT8:
      case NXSCOPE_TYPE_INT8:
#ifdef CONFIG_LOGGING_NXSCOPE_USERTYPES
      case NXSCOPE_TYPE_USER:
#endif
        {
          uint8_t u8 = 0;

          for (i = 0; i < d; i++)
            {
              DEBUGASSERT(val);
              u8 = ((FAR uint8_t *)val)[i];

              buff[j++] = u8;
            }

          break;
        }

      case NXSCOPE_TYPE_UINT16:
      case NXSCOPE_TYPE_INT16:
      case NXSCOPE_TYPE_B8:
      case NXSCOPE_TYPE_UB8:
        {
          uint16_t u16 = 0;

          for (i = 0; i < d; i++)
            {
              DEBUGASSERT(val);
              u16 = htole16(((FAR uint16_t *)val)[i]);

              buff[j++] = ((u16 >> 0) & 0xff);
              buff[j++] = ((u16 >> 8) & 0xff);
            }

          break;
        }

      case NXSCOPE_TYPE_UINT32:
      case NXSCOPE_TYPE_INT32:
      case NXSCOPE_TYPE_FLOAT:
      case NXSCOPE_TYPE_B16:
      case NXSCOPE_TYPE_UB16:
        {
          uint32_t u32 = 0;

          for (i = 0; i < d; i++)
            {
              DEBUGASSERT(val);
              u32 = htole32(((FAR uint32_t *)val)[i]);

              buff[j++] = ((u32 >> 0) & 0xff);
              buff[j++] = ((u32 >> 8) & 0xff);
              buff[j++] = ((u32 >> 16) & 0xff);
              buff[j++] = ((u32 >> 24) & 0xff);
            }

          break;
        }

#ifdef CONFIG_HAVE_LONG_LONG
      case NXSCOPE_TYPE_UINT64:
      case NXSCOPE_TYPE_INT64:
      case NXSCOPE_TYPE_DOUBLE:
      case NXSCOPE_TYPE_B32:
      case NXSCOPE_TYPE_UB32:
        {
          for (i = 0; i < d; i++)
            {
              uint64_t u64 = 0;

              DEBUGASSERT(val);
              u64 = htole64(((FAR uint64_t *)val)[i]);

              buff[j++] = ((u64 >> 0) & 0xff);
              buff[j++] = ((u64 >> 8) & 0xff);
              buff[j++] = ((u64 >> 16) & 0xff);
              buff[j++] = ((u64 >> 24) & 0xff);
              buff[j++] = ((u64 >> 32) & 0xff);
              buff[j++] = ((u64 >> 40) & 0xff);
              buff[j++] = ((u64 >> 48) & 0xff);
              buff[j++] = ((u64 >> 56) & 0xff);
            }

          break;
        }
#endif

      case NXSCOPE_TYPE_CHAR:
        {
          /* Copy string bytes and fill with '\0' */

          DEBUGASSERT(val);

          strlcpy((FAR char *)buff, (FAR const char *)val, d);
          j += strlen((FAR char *)buff);
          memset(&buff[j], '\0', d - j);
          j = d;

          break;
        }

      default:
        {
          _err("ERROR: invalid type=%d\n", type);
          DEBUGASSERT(0);
        }
    }

  return j;
}

/****************************************************************************
 * Name: nxscope_put_meta
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       stream buffer
 *
 * REVISIT: what about endianness ?
 *
 ****************************************************************************/

static int nxscope_put_meta(FAR uint8_t *buff, FAR uint8_t *meta,
                            uint8_t mlen)
{
  int i = 0;

  DEBUGASSERT(buff);

  for (i = 0; i < mlen; i++)
    {
      DEBUGASSERT(meta);
      buff[i] = meta[i];
    }

  return mlen;
}

/****************************************************************************
 * Name: nxscope_put_sample
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       stream buffer
 *
 ****************************************************************************/

static void nxscope_put_sample(FAR uint8_t *buff, FAR size_t *buff_i,
                               uint8_t type, uint8_t ch, FAR void *val,
                               uint8_t d, FAR uint8_t *meta, uint8_t mlen)
{
  size_t i = 0;

  /* Channel ID */

  buff[(*buff_i)++] = ch;

  /* Vector sample data - always little-endian */

  i = nxscope_put_vector(&buff[*buff_i], type, val, d);
  *buff_i += i;

  /* Meta data.
   * REVISIT: what about endianness ?
   */

  i = nxscope_put_meta(&buff[*buff_i], meta, mlen);
  *buff_i += i;
}

/****************************************************************************
 * Name: nxscope_put_common_m
 ****************************************************************************/

static int nxscope_put_common_m(FAR struct nxscope_s *s, uint8_t type,
                                uint8_t ch, FAR void *val, uint8_t d,
                                FAR uint8_t *meta, uint8_t mlen)
{
  FAR uint8_t                 *buff   = NULL;
  FAR size_t                  *buff_i = NULL;
  int                          ret    = OK;
#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  size_t                       tmp    = 0;
  union nxscope_chinfo_type_u  utype;
#endif

  DEBUGASSERT(s);

#ifndef CONFIG_LOGGING_NXSCOPE_DISABLE_PUTLOCK
  nxscope_lock(s);
#endif

  /* Validate data */

  ret = nxscope_ch_validate(s, ch, type, d, mlen);
  if (ret != OK)
    {
      goto errout;
    }

  /* Get buffer to send */

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  utype.u8 = type;
  if (utype.s.cri)
    {
      /* Dedicated critical channels buffer */

      buff   = s->cribuf;
      buff_i = &tmp;
    }
  else
#endif
    {
      /* Common stream buffer */

      buff   = s->streambuf;
      buff_i = &s->stream_i;
    }

  /* Put sample on buffer */

  nxscope_put_sample(buff, buff_i, type, ch, val, d, meta, mlen);

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  if (utype.s.cri)
    {
      /* Send data without buffering */

      ret = nxscope_stream_send(s, buff, buff_i);
      if (ret < 0)
        {
          _err("ERROR: nxscope_stream_send failed %d\n", ret);
          goto errout;
        }
    }
#endif

errout:
#ifndef CONFIG_LOGGING_NXSCOPE_DISABLE_PUTLOCK
  nxscope_unlock(s);
#endif

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_chan_init
 *
 * Description:
 *   Initialize nxscope channel
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   ch   - a channel id
 *   name - a channel name
 *   type - a channel data type (union nxscope_chinfo_type_u)
 *   vdim - a vector data dimension (vdim=1 for a point)
 *   mlen - a length of metadata
 *
 ****************************************************************************/

int nxscope_chan_init(FAR struct nxscope_s *s, uint8_t ch, FAR char *name,
                      uint8_t type, uint8_t vdim, uint8_t mlen)
{
  int ret = OK;

  DEBUGASSERT(s);
  DEBUGASSERT(name);

  if (ch > s->cmninfo.chmax)
    {
      _err("ERROR: invalid channel %d\n", ch);
      ret = -EINVAL;
      goto errout;
    }

  nxscope_lock(s);

#ifndef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  if (NXSCOPE_IS_CRICHAN(type))
    {
      _err("ERROR: cri channels not supported ch=%d\n", ch);
      ret = -EINVAL;
      goto errout;
    }
#endif

  /* Reset channel data */

  memset(&s->chinfo[ch], 0, sizeof(struct nxscope_chinfo_s));

  /* Copy channel info */

  s->chinfo[ch].type.u8 = type;
  s->chinfo[ch].vdim    = vdim;
  s->chinfo[ch].mlen    = mlen;
  s->chinfo[ch].name    = name;

  nxscope_unlock(s);

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_chan_en
 *
 * Description:
 *   Enable/disable a given channel
 *
 * Input Parameters:
 *   s  - a pointer to a nxscope instance
 *   ch - a channel id
 *   en - enable/disable
 *
 ****************************************************************************/

int nxscope_chan_en(FAR struct nxscope_s *s, uint8_t ch, bool en)
{
  int ret = OK;

  DEBUGASSERT(s);

  nxscope_lock(s);

  if (ch > s->cmninfo.chmax)
    {
      _err("ERROR: invalid channel %d\n", ch);
      ret = -EINVAL;
      goto errout;
    }

  if (s->chinfo[ch].type.s.dtype == NXSCOPE_TYPE_UNDEF)
    {
      _err("ERROR: channel not initialized %d\n", ch);
      ret = -EINVAL;
      goto errout;
    }

  _info("chan_en=%d %d\n", ch, en);

  /* Set enable flag */

  s->chinfo[ch].enable = en;

errout:
  nxscope_unlock(s);

  return ret;
}

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
/****************************************************************************
 * Name: nxscope_chan_div
 *
 * Description:
 *   Configure divider for a given channel
 *
 * Input Parameters:
 *   s   - a pointer to a nxscope instance
 *   ch  - a channel id
 *   div - divider value - starts from 0
 *
 ****************************************************************************/

int nxscope_chan_div(FAR struct nxscope_s *s, uint8_t ch, uint8_t div)
{
  int ret = OK;

  DEBUGASSERT(s);

  nxscope_lock(s);

  if (ch > s->cmninfo.chmax)
    {
      _err("ERROR: invalid channel %d\n", ch);
      ret = -EINVAL;
      goto errout;
    }

  _info("chan_div=%d %d\n", ch, div);

  /* Set divider */

  s->chinfo[ch].div = div;

errout:
  nxscope_unlock(s);

  return ret;
}
#endif

/****************************************************************************
 * Name: nxscope_chan_all_en
 *
 * Description:
 *   Enable/disable all channels
 *
 * Input Parameters:
 *   s  - a pointer to a nxscope instance
 *   en - enable/disable
 *
 ****************************************************************************/

int nxscope_chan_all_en(FAR struct nxscope_s *s, bool en)
{
  int     ret = OK;
  uint8_t i   = 0;

  DEBUGASSERT(s);

  for (i = 0; i < s->cmninfo.chmax; i++)
    {
      ret |= nxscope_chan_en(s, i, en);
    }

  return ret;
}

/****************************************************************************
 * Name: nxscope_put_vXXXX_m
 *
 * Description:
 *   Put a vector with metadata on the stream buffer
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   ch   - a channel id
 *   val  - a pointer to a sample data vector
 *   d    - a dimmention of sample data vector
 *   meta - a pointer to metadata
 *   mlen - a length of metadata
 *
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_put_vuint8_m
 ****************************************************************************/

int nxscope_put_vuint8_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR uint8_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UINT8,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vint8_m
 ****************************************************************************/

int nxscope_put_vint8_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR int8_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_INT8,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vuint16_m
 ****************************************************************************/

int nxscope_put_vuint16_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint16_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UINT16,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vint16_m
 ****************************************************************************/

int nxscope_put_vint16_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int16_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_INT16,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vuint32_m
 ****************************************************************************/

int nxscope_put_vuint32_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint32_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UINT32,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vint32_m
 ****************************************************************************/

int nxscope_put_vint32_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int32_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_INT32,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vuint64_m
 ****************************************************************************/

int nxscope_put_vuint64_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint64_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UINT64,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vint64_m
 ****************************************************************************/

int nxscope_put_vint64_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int64_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_INT64,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vfloat_m
 ****************************************************************************/

int nxscope_put_vfloat_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR float *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_FLOAT,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vdouble_m
 ****************************************************************************/

int nxscope_put_vdouble_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR double *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_DOUBLE,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vub8_m
 ****************************************************************************/

int nxscope_put_vub8_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR ub8_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UB8,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vb8_m
 ****************************************************************************/

int nxscope_put_vb8_m(FAR struct nxscope_s *s, uint8_t ch,
                      FAR b8_t *val, uint8_t d,
                      FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_B8,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vub16_m
 ****************************************************************************/

int nxscope_put_vub16_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR ub16_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UB16,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vb16_m
 ****************************************************************************/

int nxscope_put_vb16_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR b16_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_B16,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vub32_m
 ****************************************************************************/

int nxscope_put_vub32_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR ub32_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_UB32,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vb32_m
 ****************************************************************************/

int nxscope_put_vb32_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR b32_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_B32,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vchar_m
 *
 * NOTE: if a given string is shorten than initialized channel vdim,
 *       we put only string bytes + '\0'
 *
 ****************************************************************************/

int nxscope_put_vchar_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR char *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_common_m(s, NXSCOPE_TYPE_CHAR,
                              ch, val, d, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_vXXXX
 *
 * Description:
 *   Put a vector on the stream buffer
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   ch   - a channel id
 *   val  - a pointer to a sample data vector
 *   d    - a dimmention of sample data vector
 *
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_put_vuint8
 ****************************************************************************/

int nxscope_put_vuint8(FAR struct nxscope_s *s, uint8_t ch,
                       FAR uint8_t *val, uint8_t d)
{
  return nxscope_put_vuint8_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vint8
 ****************************************************************************/

int nxscope_put_vint8(FAR struct nxscope_s *s, uint8_t ch,
                      FAR int8_t *val, uint8_t d)
{
  return nxscope_put_vint8_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vuint16
 ****************************************************************************/

int nxscope_put_vuint16(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint16_t *val, uint8_t d)
{
  return nxscope_put_vuint16_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vint16
 ****************************************************************************/

int nxscope_put_vint16(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int16_t *val, uint8_t d)
{
  return nxscope_put_vint16_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vuint32
 ****************************************************************************/

int nxscope_put_vuint32(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint32_t *val, uint8_t d)
{
  return nxscope_put_vuint32_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vint32
 ****************************************************************************/

int nxscope_put_vint32(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int32_t *val, uint8_t d)
{
  return nxscope_put_vint32_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vuint64
 ****************************************************************************/

int nxscope_put_vuint64(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint64_t *val, uint8_t d)
{
  return nxscope_put_vuint64_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vint64
 ****************************************************************************/

int nxscope_put_vint64(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int64_t *val, uint8_t d)
{
  return nxscope_put_vint64_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vfloat
 ****************************************************************************/

int nxscope_put_vfloat(FAR struct nxscope_s *s, uint8_t ch,
                       FAR float *val, uint8_t d)
{
  return nxscope_put_vfloat_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vdouble
 ****************************************************************************/

int nxscope_put_vdouble(FAR struct nxscope_s *s, uint8_t ch,
                        FAR double *val, uint8_t d)
{
  return nxscope_put_vdouble_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vub8
 ****************************************************************************/

int nxscope_put_vub8(FAR struct nxscope_s *s, uint8_t ch,
                     FAR ub8_t *val, uint8_t d)
{
  return nxscope_put_vub8_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vb8
 ****************************************************************************/

int nxscope_put_vb8(FAR struct nxscope_s *s, uint8_t ch,
                    FAR b8_t *val, uint8_t d)
{
  return nxscope_put_vb8_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vub16
 ****************************************************************************/

int nxscope_put_vub16(FAR struct nxscope_s *s, uint8_t ch,
                      FAR ub16_t *val, uint8_t d)
{
  return nxscope_put_vub16_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vb16
 ****************************************************************************/

int nxscope_put_vb16(FAR struct nxscope_s *s, uint8_t ch,
                     FAR b16_t *val, uint8_t d)
{
  return nxscope_put_vb16_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vub32
 ****************************************************************************/

int nxscope_put_vub32(FAR struct nxscope_s *s, uint8_t ch,
                      FAR ub32_t *val, uint8_t d)
{
  return nxscope_put_vub32_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vb32
 ****************************************************************************/

int nxscope_put_vb32(FAR struct nxscope_s *s, uint8_t ch,
                     FAR b32_t *val, uint8_t d)
{
  return nxscope_put_vb32_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_vchar
 *
 * NOTE: if a given string is shorten than initialized channel vdim,
 *       we put only string bytes + '\0'
 *
 ****************************************************************************/

int nxscope_put_vchar(FAR struct nxscope_s *s, uint8_t ch,
                      FAR char *val, uint8_t d)
{
  return nxscope_put_vchar_m(s, ch, val, d, NULL, 0);
}

/****************************************************************************
 * Name: nxscope_put_XXXX_m
 *
 * Description:
 *   Put a point with metadata on the stream buffer
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   ch   - a channel id
 *   val  - sample data
 *   meta - a pointer to metadata
 *   mlen - a length of metadata
 *
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_put_uint8_m
 ****************************************************************************/

int nxscope_put_uint8_m(FAR struct nxscope_s *s, uint8_t ch, uint8_t val,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vuint8_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_int8_m
 ****************************************************************************/

int nxscope_put_int8_m(FAR struct nxscope_s *s, uint8_t ch, int8_t val,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vint8_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_uint16_m
 ****************************************************************************/

int nxscope_put_uint16_m(FAR struct nxscope_s *s, uint8_t ch, uint16_t val,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vuint16_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_int16_m
 ****************************************************************************/

int nxscope_put_int16_m(FAR struct nxscope_s *s, uint8_t ch, int16_t val,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vint16_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_uint32_m
 ****************************************************************************/

int nxscope_put_uint32_m(FAR struct nxscope_s *s, uint8_t ch, uint32_t val,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vuint32_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_int32_m
 ****************************************************************************/

int nxscope_put_int32_m(FAR struct nxscope_s *s, uint8_t ch, int32_t val,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vint32_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_uint64_m
 ****************************************************************************/

int nxscope_put_uint64_m(FAR struct nxscope_s *s, uint8_t ch, uint64_t val,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vuint64_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_int64_m
 ****************************************************************************/

int nxscope_put_int64_m(FAR struct nxscope_s *s, uint8_t ch, int64_t val,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vint64_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_float_m
 ****************************************************************************/

int nxscope_put_float_m(FAR struct nxscope_s *s, uint8_t ch, float val,
                        FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vfloat_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_double_m
 ****************************************************************************/

int nxscope_put_double_m(FAR struct nxscope_s *s, uint8_t ch, double val,
                         FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vdouble_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_b8_m
 ****************************************************************************/

int nxscope_put_b8_m(FAR struct nxscope_s *s, uint8_t ch, b8_t val,
                     FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vb8_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_b16_m
 ****************************************************************************/

int nxscope_put_b16_m(FAR struct nxscope_s *s, uint8_t ch, b16_t val,
                      FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vb16_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_ub8_m
 ****************************************************************************/

int nxscope_put_ub8_m(FAR struct nxscope_s *s, uint8_t ch, ub8_t val,
                      FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vub8_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_b32_m
 ****************************************************************************/

int nxscope_put_b32_m(FAR struct nxscope_s *s, uint8_t ch, b32_t val,
                      FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vb32_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_ub16_m
 ****************************************************************************/

int nxscope_put_ub16_m(FAR struct nxscope_s *s, uint8_t ch, ub16_t val,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vub16_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_ub32_m
 ****************************************************************************/

int nxscope_put_ub32_m(FAR struct nxscope_s *s, uint8_t ch, ub32_t val,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vub32_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_char_m
 ****************************************************************************/

int nxscope_put_char_m(FAR struct nxscope_s *s, uint8_t ch, char val,
                       FAR uint8_t *meta, uint8_t mlen)
{
  return nxscope_put_vchar_m(s, ch, &val, 1, meta, mlen);
}

/****************************************************************************
 * Name: nxscope_put_none_m
 *
 * Description:
 *   Put metadata only on the stream buffer
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   ch   - a channel id
 *   meta - a pointer to metadata
 *   mlen - a length of metadata
 *
 ****************************************************************************/

int nxscope_put_none_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR uint8_t *meta, uint8_t mlen)
{
  /* Put metadata with vector dim = 0 */

  return nxscope_put_vuint8_m(s, ch, 0, 0, meta, mlen);
}

#ifdef CONFIG_LOGGING_NXSCOPE_USERTYPES
/****************************************************************************
 * Name: nxscope_put_user_m
 *
 * Description:
 *   Put a user specific data on the stream buffer
 *
 * Input Parameters:
 *   s    - a pointer to a nxscope instance
 *   type - a channel type (starts from NXSCOPE_TYPE_USER)
 *   ch   - a channel id
 *   val  - a pointer to a sample data vector
 *   d    - a dimmention of sample data vector
 *   meta - a pointer to metadata
 *   mlen - a length of metadata
 *
 ****************************************************************************/

int nxscope_put_user_m(FAR struct nxscope_s *s, uint8_t ch,
                       uint8_t type,
                       FAR uint8_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen)
{
  /* User specific type */

  return nxscope_put_common_m(s, type, ch, val, d, meta, mlen);
}
#endif

/****************************************************************************
 * Name: nxscope_put_user_m
 *
 * Description:
 *   Put a point on the stream buffer
 *
 * Input Parameters:
 *   s   - a pointer to a nxscope instance
 *   ch  - a channel id
 *   val - sample data
 *
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_put_uint8
 ****************************************************************************/

int nxscope_put_uint8(FAR struct nxscope_s *s, uint8_t ch, uint8_t val)
{
  return nxscope_put_vuint8(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_int8
 ****************************************************************************/

int nxscope_put_int8(FAR struct nxscope_s *s, uint8_t ch, int8_t val)
{
  return nxscope_put_vint8(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_uint16
 ****************************************************************************/

int nxscope_put_uint16(FAR struct nxscope_s *s, uint8_t ch, uint16_t val)
{
  return nxscope_put_vuint16(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_int16
 ****************************************************************************/

int nxscope_put_int16(FAR struct nxscope_s *s, uint8_t ch, int16_t val)
{
  return nxscope_put_vint16(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_uint32
 ****************************************************************************/

int nxscope_put_uint32(FAR struct nxscope_s *s, uint8_t ch, uint32_t val)
{
  return nxscope_put_vuint32(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_int32
 ****************************************************************************/

int nxscope_put_int32(FAR struct nxscope_s *s, uint8_t ch, int32_t val)
{
  return nxscope_put_vint32(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_uint64
 ****************************************************************************/

int nxscope_put_uint64(FAR struct nxscope_s *s, uint8_t ch, uint64_t val)
{
  return nxscope_put_vuint64(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_int64
 ****************************************************************************/

int nxscope_put_int64(FAR struct nxscope_s *s, uint8_t ch, int64_t val)
{
  return nxscope_put_vint64(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_float
 ****************************************************************************/

int nxscope_put_float(FAR struct nxscope_s *s, uint8_t ch, float val)
{
  return nxscope_put_vfloat(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_double
 ****************************************************************************/

int nxscope_put_double(FAR struct nxscope_s *s, uint8_t ch, double val)
{
  return nxscope_put_vdouble(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_ub8
 ****************************************************************************/

int nxscope_put_ub8(FAR struct nxscope_s *s, uint8_t ch, ub8_t val)
{
  return nxscope_put_vub8(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_b8
 ****************************************************************************/

int nxscope_put_b8(FAR struct nxscope_s *s, uint8_t ch, b8_t val)
{
  return nxscope_put_vb8(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_ub16
 ****************************************************************************/

int nxscope_put_ub16(FAR struct nxscope_s *s, uint8_t ch, ub16_t val)
{
  return nxscope_put_vub16(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_b16
 ****************************************************************************/

int nxscope_put_b16(FAR struct nxscope_s *s, uint8_t ch, b16_t val)
{
  return nxscope_put_vb16(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_ub32
 ****************************************************************************/

int nxscope_put_ub32(FAR struct nxscope_s *s, uint8_t ch, ub32_t val)
{
  return nxscope_put_vub32(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_b32
 ****************************************************************************/

int nxscope_put_b32(FAR struct nxscope_s *s, uint8_t ch, b32_t val)
{
  return nxscope_put_vb32(s, ch, &val, 1);
}

/****************************************************************************
 * Name: nxscope_put_char
 ****************************************************************************/

int nxscope_put_char(FAR struct nxscope_s *s, uint8_t ch, char val)
{
  return nxscope_put_vchar(s, ch, &val, 1);
}
