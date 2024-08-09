/****************************************************************************
 * apps/include/logging/nxscope/nxscope.h
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

#ifndef __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_H
#define __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <pthread.h>
#include <stdint.h>

#include <logging/nxscope/nxscope_chan.h>
#include <logging/nxscope/nxscope_intf.h>
#include <logging/nxscope/nxscope_proto.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXSCOPE_ENABLE_LEN    (sizeof(struct nxscope_enable_data_s))
#define NXSCOPE_DIV_LEN       (sizeof(struct nxscope_div_data_s))
#define NXSCOPE_START_LEN     (sizeof(struct nxscope_start_data_s))

/* MSB bit in the channel type means a critical channel */

#define NXSCOPE_IS_CRICHAN(chtype) (chtype & 0x80)

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

/* Nxscope header ID */

enum nxscope_hdr_id_e
{
  NXSCOPE_HDRID_UNDEF   = 0,             /* Reserved */

  /* Stream frames */

  NXSCOPE_HDRID_STREAM  = 1,             /* Stream data */

  /* Get frames */

  NXSCOPE_HDRID_CMNINFO = 2,             /* Get nxscope common info */
  NXSCOPE_HDRID_CHINFO  = 3,             /* Get nxscope channel info */

  /* Special frames */

  NXSCOPE_HDRID_ACK     = 4,             /* ACK/NACK */

  /* Set frames.
   *
   * If CONFIG_LOGGING_NXSCOPE_ACKFRAMES=y this requests must
   * be confirmed with ACK frames to report success or failure.
   */

  NXSCOPE_HDRID_START   = 5,             /* Start/stop stream.
                                          *   NOTE: this frame do not follow
                                          *   'struct nxscope_set_frame_s' format
                                          */
  NXSCOPE_HDRID_ENABLE  = 6,             /* Snable/disable channels */
  NXSCOPE_HDRID_DIV     = 7,             /* Channels divider */

  /* User defined frames.
   * Must be alway the last element.
   */

  NXSCOPE_HDRID_USER   = 8
};

/* Nxscope flags */

enum nxscope_info_flags_e
{
  NXSCOPE_FLAGS_DIVIDER_SUPPORT   = (1 << 0),
  NXSCOPE_FLAGS_ACK_SUPPORT       = (1 << 1),
  NXSCOPE_FLAGS_RES2              = (1 << 2),
  NXSCOPE_FLAGS_RES3              = (1 << 3),
  NXSCOPE_FLAGS_RES4              = (1 << 4),
  NXSCOPE_FLAGS_RES5              = (1 << 5),
  NXSCOPE_FLAGS_RES6              = (1 << 6),
  NXSCOPE_FLAGS_RES7              = (1 << 7),
};

/* Nxscope stream flags */

enum nxscope_stream_flags_s
{
  NXSCOPE_STREAM_FLAGS_OVERFLOW = (1 << 0)
};

/* Nxscope start frame data */

begin_packed_struct struct nxscope_start_data_s
{
  uint8_t  start;                        /* Start/stop flag */
} end_packed_struct;

/* Nxscope enable channel data */

begin_packed_struct struct nxscope_enable_ch_data_s
{
  uint8_t en;                            /* Channel enable */
} end_packed_struct;

/* Nxscope divider channel data */

begin_packed_struct struct nxscope_div_ch_data_s
{
  uint8_t div;                           /* Channel divider - starts from 0 */
} end_packed_struct;

/* Nxscope enable frame data */

begin_packed_struct struct nxscope_enable_data_s
{
  struct nxscope_enable_ch_data_s ch[1]; /* Chmax elements */
} end_packed_struct;

/* Nxscope divider frame data */

begin_packed_struct struct nxscope_div_data_s
{
  struct nxscope_div_ch_data_s ch[1];    /* Chmax elements */
} end_packed_struct;

/* Set channel frame */

enum nxscope_set_frame_req_s
{
  NXSCOPE_SET_REQ_SINGLE = 0,   /* Single channel request */
  NXSCOPE_SET_REQ_BULK   = 1,   /* Set different values for all channels */
  NXSCOPE_SET_REQ_ALL    = 2    /* Set one value for all channles  */
};

