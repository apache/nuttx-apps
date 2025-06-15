/****************************************************************************
 * apps/include/logging/nxscope/nxscope_chan.h
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

#ifndef __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_CHAN_H
#define __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_CHAN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <fixedmath.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* Nxscope sample type */

enum nxscope_sample_dtype_e
{
  /* Default numerical types */

  NXSCOPE_TYPE_UNDEF  = 0,
  NXSCOPE_TYPE_NONE   = 1,
  NXSCOPE_TYPE_UINT8  = 2,
  NXSCOPE_TYPE_INT8   = 3,
  NXSCOPE_TYPE_UINT16 = 4,
  NXSCOPE_TYPE_INT16  = 5,
  NXSCOPE_TYPE_UINT32 = 6,
  NXSCOPE_TYPE_INT32  = 7,
  NXSCOPE_TYPE_UINT64 = 8,
  NXSCOPE_TYPE_INT64  = 9,
  NXSCOPE_TYPE_FLOAT  = 10,
  NXSCOPE_TYPE_DOUBLE = 11,
  NXSCOPE_TYPE_UB8    = 12,
  NXSCOPE_TYPE_B8     = 13,
  NXSCOPE_TYPE_UB16   = 14,
  NXSCOPE_TYPE_B16    = 15,
  NXSCOPE_TYPE_UB32   = 16,
  NXSCOPE_TYPE_B32    = 17,

  /* Char/string data */

  NXSCOPE_TYPE_CHAR   = 18,

#if 0
  /* Reserved for future use */

  NXSCOPE_TYPE_WCHAR  = 19,
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_USERTYPES
  /* User defined types starts from here.
   * NXSCOPE_TYPE_USER must be always the last element.
   *
   * Type of size 1B. Together with channel.vdim this
   * can be used to send custom protocol data.
   */

  NXSCOPE_TYPE_USER   = 20,
#endif

  /* 5 bits reserved for data type */

  NXSCOPE_TYPE_LAST   = 31,
};

/* Forward declaration */

struct nxscope_s;

/****************************************************************************
 * Public Function Puttypes
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

int nxscope_chan_init(FAR struct nxscope_s *s, uint8_t ch,
                      FAR char *name, uint8_t type, uint8_t vdim,
                      uint8_t mlen);

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

int nxscope_chan_en(FAR struct nxscope_s *s, uint8_t chan, bool en);

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

int nxscope_chan_div(FAR struct nxscope_s *s, uint8_t chan, uint8_t div);
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

int nxscope_chan_all_en(FAR struct nxscope_s *s, bool en);

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

int nxscope_put_vuint8_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR uint8_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vint8_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR int8_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vuint16_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint16_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vint16_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int16_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vuint32_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint32_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vint32_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int32_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vuint64_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR uint64_t *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vint64_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR int64_t *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vfloat_m(FAR struct nxscope_s *s, uint8_t ch,
                         FAR float *val, uint8_t d,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vdouble_m(FAR struct nxscope_s *s, uint8_t ch,
                          FAR double *val, uint8_t d,
                          FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vub8_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR ub8_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vb8_m(FAR struct nxscope_s *s, uint8_t ch,
                      FAR b8_t *val, uint8_t d,
                      FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vub16_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR ub16_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vb16_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR b16_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vub32_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR ub32_t *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vb32_m(FAR struct nxscope_s *s, uint8_t ch,
                       FAR b32_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_vchar_m(FAR struct nxscope_s *s, uint8_t ch,
                        FAR char *val, uint8_t d,
                        FAR uint8_t *meta, uint8_t mlen);

/****************************************************************************
 * Name: nxscope_put_vXXXX
 *
 * Description:
 *   Put a vector on the stream buffer
 *
 * Input Parameters:
 *   s   - a pointer to a nxscope instance
 *   ch  - a channel id
 *   val - a pointer to a sample data vector
 *   d   - a dimmention of sample data vector
 *
 ****************************************************************************/

int nxscope_put_vuint8(FAR struct nxscope_s *s, uint8_t ch,
                       FAR uint8_t *val, uint8_t d);
int nxscope_put_vint8(FAR struct nxscope_s *s, uint8_t ch,
                      FAR int8_t *val, uint8_t d);
int nxscope_put_vuint16(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint16_t *val, uint8_t d);
int nxscope_put_vint16(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int16_t *val, uint8_t d);
int nxscope_put_vuint32(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint32_t *val, uint8_t d);
int nxscope_put_vint32(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int32_t *val, uint8_t d);
int nxscope_put_vuint64(FAR struct nxscope_s *s, uint8_t ch,
                        FAR uint64_t *val, uint8_t d);
int nxscope_put_vint64(FAR struct nxscope_s *s, uint8_t ch,
                       FAR int64_t *val, uint8_t d);
int nxscope_put_vfloat(FAR struct nxscope_s *s, uint8_t ch,
                       FAR float *val, uint8_t d);
