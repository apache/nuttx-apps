/****************************************************************************
 * apps/logging/nxscope/nxscope.c
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <logging/nxscope/nxscope.h>

#include "nxscope_internals.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NOTE: channel name is always terminated with a null-character */

#define CHINFO_DATA_SIZE_MAX (sizeof(struct nxscope_chinfo_s) -   \
                              sizeof(char *) + CHAN_NAMELEN_MAX + 1)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_frame_send
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_frame_send(FAR struct nxscope_s *s, uint8_t id,
                              FAR uint8_t *data, size_t dlen)
{
  size_t tx_i = 0;
  int    ret  = OK;

  DEBUGASSERT(s);
  DEBUGASSERT(data);

#ifdef CONFIG_DEBUG_FEATURES
  /* Validate TX buffer space */

  if (s->txbuf_len < dlen + s->proto_cmd->footlen + s->proto_cmd->hdrlen)
    {
      ret = -ENOBUFS;
      _err("ERROR: no space in txbuf %d\n", ret);
      goto errout;
    }
#endif

  /* Offset for hdr */

  tx_i = s->proto_cmd->hdrlen;

  /* Copy data */

  memcpy(&s->txbuf[tx_i], data, dlen);
  tx_i += dlen;

  /* Finalize a new frame */

  ret = PROTO_FRAME_FINAL(s, s->proto_cmd, id, s->txbuf, &tx_i);
  if (ret < 0)
    {
      _err("ERROR: PROTO_FRAME_FINAL failed %d\n", ret);
      goto errout;
    }

  /* Send frame */

  ret = INTF_SEND(s, s->intf_cmd, s->txbuf, tx_i);
  if (ret < 0)
    {
      _err("ERROR: INTF_SEND failed %d\n", ret);
    }

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_cmninfo_send
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_cmninfo_send(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  return nxscope_frame_send(s,
                            NXSCOPE_HDRID_CMNINFO,
                            (FAR uint8_t *)&s->cmninfo,
                            sizeof(struct nxscope_info_cmn_s));
}

/****************************************************************************
 * Name: nxscope_chinfo_send
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_chinfo_send(FAR struct nxscope_s *s, uint8_t ch)
{
  uint8_t data[CHINFO_DATA_SIZE_MAX];
  size_t  namelen = 0;
  size_t  txlen   = 0;
  size_t  tmp     = 0;

  DEBUGASSERT(s);

  /* Copy channel info */

  tmp = sizeof(struct nxscope_chinfo_s) - sizeof(char *);
  memcpy(data, &s->chinfo[ch], tmp);

  namelen = strnlen(s->chinfo[ch].name, CHAN_NAMELEN_MAX);
  memcpy(&data[tmp], s->chinfo[ch].name, namelen);

  /* Treminate name wit a null-character */

  txlen = tmp  + namelen + 1;
  data[txlen - 1] = '\0';

  /* Send frame */

  return nxscope_frame_send(s, NXSCOPE_HDRID_CHINFO, data, txlen);
}

/****************************************************************************
 * Name: nxscope_enable_req
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_enable_req(FAR struct nxscope_s *s,
                              FAR struct nxscope_set_frame_s *set,
                              uint16_t dlen)
{
  FAR struct nxscope_enable_data_s *data = NULL;
  int                               ret  = OK;
  int                               i    = 0;

  DEBUGASSERT(s);
  DEBUGASSERT(set);

  /* Get data */

  data = (FAR struct nxscope_enable_data_s *) set->data;

  /* Handle set request */

  switch (set->req)
    {
      case NXSCOPE_SET_REQ_SINGLE:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_ENABLE_LEN - 1))
            {
              _err("ERROR: invalid enable single dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          /* Validate data */

          if (data->ch[0].en != 0 && data->ch[0].en != 1)
            {
              ret = -EINVAL;
              goto errout;
            }

          if (set->chan > s->cmninfo.chmax)
            {
              ret = -EINVAL;
              goto errout;
            }

          /* Write configuration */

          s->chinfo[set->chan].enable = data->ch[0].en;

          break;
        }

      case NXSCOPE_SET_REQ_BULK:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_ENABLE_LEN * s->cmninfo.chmax - 1))
            {
              _err("ERROR: invalid enable bulk dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          /* Validate data */

          for (i = 0; i < s->cmninfo.chmax; i++)
            {
              if (data->ch[i].en != 0 && data->ch[i].en != 1)
                {
                  ret = -EINVAL;
                  goto errout;
                }
            }

          /* Write configuration */

          for (i = 0; i < s->cmninfo.chmax; i++)
            {
              s->chinfo[i].enable = data->ch[i].en;
            }

          break;
        }

      case NXSCOPE_SET_REQ_ALL:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_ENABLE_LEN - 1))
            {
              _err("ERROR: invalid enable all dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          if (data->ch[0].en != 0 && data->ch[0].en != 1)
            {
              ret = -EINVAL;
              goto errout;
            }

          /* Write configuration */

          for (i = 0; i < s->cmninfo.chmax; i++)
            {
              s->chinfo[i].enable = data->ch[0].en;
            }

          break;
        }

      default:
        {
          _err("ERROR: invalid set->req=%d\n", set->req);
          ret = -EINVAL;
          break;
        }
    }

