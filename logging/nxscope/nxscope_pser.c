/****************************************************************************
 * apps/logging/nxscope/nxscope_pser.c
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

#include <nuttx/crc16.h>

#include <logging/nxscope/nxscope.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXSCOPE_HDR_LEN       (sizeof(struct nxscope_hdr_s))
#define NXSCOPE_DATA_START    (NXSCOPE_HDR_LEN)
#define NXSCOPE_BASEFRAME_LEN (NXSCOPE_HDR_LEN + NXSCOPE_CRC_LEN)
#define NXSCOPE_HDR_SOF       (0x55)
#define NXSCOPE_CRC_LEN       (sizeof(uint16_t))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Nxscope serial protocol frame:
 * +--------------------+------------+-------------+
 * | HDR [4B]           | frame data | footer [2B] |
 * +-----+---------+----+------------+-------------+
 * | SOF | len [1] | id | frame data | crc16 [2]   |
 * +-----+----+----+----+------------+------+------+
 * | 0   | 1  | 2  | 3  | ...        | n+1  | n+2  |
 * +-----+----+----+----+------------+------+------+
 *
 * [1] - always little-endian
 * [2] - always big-endian
 */

/* Nxscope header */

begin_packed_struct struct nxscope_hdr_s
{
  uint8_t  sof;                 /* SOF */
  uint16_t len;                 /* Frame length */
  uint8_t  id;                  /* Frame ID */
} end_packed_struct;

/* Nxscope footer */

begin_packed_struct struct nxscope_footer_s
{
  uint16_t crc16;               /* check sum (see nxscope_frame_final()) */
} end_packed_struct;

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

static int nxscope_frame_get(FAR struct nxscope_proto_s *p,
                             FAR uint8_t *buff, size_t len,
                             FAR struct nxscope_frame_s *frame);
static int nxscope_frame_final(FAR struct nxscope_proto_s *p,
                               uint8_t id,
                               FAR uint8_t *buff, FAR size_t *len);

/****************************************************************************
 * Public Data
 ****************************************************************************/

static struct nxscope_proto_ops_s g_nxscope_proto_ser_ops =
{
  nxscope_frame_get,
  nxscope_frame_final,
};

static struct nxscope_proto_s g_nxscope_proto_ser =
{
  false,
  NULL,
  &g_nxscope_proto_ser_ops,
  (size_t)NXSCOPE_HDR_LEN,
  (size_t)NXSCOPE_CRC_LEN
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_hdr_fill
 ****************************************************************************/

static void nxscope_hdr_fill(FAR uint8_t *buff, uint8_t id, uint16_t len)
{
  FAR struct nxscope_hdr_s *hdr = NULL;

  DEBUGASSERT(buff);

  hdr = (FAR struct nxscope_hdr_s *)buff;

  hdr->sof = NXSCOPE_HDR_SOF;

  /* Length always as little endian */

  hdr->len = htole16(len);
  hdr->id  = id;
}

/****************************************************************************
 * Name: nxscope_frame_get
 ****************************************************************************/

static int nxscope_frame_get(FAR struct nxscope_proto_s *p,
                             FAR uint8_t *buff, size_t len,
                             FAR struct nxscope_frame_s *frame)
{
  FAR struct nxscope_hdr_s *hdr = NULL;
  uint16_t                  crc = 0;
  int                       ret = OK;
  int                       i   = 0;

  DEBUGASSERT(p);
  DEBUGASSERT(buff);
  DEBUGASSERT(frame);

  UNUSED(p);

  /* Find SOF */

  for (i = 0; i < len - NXSCOPE_HDR_LEN; i++)
    {
      hdr = (FAR struct nxscope_hdr_s *)&buff[i];

      /* Verify SOF */

      if (hdr->sof == NXSCOPE_HDR_SOF)
        {
          break;
        }
    }

  /* Check for no header */

  if (hdr == NULL)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Check for no SOF in header */

  if (hdr->sof != NXSCOPE_HDR_SOF)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Verify len - always little-endian */

  hdr->len = htole16(hdr->len);

  if ((len - i) < hdr->len)
    {
      ret = -EINVAL;
      goto errout;
    }

  /* Verify crc16 for the whole frame */

  crc = crc16xmodem(&buff[i], hdr->len);
  if (crc != 0)
    {
      _err("ERROR: invalid crc16 %d\n", crc);
      ret = -EINVAL;
      goto errout;
    }

  /* Return the frame data */

  frame->id   = hdr->id;
  frame->data = &buff[i + NXSCOPE_DATA_START];
  frame->drop = hdr->len + i;
  frame->dlen = hdr->len - NXSCOPE_BASEFRAME_LEN;

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_frame_final
 ****************************************************************************/

static int nxscope_frame_final(FAR struct nxscope_proto_s *p,
                               uint8_t id,
                               FAR uint8_t *buff, FAR size_t *len)
{
  uint16_t crc = 0;
  int      ret = OK;

  DEBUGASSERT(p);
  DEBUGASSERT(buff);
  DEBUGASSERT(len);

  if (*len <= NXSCOPE_HDR_LEN)
    {
      /* No data */

      ret = -ENODATA;
      goto errout;
    }

  /* Fill hdr */

  nxscope_hdr_fill(buff, id, *len + NXSCOPE_CRC_LEN);

  /* Add crc16 - always as big-endian.
   *
   * crc16 xmodem:
   *   polynominal     = 0x1021
   *   initial value   = 0x0000
   *   final xor value = 0x0000
   */

  crc = crc16xmodem(buff, *len);

#ifdef CONFIG_ENDIAN_BIG
  buff[(*len)++] = (crc >> 0) & 0xff;
  buff[(*len)++] = (crc >> 8) & 0xff;
#else
  buff[(*len)++] = (crc >> 8) & 0xff;
  buff[(*len)++] = (crc >> 0) & 0xff;
#endif

errout:
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_proto_ser_init
 ****************************************************************************/

int nxscope_proto_ser_init(FAR struct nxscope_proto_s *proto, FAR void *cfg)
{
  DEBUGASSERT(proto);

  /* cfg argument not used, but keept here for compatibility with
   * future protocol implementations.
   */

  UNUSED(cfg);

  /* Just copy protocol data */

  memcpy(proto, &g_nxscope_proto_ser, sizeof(struct nxscope_proto_s));

  /* Initialized */

  proto->initialized = true;

  return OK;
}

/****************************************************************************
 * Name: nxscope_proto_ser_deinit
 ****************************************************************************/

void nxscope_proto_ser_deinit(FAR struct nxscope_proto_s *proto)
{
  DEBUGASSERT(proto);

  /* Nothings here */

  UNUSED(proto);
}