/* Set frame common data */

begin_packed_struct struct nxscope_set_frame_s
{
  uint8_t req;                  /* Request type */
  uint8_t chan;                 /* Channel id */
  uint8_t data[1];              /* n bytes data - depends on the request type */
} end_packed_struct;

/* Chanel type */

begin_packed_struct struct nxscope_chinfo_type_s
{
  uint8_t dtype:5;                       /* Data type */
  uint8_t _res:2;                        /* Reserved for future use */
  uint8_t cri:1;                         /* Criticial channel - no buffering */
} end_packed_struct;

/* Chanel type union */

union nxscope_chinfo_type_u
{
  struct nxscope_chinfo_type_s s;
  uint8_t                      u8;
};

/* Nxscope channel info */

begin_packed_struct struct nxscope_chinfo_s
{
  uint8_t                     enable;    /* Enable flag */
  union nxscope_chinfo_type_u type;      /* Channel data type */
  uint8_t                     vdim;      /* Vector dimention */
  uint8_t                     div;       /* Divider - starts from 0 */
  uint8_t                     mlen;      /* Metadata size */
  FAR char                   *name;      /* Chanel name */
} end_packed_struct;

/* Nxscope info common */

begin_packed_struct struct nxscope_info_cmn_s
{
  uint8_t chmax;                         /* Supported channels */
  uint8_t flags;                         /* Flags (enum nxscope_info_flags_e) */
  uint8_t rx_padding;                    /* RX padding (>0 if used) */
} end_packed_struct;

/* Nxscope sample hdr:
 *
 *   +----------+-------------+----------+
 *   | channel  | sample data | metadata |
 *   +----------+-------------+----------+
 *   | 1B       | n bytes [1] | m bytes  |
 *   +----------+-------------+----------+
 *
 *   [1] - sizeof(channel_type) * channel_vdim
 *         NOTE: sample data always little-endian !
 *
 */

struct nxscope_sample_s
{
  uint8_t chan;                          /* 1 byte: Channel id - starts from 0 */
                                         /* n bytes: Data */
                                         /* m bytes: Metadata */
};

/* Nxscope stream data:
 *
 *   +----------+--------------+
 *   | flags    | samples data |
 *   +----------+--------------+
 *   | 1B       | n bytes      |
 *   +----------+--------------+
 *
 */

struct nxscope_stream_s
{
  uint8_t                 flags;             /* stream flags */
  struct nxscope_sample_s samples[1];        /* stream samples */
};

/* Nxscope callbacks */

struct nxscope_callbacks_s
{
  /* User-defined id callback */

  FAR void *userid_priv;
  CODE int (*userid)(FAR void *priv, uint8_t id, FAR uint8_t *buff);

  /* Start request callback */

  FAR void *start_priv;
  CODE int (*start)(FAR void *priv, bool start);
};

/* Nxscope general configuration */

struct nxscope_cfg_s
{
  /* NOTE: It is possible to configure separate interface
   * and protocol for control commands and data stream.
   */

  /* Interface implementation for commands */

  FAR struct nxscope_intf_s *intf_cmd;

  /* Interface implementation for stream data */

  FAR struct nxscope_intf_s *intf_stream;

  /* Protocol implementation for commands */

  FAR struct nxscope_proto_s *proto_cmd;

  /* Protocol implementation for stream data */

  FAR struct nxscope_proto_s *proto_stream;

  /* Callbacks */

  FAR struct nxscope_callbacks_s *callbacks;

  /* Number of suppoted channels */

  uint8_t channels;

  /* Stream buffer len */

  size_t streambuf_len;

  /* RX buffer len */

  size_t rxbuf_len;

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  /* Critical buffer len.
   *
   * It's the user's responsibility to correctly choose this value.
   * The minimal buffer size for critical channels can be calculate
   * with this formula:
   *    buff_size = max(proto_stream->hdrlen + 1 + type_size * vdim +
   *                    meta_len + proto_stream->footlen)
   *
   *    where max() means the maximum value from all initialized critical
   *    channels.
   *
   *  When CONFIG_DEBUG_FEATURES is enabled the correct buffer size is
   *  verified in run-time.
   */