errout:
  return ret;
}

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
/****************************************************************************
 * Name: nxscope_div_req
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_div_req(FAR struct nxscope_s *s,
                           FAR struct nxscope_set_frame_s *set,
                           uint16_t dlen)
{
  FAR struct nxscope_div_data_s *data = NULL;
  int                            ret  = OK;
  int                            i    = 0;

  DEBUGASSERT(s);
  DEBUGASSERT(set);

  /* Get data */

  data = (FAR struct nxscope_div_data_s *)set->data;

  /* Handle set request */

  switch (set->req)
    {
      case NXSCOPE_SET_REQ_SINGLE:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_DIV_LEN - 1))
            {
              _err("ERROR: invalid div single dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          if (set->chan > s->cmninfo.chmax)
            {
              ret = -EINVAL;
              goto errout;
            }

          /* Write configuration */

          s->chinfo[set->chan].div = data->ch[0].div;

          break;
        }

      case NXSCOPE_SET_REQ_BULK:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_DIV_LEN * s->cmninfo.chmax - 1))
            {
              _err("ERROR: invalid div bulk dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          /* Write configuration */

          for (i = 0; i < s->cmninfo.chmax; i++)
            {
              s->chinfo[i].div = data->ch[i].div;
            }

          break;
        }

      case NXSCOPE_SET_REQ_ALL:
        {
          /* Verify request length */

          if (dlen != (sizeof(struct nxscope_set_frame_s) +
                       NXSCOPE_DIV_LEN - 1))
            {
              _err("ERROR: invalid div all dlen = %d\n", dlen);
              ret = -EINVAL;
              goto errout;
            }

          /* Write configuration */

          for (i = 0; i < s->cmninfo.chmax; i++)
            {
              s->chinfo[i].div = data->ch[0].div;
            }

          break;
        }

      default:
        {
          _err("ERROR: invalid set->req=%d\n", set->req);
          ret = -EINVAL;
          break;
        }
    }

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: nxscope_start_set
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_start_set(FAR struct nxscope_s *s, bool start)
{
  int ret = OK;

  s->start = start;

  /* User specific callback */

  if (s->callbacks != NULL && s->callbacks->start != NULL)
    {
      ret = s->callbacks->start(s->callbacks->start_priv, start);
      if (ret < 0)
        {
          _err("ERROR: s->callbacks->start failed %d\n", ret);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: nxscope_start_req
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static int nxscope_start_req(FAR struct nxscope_s *s,
                             FAR struct nxscope_start_data_s *data)
{
  int ret = -EINVAL;

  DEBUGASSERT(s);
  DEBUGASSERT(data);

  if (data->start == 0 || data->start == 1)
    {
      _info("data->start=%d\n", data->start);
      ret = nxscope_start_set(s, data->start);
    }

  return ret;
}

/****************************************************************************
 * Name: nxscope_stream_reset
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static void nxscope_stream_reset(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  /* Offset for hdr + 1 byte for flags */

  s->stream_i = s->proto_stream->hdrlen + 1;

  /* Reset flags */

  s->streambuf[s->proto_stream->hdrlen] = 0;
}

/****************************************************************************
 * Name: nxscope_stream_empty
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static bool nxscope_stream_empty(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  if (s->stream_i > s->proto_stream->hdrlen + 1)
    {
      return false;
    }
  else
    {
      return true;
    }
}

#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
/****************************************************************************
 * Name: nxscope_ack
 *
 * NOTE: This function assumes that we have exclusive access to the nxscope
 *       instance
 *
 ****************************************************************************/

static void nxscope_ack(FAR struct nxscope_s *s, int ret)
{
  DEBUGASSERT(s);

  nxscope_frame_send(s, NXSCOPE_HDRID_ACK, (FAR uint8_t *)&ret, 4);
}
#endif

/****************************************************************************
 * Name: nxscope_recv_handle
 ****************************************************************************/

static int nxscope_recv_handle(FAR struct nxscope_s *s, uint8_t id,
                               uint16_t dlen, FAR uint8_t *buf)
{
  int ret = OK;

  DEBUGASSERT(s);
  DEBUGASSERT(buf);

  nxscope_lock(s);

  switch (id)
    {
      case NXSCOPE_HDRID_CMNINFO:
        {
          _info("NXSCOPE_HDRID_CMNINFO\n");

          /* Verify request length */

          if (dlen != 0)
            {
              _err("ERROR: cmninfo request invalid dlen = %d\n", dlen);
              goto errout;
            }

          /* Send cmninfo response */

          ret = nxscope_cmninfo_send(s);

          break;
        }

      case NXSCOPE_HDRID_CHINFO:
        {
          FAR uint8_t *ch = buf;

          _info("NXSCOPE_HDRID_CHINFO %d\n", *ch);

          /* Verify request length */

          if (dlen != 1)
            {
              _err("ERROR: chinfo request invalid dlen = %d\n", dlen);
              goto errout;
            }

          /* Send chinfo response */

          ret = nxscope_chinfo_send(s, *ch);

          break;
        }

      case NXSCOPE_HDRID_START:
        {
          FAR struct nxscope_start_data_s *data =
            (FAR struct nxscope_start_data_s *)buf;

          _info("NXSCOPE_HDRID_START\n");

          /* Verify request length */

          if (dlen != NXSCOPE_START_LEN)
            {
              _err("ERROR: start request invalid dlen = %d\n", dlen);
              goto errout;
            }

          /* Start request */

          ret = nxscope_start_req(s, data);

#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
          /* Send ACK */

          nxscope_ack(s, ret);
#endif

          break;
        }

      case NXSCOPE_HDRID_ENABLE:
        {
          FAR struct nxscope_set_frame_s *data =
            (FAR struct nxscope_set_frame_s *)buf;

          _info("NXSCOPE_HDRID_ENABLE\n");

          /* Enable request */

          ret = nxscope_enable_req(s, data, dlen);

#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
          /* Send ACK */

          nxscope_ack(s, ret);
#endif

          break;
        }

      case NXSCOPE_HDRID_DIV:
        {
#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
          FAR struct nxscope_set_frame_s *data =
            (FAR struct nxscope_set_frame_s *)buf;

          _info("NXSCOPE_HDRID_DIV\n");

          /* Divider request */

          ret = nxscope_div_req(s, data, dlen);
#else
          ret = -EPERM;
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
          /* Send ACK */

          nxscope_ack(s, ret);
#endif

          break;
        }

      default:
        {
          /* User specific commands */

          if (s->callbacks != NULL && s->callbacks->userid != NULL)
            {
              ret = s->callbacks->userid(s->callbacks->userid_priv, id, buf);
              if (ret < 0)
                {
                  _err("ERROR: s->callbacks->userid failed %d\n", ret);
                }
            }
          else
            {
              _err("ERROR: unsupported id %d\n", id);
              ret = -EINVAL;
            }
        }
    }

errout:
  nxscope_unlock(s);

  return ret;
}

/****************************************************************************
 * Public Functions
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

int nxscope_init(FAR struct nxscope_s *s, FAR struct nxscope_cfg_s *cfg)
{
  int ret = OK;
  int i   = 0;

  DEBUGASSERT(s);
  DEBUGASSERT(cfg);

  /* Reset structure */

  memset(s, 0, sizeof(struct nxscope_s));

  /* Connect command interface */

  DEBUGASSERT(cfg->intf_cmd);
  DEBUGASSERT(cfg->intf_cmd->ops->send);
  DEBUGASSERT(cfg->intf_cmd->ops->recv);

  /* Must be initialized */

  if (!cfg->intf_cmd->initialized)
    {
      ret = -EINVAL;
      goto errout;
    }

  s->intf_cmd = cfg->intf_cmd;

  /* Connect command protocol handler */

  DEBUGASSERT(cfg->proto_cmd);
  DEBUGASSERT(cfg->proto_cmd->ops->frame_get);
  DEBUGASSERT(cfg->proto_cmd->ops->frame_final);

  /* Must be initialized */

  if (!cfg->proto_cmd->initialized)
    {
      ret = -EINVAL;
      goto errout;
    }

  s->proto_cmd = cfg->proto_cmd;

  /* Connect stream interface */

  DEBUGASSERT(cfg->intf_stream);
  DEBUGASSERT(cfg->intf_stream->ops->send);
  DEBUGASSERT(cfg->intf_stream->ops->recv);

  /* Must be initialized */

  if (!cfg->intf_stream->initialized)
    {
      ret = -EINVAL;
      goto errout;
    }

  s->intf_stream = cfg->intf_stream;

  /* Connect stream protocol handler */

  DEBUGASSERT(cfg->proto_stream);
  DEBUGASSERT(cfg->proto_stream->ops->frame_get);
  DEBUGASSERT(cfg->proto_stream->ops->frame_final);

  /* Must be initialized */

  if (!cfg->proto_stream->initialized)
    {
      ret = -EINVAL;
      goto errout;
    }

  s->proto_stream = cfg->proto_stream;

  /* Connect callbacks (optional) */

  s->callbacks = cfg->callbacks;

  /* Allocate memory for stream buffer */

  DEBUGASSERT(cfg->streambuf_len > 0);

  s->streambuf = zalloc(cfg->streambuf_len);
  if (s->streambuf == NULL)
    {
      ret = -errno;
      _err("ERROR: streambuf zalloc failed %d\n", ret);
      goto errout;
    }

  s->streambuf_len = cfg->streambuf_len;

  /* Allocate memory for nxscope channels info */

  DEBUGASSERT(cfg->channels > 0);

  s->chinfo_size = cfg->channels * sizeof(struct nxscope_chinfo_s);

  s->chinfo = zalloc(s->chinfo_size);
  if (s->chinfo == NULL)
    {
      ret = -errno;
      _err("ERROR: chinfo zalloc failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  /* Allocate memory for divider counters */

  s->cntr = zalloc(cfg->channels * sizeof(uint32_t));
  if (s->cntr == NULL)
    {
      ret = -errno;
      _err("ERROR: cntr zalloc failed %d\n", ret);
      goto errout;
    }
#endif

  /* Allocate memory for RX buffer */

  DEBUGASSERT(cfg->rxbuf_len > 0);
  s->rxbuf_len = cfg->rxbuf_len;

  s->rxbuf = zalloc(s->rxbuf_len);
  if (s->rxbuf == NULL)
    {
      ret = -errno;
      _err("ERROR: rxbuf zalloc failed %d\n", ret);
      goto errout;
    }

  /* Allocate memory for TX buffer.
   * We must fit the longest possible CHINFO response.
   */

  s->txbuf_len = (CHINFO_DATA_SIZE_MAX + s->proto_cmd->footlen +
                  s->proto_cmd->hdrlen);
  s->txbuf = zalloc(s->txbuf_len);
  if (s->txbuf == NULL)
    {
      ret = -errno;
      _err("ERROR: txbuf zalloc failed %d\n", ret);
      goto errout;
    }

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  /* Allocate memory for critical channels buffer */

  s->cribuf_len = cfg->cribuf_len;
  s->cribuf = zalloc(s->cribuf_len);
  if (s->cribuf == NULL)
    {
      ret = -errno;
      _err("ERROR: cribuf zalloc failed %d\n", ret);
      goto errout;
    }
#endif

  /* Initialize lock */

  ret = pthread_mutex_init(&s->lock, NULL);
  if (ret != 0)
    {
      _err("ERROR: pthread_mutex_init failed %d\n", errno);
      goto errout;
    }

  /* Reset stream buffer */

  nxscope_stream_reset(s);

  /* Initialize info data */

  s->cmninfo.chmax = cfg->channels;

  s->cmninfo.flags = 0;
#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  s->cmninfo.flags |= NXSCOPE_FLAGS_DIVIDER_SUPPORT;
#endif
#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
  s->cmninfo.flags |= NXSCOPE_FLAGS_ACK_SUPPORT;
#endif

  s->cmninfo.rx_padding = cfg->rx_padding;

  /* Initialize channels */

  for (i = 0; i < cfg->channels; i++)
    {
      s->chinfo[i].name = "\0";
    }

  return OK;

errout:

  if (s->streambuf != NULL)
    {
      free(s->streambuf);
    }

  if (s->chinfo != NULL)
    {
      free(s->chinfo);
    }

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  if (s->cntr != NULL)
    {
      free(s->cntr);
    }
#endif

  if (s->rxbuf != NULL)
    {
      free(s->rxbuf);
    }

  if (s->txbuf != NULL)
    {
      free(s->txbuf);
    }

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  if (s->cribuf != NULL)
    {
      free(s->cribuf);
    }
#endif

  return ret;
}

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

void nxscope_deinit(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  /* Free mutex */

  pthread_mutex_destroy(&s->lock);

  /* Free allocated memory */

  if (s->streambuf != NULL)
    {
      free(s->streambuf);
    }

  if (s->chinfo != NULL)
    {
      free(s->chinfo);
    }

#ifdef CONFIG_LOGGING_NXSCOPE_DIVIDER
  if (s->cntr != NULL)
    {
      free(s->cntr);
    }
#endif

#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  if (s->cribuf != NULL)
    {
      free(s->cribuf);
    }
#endif

  if (s->txbuf != NULL)
    {
      free(s->txbuf);
    }
}

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

void nxscope_lock(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  pthread_mutex_lock(&s->lock);
}

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

void nxscope_unlock(FAR struct nxscope_s *s)
{
  DEBUGASSERT(s);

  pthread_mutex_unlock(&s->lock);
}

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

int nxscope_stream(FAR struct nxscope_s *s)
{
  int ret = OK;

  DEBUGASSERT(s);

  nxscope_lock(s);

  /* Do nothing if stream not started */

  if (!s->start)
    {
      ret = OK;
      goto errout;
    }

  /* Do nothing if no data */

  if (nxscope_stream_empty(s))
    {
      goto errout;
    }

  /* Send stream data */

  ret = nxscope_stream_send(s, s->streambuf, &s->stream_i);
  if (ret < 0)
    {
      _err("ERROR: nxscope_stream_send failed %d\n", ret);
      goto errout;
    }

  /* Reset stream buffer */

  nxscope_stream_reset(s);

errout:
  nxscope_unlock(s);

  return ret;
}

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

int nxscope_recv(FAR struct nxscope_s *s)
{
  int                    ret = OK;
  size_t                 size_left = 0;
  size_t                 bytes_left = 0;
  struct nxscope_frame_s frame;

  DEBUGASSERT(s);

  do
    {
      /* Accumulate data until buffer empty */

      size_left = s->rxbuf_len - s->rxbuf_i;
      ret = INTF_RECV(s, s->intf_cmd, &s->rxbuf[s->rxbuf_i], size_left);
      if (ret <= 0)
        {
          break;
        }

      s->rxbuf_i += ret;
    }
  while (1);

  /* Return if no data */

  if (s->rxbuf_i == 0)
    {
      goto errout;
    }

  /* Get frame */

  ret = PROTO_FRAME_GET(s, s->proto_cmd, s->rxbuf, s->rxbuf_i, &frame);
  if (ret < 0)
    {
      /* Do not pass err further */

      ret = OK;
      goto errout;
    }

  /* Handle frame */

  ret = nxscope_recv_handle(s, frame.id, (uint16_t)frame.dlen, frame.data);
  if (ret < 0)
    {
      _err("ERROR: nxscope_recv_handle failed %d\n", ret);
      goto errout;
    }

  /* Keep the remaining data */

  bytes_left = s->rxbuf_i - frame.drop;
  if (bytes_left > 0)
    {
      memmove(s->rxbuf, &s->rxbuf[frame.drop], bytes_left);
    }

errout:
  s->rxbuf_i = bytes_left;

  return ret;
}

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

int nxscope_stream_start(FAR struct nxscope_s *s, bool start)
{
  int ret = OK;

  DEBUGASSERT(s);

  nxscope_lock(s);

  _info("force stream_start=%d\n", start);
  ret = nxscope_start_set(s, start);

  nxscope_unlock(s);

  return ret;
}