int nxscope_put_vdouble(FAR struct nxscope_s *s, uint8_t ch,
                        FAR double *val, uint8_t d);
int nxscope_put_vub8(FAR struct nxscope_s *s, uint8_t ch,
                     FAR ub8_t *val, uint8_t d);
int nxscope_put_vb8(FAR struct nxscope_s *s, uint8_t ch,
                    FAR b8_t *val, uint8_t d);
int nxscope_put_vub16(FAR struct nxscope_s *s, uint8_t ch,
                      FAR ub16_t *val, uint8_t d);
int nxscope_put_vb16(FAR struct nxscope_s *s, uint8_t ch,
                     FAR b16_t *val, uint8_t d);
int nxscope_put_vub32(FAR struct nxscope_s *s, uint8_t ch,
                      FAR ub32_t *val, uint8_t d);
int nxscope_put_vb32(FAR struct nxscope_s *s, uint8_t ch,
                     FAR b32_t *val, uint8_t d);
int nxscope_put_vchar(FAR struct nxscope_s *s, uint8_t ch,
                      FAR char *val, uint8_t d);

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

int nxscope_put_uint8_m(FAR struct nxscope_s *s, uint8_t ch, uint8_t val,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_int8_m(FAR struct nxscope_s *s, uint8_t ch, int8_t val,
                      FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_uint16_m(FAR struct nxscope_s *s, uint8_t ch, uint16_t val,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_int16_m(FAR struct nxscope_s *s, uint8_t ch, int16_t val,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_uint32_m(FAR struct nxscope_s *s, uint8_t ch, uint32_t val,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_int32_m(FAR struct nxscope_s *s, uint8_t ch, int32_t val,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_uint64_m(FAR struct nxscope_s *s, uint8_t ch, uint64_t val,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_int64_m(FAR struct nxscope_s *s, uint8_t ch, int64_t val,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_float_m(FAR struct nxscope_s *s, uint8_t ch, float val,
                        FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_double_m(FAR struct nxscope_s *s, uint8_t ch, double val,
                         FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_ub8_m(FAR struct nxscope_s *s, uint8_t ch, ub8_t val,
                      FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_b8_m(FAR struct nxscope_s *s, uint8_t ch, b8_t val,
                     FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_ub16_m(FAR struct nxscope_s *s, uint8_t ch, ub16_t val,
                       FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_b16_m(FAR struct nxscope_s *s, uint8_t ch, b16_t val,
                      FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_ub32_m(FAR struct nxscope_s *s, uint8_t ch, ub32_t val,
                       FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_b32_m(FAR struct nxscope_s *s, uint8_t ch, b32_t val,
                      FAR uint8_t *meta, uint8_t mlen);
int nxscope_put_char_m(FAR struct nxscope_s *s, uint8_t ch, char val,
                       FAR uint8_t *meta, uint8_t mlen);

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
                       FAR uint8_t *meta, uint8_t mlen);

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

int nxscope_put_user_m(FAR struct nxscope_s *s, uint8_t ch, uint8_t type,
                       FAR uint8_t *val, uint8_t d,
                       FAR uint8_t *meta, uint8_t mlen);
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

int nxscope_put_uint8(FAR struct nxscope_s *s, uint8_t ch, uint8_t val);
int nxscope_put_int8(FAR struct nxscope_s *s, uint8_t ch, int8_t val);
int nxscope_put_uint16(FAR struct nxscope_s *s, uint8_t ch, uint16_t val);
int nxscope_put_int16(FAR struct nxscope_s *s, uint8_t ch, int16_t val);
int nxscope_put_uint32(FAR struct nxscope_s *s, uint8_t ch, uint32_t val);
int nxscope_put_int32(FAR struct nxscope_s *s, uint8_t ch, int32_t val);
int nxscope_put_uint64(FAR struct nxscope_s *s, uint8_t ch, uint64_t val);
int nxscope_put_int64(FAR struct nxscope_s *s, uint8_t ch, int64_t val);
int nxscope_put_float(FAR struct nxscope_s *s, uint8_t ch, float val);
int nxscope_put_double(FAR struct nxscope_s *s, uint8_t ch, double val);
int nxscope_put_ub8(FAR struct nxscope_s *s, uint8_t ch, ub8_t val);
int nxscope_put_b8(FAR struct nxscope_s *s, uint8_t ch, b8_t val);
int nxscope_put_ub16(FAR struct nxscope_s *s, uint8_t ch, ub16_t val);
int nxscope_put_b16(FAR struct nxscope_s *s, uint8_t ch, b16_t val);
int nxscope_put_ub32(FAR struct nxscope_s *s, uint8_t ch, ub32_t val);
int nxscope_put_b32(FAR struct nxscope_s *s, uint8_t ch, b32_t val);
int nxscope_put_char(FAR struct nxscope_s *s, uint8_t ch, char val);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_CHAN_H */