  size_t cribuf_len;
#endif

  /* RX padding.
   *
   * This option will be provided for client in common info data
   * and can be useful if we use DMA for receiving frames
   */

  uint8_t rx_padding;
};

/* Nxscope data */

struct nxscope_s
{
  /* Nxscope interface handlers */

  FAR struct nxscope_intf_s   *intf_cmd;
  FAR struct nxscope_intf_s   *intf_stream;

  /* Nxscope protocol handlers */

  FAR struct nxscope_proto_s  *proto_cmd;
  FAR struct nxscope_proto_s  *proto_stream;

  /* Callbacks */

  FAR struct nxscope_callbacks_s *callbacks;

  /* Nxscope common info */

  struct nxscope_info_cmn_s    cmninfo;

  /* Channels info, chmax elements */

  FAR struct nxscope_chinfo_s *chinfo;
  size_t                       chinfo_size;

  /* Nxscope data */

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  FAR uint32_t                *cntr;
#endif
  uint8_t                      start;

  /* Stream data */

  FAR uint8_t                 *streambuf;
  size_t                       streambuf_len;
  size_t                       stream_i;
  bool                         stream_retry;

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  /* Critical buffer data */

  FAR uint8_t                 *cribuf;
  size_t                       cribuf_len;
#endif

  /* RX data buffer */

  FAR uint8_t                 *rxbuf;
  size_t                       rxbuf_len;
  size_t                       rxbuf_i;

  /* TX data buffer */

  FAR uint8_t                 *txbuf;
  size_t                       txbuf_len;

  /* Exclusive access */

  pthread_mutex_t              lock;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_init
 *
 * Description:
 *   Initialize a nxscope instance
 *
 * Input Parameters:
 *   s   - a pointer to a nxscope instance
 *   cfg - a pointer to a nxscope configuration data
 *
 ****************************************************************************/

int nxscope_init(FAR struct nxscope_s *s, FAR struct nxscope_cfg_s *cfg);

/****************************************************************************
 * Name: nxscope_deinit
 *
 * Description:
 *   De-initialize a nxscope instance
 *
 * Input Parameters:
 *   s - a pointer to a nxscope instance
 *
 ****************************************************************************/

void nxscope_deinit(FAR struct nxscope_s *s);

/****************************************************************************
 * Name: nxscope_lock
 *
 * Description:
 *   Lock a nxscope instance
 *
 * Input Parameters:
 *   s - a pointer to a nxscope instance
 *
 ****************************************************************************/

void nxscope_lock(FAR struct nxscope_s *s);

/****************************************************************************
 * Name: nxscope_unlock
 *
 * Description:
 *   Unlock a nxscope instance
 *
 * Input Parameters:
 *   s - a pointer to a nxscope instance
 *
 ****************************************************************************/

void nxscope_unlock(FAR struct nxscope_s *s);

/****************************************************************************
 * Name: nxscope_stream
 *
 * Description:
 *   Send nxscope stream data.
 *
 *   NOTE: It's the user's responsibility to periodically call this function.
 *
 * Input Parameters:
 *   s - a pointer to a nxscope instance
 *
 ****************************************************************************/

int nxscope_stream(FAR struct nxscope_s *s);

/****************************************************************************
 * Name: nxscope_recv
 *
 * Description:
 *   Receive and handle nxscope protocol data.
 *
 *   NOTE: It's the user's responsibility to periodically call this function.
 *
 * Input Parameters:
 *   s - a pointer to a nxscope instance
 *
 ****************************************************************************/

int nxscope_recv(FAR struct nxscope_s *s);

/****************************************************************************
 * Name: nxscope_stream_start
 *
 * Description:
 *   Start/stop data stream
 *
 * Input Parameters:
 *   s     - a pointer to a nxscope instance
 *   start - start/stop
 *
 ****************************************************************************/

int nxscope_stream_start(FAR struct nxscope_s *s, bool start);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_LOGGING_NXSCOPE_NXSCOPE_H */
