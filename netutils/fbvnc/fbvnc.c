/****************************************************************************
 * apps/netutils/fbvnc/fbvnc.c
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

#include <sys/socket.h>
#include <sys/time.h>

#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/video/rfb.h>

#include <netutils/fbvnc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VNC_PORT         CONFIG_NETUTILS_FBVNC_PORT
#define VNC_STACKSIZE    CONFIG_NETUTILS_FBVNC_STACKSIZE
#define VNC_PRIORITY     CONFIG_NETUTILS_FBVNC_PRIORITY
#define VNC_NAME         CONFIG_NETUTILS_FBVNC_NAME
#define VNC_WIDTH        CONFIG_NETUTILS_FBVNC_FB_WIDTH
#define VNC_HEIGHT       CONFIG_NETUTILS_FBVNC_FB_HEIGHT
#define VNC_BYTESPP      CONFIG_NETUTILS_FBVNC_FB_BYTESPP
#define VNC_STRIDE       (VNC_WIDTH * VNC_BYTESPP)
#define VNC_MAX_DIRTY    CONFIG_NETUTILS_FBVNC_MAX_DIRTY
#define VNC_SEND_CHUNK   CONFIG_NETUTILS_FBVNC_SEND_CHUNK
#define VNC_SEND_TIMEOUT CONFIG_NETUTILS_FBVNC_SEND_TIMEOUT
#define VNC_MIN_INTERVAL CONFIG_NETUTILS_FBVNC_MIN_UPDATE_MS

/* How long to keep trying when the stack is out of buffers.  Ten seconds
 * in total: long enough to ride out a burst, short enough that a client
 * that has really gone away is noticed.
 */

#define VNC_SEND_RETRY_MS 10

/* Buffer exhaustion is invisible to poll(), so that one wait is on the
 * clock -- but a short one:  the pool drains as the wire empties, not on
 * any schedule of ours.
 */

#define VNC_SEND_ENOMEM_US 1000
/* Consecutive waits, of VNC_SEND_RETRY_MS each, before a send is called
 * lost.  Any progress at all resets it, so reaching this means the
 * connection has stopped moving entirely -- and a client waiting half a
 * minute for a frame is worse off than one dropped and reconnected, which
 * costs it a single screen.
 */

#define VNC_SEND_RETRIES  300

/* The framebuffer's own layout.  Everything the server sends is derived
 * from this;  see fbvnc_parsepixelfmt for what happens when a client
 * asks for something else.
 */

#define VNC_NATIVE_BPP    16
#define VNC_NATIVE_RMAX   31
#define VNC_NATIVE_GMAX   63
#define VNC_NATIVE_BMAX   31
#define VNC_NATIVE_RSHIFT 11
#define VNC_NATIVE_GSHIFT 5
#define VNC_NATIVE_BSHIFT 0

/* Hextile sub-encoding bits (RFB 7.7.4).  rfb.h has the encoding number
 * but not these.
 */

#define RFB_HEXTILE_RAW          0x01
#define RFB_HEXTILE_BG           0x02
#define RFB_HEXTILE_FG           0x04
#define RFB_HEXTILE_ANYSUBRECTS  0x08
#define RFB_HEXTILE_SUBRECTSCOL  0x10

#define VNC_HEXTILE_TILE          16

/* TRLE uses the same tiling.  Sixteen is also the largest palette that
 * still packs into whole bits per index (four), which is where the gain
 * over Hextile comes from on a flat interface.
 */

#define VNC_TRLE_TILE             16
#define VNC_TRLE_MAXPAL           16
#define VNC_HEXTILE_MAX_SUBRECTS 128

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* The pixel format the client last asked for */

struct fbvnc_fmt_s
{
  uint8_t  bpp;
  uint8_t  bytespp;
  uint16_t rmax;
  uint16_t gmax;
  uint16_t bmax;
  uint8_t  rshift;
  uint8_t  gshift;
  uint8_t  bshift;
  bool     bigendian;

  /* True when the client's format is byte-for-byte the framebuffer's, so
   * that a rectangle can go out straight from display memory.
   */

  bool     native;
};

struct fbvnc_state_s
{
  volatile bool running;
  volatile bool connected;
  fbvnc_snapshot_t snapshot;
  fbvnc_event_t on_connect;
  fbvnc_event_t on_disconnect;
  fbvnc_event_t on_invalidate;
  fbvnc_pointer_t on_pointer;
  fbvnc_key_t on_key;
  pthread_t thread;
  int listensock;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct fbvnc_state_s g_fbvnc;

/* Served geometry:  configuration defaults until fbvnc_start says
 * otherwise.
 */

static uint16_t g_w      = VNC_WIDTH;
static uint16_t g_h      = VNC_HEIGHT;
static uint16_t g_stride = VNC_STRIDE;

/* The encoding picked for this session, decided once the client's
 * SetEncodings message lands and then used for every rectangle.  Raw is
 * the fallback every RFB client is required to accept.
 */

static int32_t g_encoding = RFB_ENCODING_RAW;

/* When the last update finished, for the rate cap below */

static struct timespec g_lastupdate;

/* Sending a frame takes as long as the frame is large, and while it is in
 * flight the client's key and pointer messages sit unread in the socket.
 * The reader and the sender are therefore separate:  this thread only ever
 * parses messages and dispatches input, and hands the sender one token per
 * update the client asks for, so every request still gets exactly one
 * answer.
 */

static pthread_t g_sender;
static sem_t g_updatesem;

/* Whether the client is owed a frame.  It is a flag and not a count:  a
 * client that asks again while one is being sent is asking for what the
 * screen looks like now, not for two frames, and answering every request
 * separately means sending it a queue of pictures that were already out
 * of date when they left.
 */

static volatile bool g_updatereq;

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
/* Bytes handed to the network stack for the update being built */

static uint32_t g_wirebytes;
#endif
static volatile bool g_clientrun;
static struct fbvnc_fmt_s g_fmt;

/* Scratch used only when the client's pixel format differs from the
 * framebuffer's.  In the native case nothing is copied at all.
 */

static uint8_t g_fbvnc_cvt[VNC_SEND_CHUNK];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbvnc_sendall
 ****************************************************************************/

static int fbvnc_sendall(int sock, FAR const void *buf, size_t len)
{
  FAR const uint8_t *ptr = buf;
#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  size_t wire = len;
#endif
  unsigned int retries = 0;
  size_t chunk;
  ssize_t nsent;

  while (len > 0)
    {
      chunk = len > VNC_SEND_CHUNK ? VNC_SEND_CHUNK : len;

      nsent = send(sock, ptr, chunk, 0);
      if (nsent < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          /* Running out of buffers, or a full send queue, says to come
           * back in a moment -- not that the connection is finished.
           * Treating it as fatal is what made a client drop and then
           * reconnect, which costs a whole screen to start over with.
           */

          if (errno == ENOMEM || errno == EAGAIN || errno == EWOULDBLOCK)
            {
              struct pollfd pfd;
              bool nobufs = errno == ENOMEM;

              if (++retries > VNC_SEND_RETRIES)
                {
                  return -errno;
                }

              /* Wait for the socket to take more, rather than for a fixed
               * sleep to expire.  Room appears with the next
               * acknowledgement, which on a busy link is a fraction of a
               * millisecond;  sleeping ten of them per chunk spends most
               * of a frame's time doing nothing, and it showed as a
               * transfer running at a third of what the wire could carry.
               */

              pfd.fd      = sock;
              pfd.events  = POLLOUT;
              pfd.revents = 0;

              poll(&pfd, 1, VNC_SEND_RETRY_MS);

              if (nobufs)
                {
                  usleep(VNC_SEND_ENOMEM_US);
                }

              continue;
            }

          return -errno;
        }
      else if (nsent == 0)
        {
          return -ECONNRESET;
        }

      retries = 0;
      ptr += nsent;
      len -= nsent;
    }

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  g_wirebytes += wire;
#endif

  return OK;
}

/****************************************************************************
 * Name: fbvnc_recvall
 ****************************************************************************/

static int fbvnc_recvall(int sock, FAR void *buf, size_t len)
{
  FAR uint8_t *ptr = buf;
  ssize_t nrecvd;

  while (len > 0)
    {
      nrecvd = recv(sock, ptr, len, 0);
      if (nrecvd < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }
      else if (nrecvd == 0)
        {
          return -ECONNRESET;
        }

      ptr += nrecvd;
      len -= nrecvd;
    }

  return OK;
}

/****************************************************************************
 * Name: fbvnc_isnative
 *
 * Description:
 *   Decide whether the client's format lets us send framebuffer memory
 *   verbatim.  This is the whole point of advertising the native format:
 *   in the common case there is no conversion and no copy.
 *
 ****************************************************************************/

static bool fbvnc_isnative(FAR const struct fbvnc_fmt_s *fmt)
{
  return fmt->bpp == VNC_NATIVE_BPP &&
         fmt->rmax == VNC_NATIVE_RMAX &&
         fmt->gmax == VNC_NATIVE_GMAX &&
         fmt->bmax == VNC_NATIVE_BMAX &&
         fmt->rshift == VNC_NATIVE_RSHIFT &&
         fmt->gshift == VNC_NATIVE_GSHIFT &&
         fmt->bshift == VNC_NATIVE_BSHIFT &&
         !fmt->bigendian;
}

/****************************************************************************
 * Name: fbvnc_setnative
 ****************************************************************************/

static void fbvnc_setnative(FAR struct fbvnc_fmt_s *fmt)
{
  fmt->bpp       = VNC_NATIVE_BPP;
  fmt->bytespp   = VNC_NATIVE_BPP / 8;
  fmt->rmax      = VNC_NATIVE_RMAX;
  fmt->gmax      = VNC_NATIVE_GMAX;
  fmt->bmax      = VNC_NATIVE_BMAX;
  fmt->rshift    = VNC_NATIVE_RSHIFT;
  fmt->gshift    = VNC_NATIVE_GSHIFT;
  fmt->bshift    = VNC_NATIVE_BSHIFT;
  fmt->bigendian = false;
  fmt->native    = true;
}

/****************************************************************************
 * Name: fbvnc_cvtrow
 *
 * Description:
 *   Repack one row of RGB565 into the layout the client asked for --
 *   whatever its width:  a client that asks for eight bits per pixel and
 *   is answered in sixteen decodes noise, because in RFB the client
 *   chooses the format and the server obeys.
 *
 *   Only reached when that layout is not the framebuffer's own.
 *
 ****************************************************************************/

static void fbvnc_cvtrow(FAR const uint8_t *src, FAR uint8_t *dst,
                             uint16_t npixels,
                             FAR const struct fbvnc_fmt_s *fmt)
{
  uint16_t pixel;
  uint32_t value;
  uint16_t i;
  uint8_t n;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  for (i = 0; i < npixels; i++)
    {
      pixel = src[0] | (src[1] << 8);
      src += 2;

      r = (pixel >> VNC_NATIVE_RSHIFT) & VNC_NATIVE_RMAX;
      g = (pixel >> VNC_NATIVE_GSHIFT) & VNC_NATIVE_GMAX;
      b = (pixel >> VNC_NATIVE_BSHIFT) & VNC_NATIVE_BMAX;

      value = ((uint32_t)(r * fmt->rmax / VNC_NATIVE_RMAX) << fmt->rshift) |
              ((uint32_t)(g * fmt->gmax / VNC_NATIVE_GMAX) << fmt->gshift) |
              ((uint32_t)(b * fmt->bmax / VNC_NATIVE_BMAX) << fmt->bshift);

      /* Byte order is the client's, and so is the width:  one, two or
       * four bytes, most significant first only if it asked for that.
       */

      for (n = 0; n < fmt->bytespp; n++)
        {
          *dst++ = fmt->bigendian ?
                   value >> ((fmt->bytespp - 1 - n) * 8) :
                   value >> (n * 8);
        }
    }
}

/****************************************************************************
 * Name: fbvnc_sendrect
 *
 * Description:
 *   Send one rectangle using the Raw encoding.
 *
 ****************************************************************************/

static int fbvnc_sendrect(int sock, FAR const uint8_t *fb,
                              FAR const struct fbvnc_rect_s *rect,
                              FAR const struct fbvnc_fmt_s *fmt)
{
  struct rfb_rectangle_s hdr;
  FAR const uint8_t *row;
  uint16_t y;
  int ret;

  rfb_putbe16(hdr.xpos, rect->x);
  rfb_putbe16(hdr.ypos, rect->y);
  rfb_putbe16(hdr.width, rect->w);
  rfb_putbe16(hdr.height, rect->h);
  rfb_putbe32(hdr.encoding, RFB_ENCODING_RAW);

  /* The data[] member is a placeholder for the pixels that follow, so
   * only the header proper goes out here.
   */

  ret = fbvnc_sendall(sock, &hdr, SIZEOF_RFB_RECTANGE_S(0));
  if (ret < 0)
    {
      return ret;
    }

  row = fb + rect->y * g_stride + rect->x * VNC_BYTESPP;

  if (fmt->native && rect->x == 0 && rect->w == g_w)
    {
      /* Full-width rectangle in the framebuffer's own format:  the rows
       * are contiguous, so the whole block leaves display memory in one
       * call with nothing copied.
       */

      return fbvnc_sendall(sock, row, rect->h * g_stride);
    }

  for (y = 0; y < rect->h; y++)
    {
      if (fmt->native)
        {
          ret = fbvnc_sendall(sock, row, rect->w * VNC_BYTESPP);
        }
      else
        {
          fbvnc_cvtrow(row, g_fbvnc_cvt, rect->w, fmt);
          ret = fbvnc_sendall(sock, g_fbvnc_cvt,
                                  rect->w * fmt->bytespp);
        }

      if (ret < 0)
        {
          return ret;
        }

      row += g_stride;
    }

  return OK;
}

#ifdef CONFIG_NETUTILS_FBVNC_ENCODING_HEXTILE

/****************************************************************************
 * Name: fbvnc_tilepalette
 *
 * Description:
 *   Count the colours in a tile, giving up once there are more than two.
 *   Two is the interesting boundary:  one colour is a fill, two can be
 *   described as a background plus runs of a foreground, and beyond that
 *   Raw is usually smaller than any description of the difference.
 *
 * Returned Value:
 *   The number of distinct colours, or 3 to mean "more than two".  bg is
 *   set to the most frequent colour and fg to the other one.
 *
 ****************************************************************************/

static int fbvnc_tilepalette(FAR const uint8_t *fb,
                                 uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h,
                                 FAR uint16_t *bg, FAR uint16_t *fg)
{
  FAR const uint16_t *row;
  uint16_t colour[2];
  uint32_t count[2];
  uint16_t pixel;
  uint16_t i;
  uint16_t j;
  int ncolours = 0;

  count[0] = 0;
  count[1] = 0;
  colour[0] = 0;
  colour[1] = 0;

  for (i = 0; i < h; i++)
    {
      row = (FAR const uint16_t *)(fb + (y + i) * g_stride) + x;

      for (j = 0; j < w; j++)
        {
          pixel = row[j];

          if (ncolours > 0 && pixel == colour[0])
            {
              count[0]++;
            }
          else if (ncolours > 1 && pixel == colour[1])
            {
              count[1]++;
            }
          else if (ncolours < 2)
            {
              colour[ncolours] = pixel;
              count[ncolours] = 1;
              ncolours++;
            }
          else
            {
              return 3;
            }
        }
    }

  /* The background is the colour worth not describing */

  if (ncolours == 2 && count[1] > count[0])
    {
      *bg = colour[1];
      *fg = colour[0];
    }
  else
    {
      *bg = colour[0];
      *fg = colour[1];
    }

  return ncolours;
}

/****************************************************************************
 * Name: fbvnc_tilesubrects
 *
 * Description:
 *   Describe a two-colour tile as runs of the foreground colour over the
 *   background.  Runs are found a row at a time and emitted as one-row
 *   subrectangles;  merging vertically would cost maybe a fifth of the
 *   bytes but a good deal of the simplicity.
 *
 * Returned Value:
 *   The number of subrectangles written, or a negative value if there were
 *   more than would fit.
 *
 ****************************************************************************/

static int fbvnc_tilesubrects(FAR const uint8_t *fb,
                                  uint16_t x, uint16_t y,
                                  uint16_t w, uint16_t h,
                                  uint16_t fg, FAR uint8_t *dest,
                                  int maxsubrects)
{
  FAR const uint16_t *row;
  uint16_t i;
  uint16_t j;
  uint16_t start;
  int n = 0;

  for (i = 0; i < h; i++)
    {
      row = (FAR const uint16_t *)(fb + (y + i) * g_stride) + x;

      j = 0;
      while (j < w)
        {
          if (row[j] != fg)
            {
              j++;
              continue;
            }

          start = j;
          while (j < w && row[j] == fg)
            {
              j++;
            }

          if (n >= maxsubrects)
            {
              return -1;
            }

          /* x and y share a byte, as do width - 1 and height - 1 */

          *dest++ = (start << 4) | i;
          *dest++ = ((j - start - 1) << 4) | 0;
          n++;
        }
    }

  return n;
}

/****************************************************************************
 * Name: fbvnc_tilepalette_n
 *
 * Description:
 *   Collect the distinct colours of one tile.  Returns how many there
 *   are, or zero if there are more than the caller can hold -- which is
 *   the answer that says to send the tile as it is.
 *
 *   The index each pixel resolved to is written out as it goes:  finding
 *   it again while packing would mean searching the palette a second time
 *   for every pixel, and that search is what the encoding costs.
 *
 ****************************************************************************/

static int fbvnc_tilepalette_n(FAR const uint8_t *fb,
                                   uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h,
                                   FAR uint16_t *palette, int max,
                                   FAR uint8_t *index)
{
  FAR const uint16_t *row;
  uint16_t pixel;
  uint16_t i;
  uint16_t j;
  int n = 0;
  int k;

  for (i = 0; i < h; i++)
    {
      row = (FAR const uint16_t *)(fb + (y + i) * g_stride) + x;

      for (j = 0; j < w; j++)
        {
          pixel = row[j];

          for (k = 0; k < n; k++)
            {
              if (palette[k] == pixel)
                {
                  break;
                }
            }

          if (k == n)
            {
              if (n == max)
                {
                  return 0;
                }

              palette[n++] = pixel;
            }

          *index++ = k;
        }
    }

  return n;
}

/****************************************************************************
 * Name: fbvnc_sendrect_trle
 *
 * Description:
 *   Send one rectangle using the TRLE encoding (RFB 7.7.5).
 *
 *   Same sixteen pixel tiles as Hextile, but a tile of few colours is sent
 *   as a palette and an index per pixel rather than as a list of
 *   sub-rectangles.  On an interface of flat panels and text -- which is
 *   what a widget toolkit draws -- most tiles have one or two colours, and
 *   two colours cost one bit per pixel instead of a rectangle each.
 *
 *   The run-length subencodings are deliberately absent:  they pay off on
 *   long horizontal runs, which the packed palette already covers at a
 *   fixed size, and leaving them out keeps this a single pass over each
 *   tile.
 *
 ****************************************************************************/

static int fbvnc_sendrect_trle(int sock, FAR const uint8_t *fb,
                                   FAR const struct fbvnc_rect_s *r,
                                   FAR const struct fbvnc_fmt_s *fmt)
{
  /* Static rather than automatic:  larger than the server thread's stack,
   * and there is only ever one client.
   */

  static uint8_t buf[VNC_SEND_CHUNK];

  static uint8_t index[VNC_TRLE_TILE * VNC_TRLE_TILE];

  uint16_t palette[VNC_TRLE_MAXPAL];
  struct rfb_rectangle_s hdr;
  FAR const uint16_t *row;
  size_t used = 0;
  uint16_t tx;
  uint16_t ty;
  uint16_t i;
  uint16_t j;
  int npal;
  int ret;

  UNUSED(fmt);

  rfb_putbe16(hdr.xpos, r->x);
  rfb_putbe16(hdr.ypos, r->y);
  rfb_putbe16(hdr.width, r->w);
  rfb_putbe16(hdr.height, r->h);
  rfb_putbe32(hdr.encoding, RFB_ENCODING_TRLE);

  ret = fbvnc_sendall(sock, &hdr, SIZEOF_RFB_RECTANGE_S(0));
  if (ret < 0)
    {
      return ret;
    }

  for (ty = 0; ty < r->h; ty += VNC_TRLE_TILE)
    {
      uint16_t th = r->h - ty;
      th = th > VNC_TRLE_TILE ? VNC_TRLE_TILE : th;

      for (tx = 0; tx < r->w; tx += VNC_TRLE_TILE)
        {
          uint16_t tw = r->w - tx;
          FAR uint8_t *p;
          size_t rawsize;
          int bits;

          tw = tw > VNC_TRLE_TILE ? VNC_TRLE_TILE : tw;

          /* Flush when the largest possible tile would not fit */

          if (used + 1 + tw * th * VNC_BYTESPP > sizeof(buf))
            {
              ret = fbvnc_sendall(sock, buf, used);
              if (ret < 0)
                {
                  return ret;
                }

              used = 0;
            }

          p       = &buf[used];
          rawsize = 1 + tw * th * VNC_BYTESPP;
          npal    = fbvnc_tilepalette_n(fb, r->x + tx, r->y + ty,
                                            tw, th, palette,
                                            VNC_TRLE_MAXPAL, index);

          /* One colour:  the tile is that colour and nothing else */

          if (npal == 1)
            {
              *p++ = 1;
              *p++ = palette[0];
              *p++ = palette[0] >> 8;
              used += p - &buf[used];
              continue;
            }

          bits = npal == 2 ? 1 : (npal <= 4 ? 2 : 4);

          if (npal > 1 &&
              1 + npal * VNC_BYTESPP + ((tw * bits + 7) / 8) * th < rawsize)
            {
              /* Packed palette:  the colours, then an index per pixel,
               * most significant bits first, each row padded to a byte.
               */

              *p++ = npal;

              for (i = 0; i < npal; i++)
                {
                  *p++ = palette[i];
                  *p++ = palette[i] >> 8;
                }

              for (i = 0; i < th; i++)
                {
                  uint8_t acc = 0;
                  int nbits = 0;

                  for (j = 0; j < tw; j++)
                    {
                      acc = (acc << bits) | index[i * tw + j];
                      nbits += bits;

                      if (nbits == 8)
                        {
                          *p++  = acc;
                          acc   = 0;
                          nbits = 0;
                        }
                    }

                  if (nbits > 0)
                    {
                      *p++ = acc << (8 - nbits);
                    }
                }

              used += p - &buf[used];
              continue;
            }

          /* Too many colours to be worth describing:  send the pixels */

          *p++ = 0;

          for (i = 0; i < th; i++)
            {
              row = (FAR const uint16_t *)
                    (fb + (r->y + ty + i) * g_stride) + r->x + tx;
              memcpy(p, row, tw * VNC_BYTESPP);
              p += tw * VNC_BYTESPP;
            }

          used += p - &buf[used];
        }
    }

  if (used > 0)
    {
      return fbvnc_sendall(sock, buf, used);
    }

  return OK;
}

/****************************************************************************
 * Name: fbvnc_sendrect_hextile
 *
 * Description:
 *   Send one rectangle using the Hextile encoding (RFB 7.7.4).
 *
 *   Only the native pixel format is handled.  Anything else falls back to
 *   Raw, because a converted tile would have to be materialised first and
 *   the case does not arise with the clients this serves.
 *
 ****************************************************************************/

static int fbvnc_sendrect_hextile(int sock, FAR const uint8_t *fb,
                                      FAR const struct fbvnc_rect_s *r,
                                      FAR const struct fbvnc_fmt_s *fmt)
{
  /* Static rather than automatic:  together these are larger than the
   * server thread's stack, and there is only ever one client.
   */

  static uint8_t buf[VNC_SEND_CHUNK];
  static uint8_t subrects[2 * VNC_HEXTILE_MAX_SUBRECTS];

  struct rfb_rectangle_s hdr;
  size_t used = 0;
  uint16_t lastbg = 0;
  uint16_t lastfg = 0;
  bool havebg = false;
  bool havefg = false;
  uint16_t tx;
  uint16_t ty;
  uint16_t i;
  uint16_t bg;
  uint16_t fg;
  int ncolours;
  int nsub;
  int ret;

  rfb_putbe16(hdr.xpos, r->x);
  rfb_putbe16(hdr.ypos, r->y);
  rfb_putbe16(hdr.width, r->w);
  rfb_putbe16(hdr.height, r->h);
  rfb_putbe32(hdr.encoding, RFB_ENCODING_HEXTILE);

  ret = fbvnc_sendall(sock, &hdr, SIZEOF_RFB_RECTANGE_S(0));
  if (ret < 0)
    {
      return ret;
    }

  for (ty = 0; ty < r->h; ty += VNC_HEXTILE_TILE)
    {
      uint16_t th = r->h - ty;
      th = th > VNC_HEXTILE_TILE ? VNC_HEXTILE_TILE : th;

      for (tx = 0; tx < r->w; tx += VNC_HEXTILE_TILE)
        {
          uint16_t tw = r->w - tx;
          size_t rawsize;
          size_t subsize;
          FAR uint8_t *p;

          tw = tw > VNC_HEXTILE_TILE ? VNC_HEXTILE_TILE : tw;

          /* Flush when the largest possible tile would not fit */

          if (used + 5 + tw * th * VNC_BYTESPP > sizeof(buf))
            {
              ret = fbvnc_sendall(sock, buf, used);
              if (ret < 0)
                {
                  return ret;
                }

              used = 0;
            }

          p = &buf[used];
          rawsize = 1 + tw * th * VNC_BYTESPP;

          ncolours = fbvnc_tilepalette(fb, r->x + tx, r->y + ty,
                                           tw, th, &bg, &fg);

          if (ncolours <= 1)
            {
              *p++ = havebg && bg == lastbg ? 0 : RFB_HEXTILE_BG;

              if (!havebg || bg != lastbg)
                {
                  *p++ = bg;
                  *p++ = bg >> 8;
                  lastbg = bg;
                  havebg = true;
                }

              used += p - &buf[used];
              continue;
            }

          if (ncolours == 2)
            {
              nsub = fbvnc_tilesubrects(fb, r->x + tx, r->y + ty,
                                            tw, th, fg, subrects,
                                            VNC_HEXTILE_MAX_SUBRECTS);
              if (nsub > 0)
                {
                  subsize = 1 + 1 + 2 * nsub +
                            ((havebg && bg == lastbg) ? 0 : VNC_BYTESPP) +
                            ((havefg && fg == lastfg) ? 0 : VNC_BYTESPP);

                  if (subsize < rawsize)
                    {
                      FAR uint8_t *mask = p++;

                      *mask = RFB_HEXTILE_ANYSUBRECTS;

                      if (!havebg || bg != lastbg)
                        {
                          *mask |= RFB_HEXTILE_BG;
                          *p++ = bg;
                          *p++ = bg >> 8;
                          lastbg = bg;
                          havebg = true;
                        }

                      if (!havefg || fg != lastfg)
                        {
                          *mask |= RFB_HEXTILE_FG;
                          *p++ = fg;
                          *p++ = fg >> 8;
                          lastfg = fg;
                          havefg = true;
                        }

                      *p++ = nsub;
                      memcpy(p, subrects, 2 * nsub);
                      p += 2 * nsub;

                      used += p - &buf[used];
                      continue;
                    }
                }
            }

          /* Raw.  A raw tile says nothing about the background or the
           * foreground, so what the client remembers of them is unchanged.
           */

          *p++ = RFB_HEXTILE_RAW;

          for (i = 0; i < th; i++)
            {
              memcpy(p, fb + (r->y + ty + i) * g_stride +
                     (r->x + tx) * VNC_BYTESPP, tw * VNC_BYTESPP);
              p += tw * VNC_BYTESPP;
            }

          used += p - &buf[used];
        }
    }

  if (used > 0)
    {
      return fbvnc_sendall(sock, buf, used);
    }

  return OK;
}
#endif /* CONFIG_NETUTILS_FBVNC_ENCODING_HEXTILE */

/****************************************************************************
 * Name: fbvnc_sendupdate
 ****************************************************************************/

static int fbvnc_sendupdate(int sock,
                                FAR const struct fbvnc_fmt_s *fmt)
{
  struct fbvnc_rect_s rects[VNC_MAX_DIRTY];
  struct rfb_framebufferupdate_s hdr;
  FAR const uint8_t *fb;
#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  struct timespec t0;
  struct timespec t1;
  struct timespec t2;
  uint32_t nbytes = 0;
#endif
  uint32_t nrects = 0;
  uint32_t i;
  int ret;

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  clock_gettime(CLOCK_MONOTONIC, &t0);
  g_wirebytes = 0;
#endif

  fb = g_fbvnc.snapshot(rects, VNC_MAX_DIRTY, &nrects);

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  clock_gettime(CLOCK_MONOTONIC, &t1);
#endif
  if (fb == NULL)
    {
      syslog(LOG_WARNING, "fbvnc: snapshot failed\n");
      return -EIO;
    }

  /* Nothing has changed.  An empty update is a valid answer, but it is an
   * answer the client will immediately ask again for, so the request is
   * left standing instead:  RFB lets the server reply when it has
   * something, and every empty round trip costs a snapshot -- which,
   * where the dirty areas come from comparing frames, is the whole
   * screen read twice.
   */

  if (nrects == 0)
    {
      return 0;
    }

  memset(&hdr, 0, sizeof(hdr));
  hdr.msgtype = RFB_FBUPDATE_MSG;
  rfb_putbe16(hdr.nrect, nrects);

  ret = fbvnc_sendall(sock, &hdr, SIZEOF_RFB_FRAMEBUFFERUPDATE_S(0));
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < nrects; i++)
    {
#ifdef CONFIG_NETUTILS_FBVNC_ENCODING_TRLE
      if (g_encoding == RFB_ENCODING_TRLE && fmt->native)
        {
          ret = fbvnc_sendrect_trle(sock, fb, &rects[i], fmt);
        }
      else
#endif
#ifdef CONFIG_NETUTILS_FBVNC_ENCODING_HEXTILE
      if (g_encoding == RFB_ENCODING_HEXTILE && fmt->native)
        {
          ret = fbvnc_sendrect_hextile(sock, fb, &rects[i], fmt);
        }
      else
#endif
        {
          ret = fbvnc_sendrect(sock, fb, &rects[i], fmt);
        }

      if (ret < 0)
        {
          return ret;
        }

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
      nbytes += rects[i].w * rects[i].h * fmt->bytespp;
#endif
    }

#ifdef CONFIG_NETUTILS_FBVNC_TRACE
  /* What an update costs is the whole question on a link this size, so
   * say it rather than leave it to be guessed at.  Splitting the snapshot
   * from the sending is what says which side to look at.  It is a line
   * per update, so it is asked for rather than assumed.
   */

  clock_gettime(CLOCK_MONOTONIC, &t2);

  /* "queued" and not "sent":  with write buffering the stack takes the
   * bytes and returns, and they leave the wire afterwards.  Reading this
   * as delivery makes a frame look faster than the link can carry it.
   */

  syslog(LOG_INFO, "fbvnc: update: %" PRIu32 " rect(s), %" PRIu32
                   " px of screen, %" PRIu32 " bytes on the wire, "
                   "snapshot %" PRIu32 " ms, queued %" PRIu32 " ms\n",
         nrects, nbytes / VNC_BYTESPP, g_wirebytes,
         (uint32_t)((t1.tv_sec - t0.tv_sec) * 1000 +
                    (t1.tv_nsec - t0.tv_nsec) / 1000000),
         (uint32_t)((t2.tv_sec - t1.tv_sec) * 1000 +
                    (t2.tv_nsec - t1.tv_nsec) / 1000000));
#endif

  return (int)nrects;
}

/****************************************************************************
 * Name: fbvnc_handshake
 *
 * Description:
 *   RFB 3.7 handshake.  3.7 rather than 3.3 because it negotiates
 *   security with a list of types, which is what a password would need
 *   later;  3.8 only adds a SecurityResult message to the None path,
 *   which buys nothing here.
 *
 ****************************************************************************/

static int fbvnc_handshake(int sock, FAR struct fbvnc_fmt_s *fmt)
{
  struct rfb_serverinit_s sinit;
  char version[sizeof(RFB_PROTOCOL_VERSION_3p7) - 1];
  uint8_t sectypes[2];
  uint8_t selected;
  uint8_t shared;
  size_t namelen;
  int ret;

  ret = fbvnc_sendall(sock, RFB_PROTOCOL_VERSION_3p7, sizeof(version));
  if (ret < 0)
    {
      return ret;
    }

  ret = fbvnc_recvall(sock, version, sizeof(version));
  if (ret < 0)
    {
      return ret;
    }

  syslog(LOG_INFO, "fbvnc: client version %.11s\n", version);

  /* Offer exactly one security type.  The count byte comes first, then
   * the types themselves.
   */

  sectypes[0] = 1;
  sectypes[1] = RFB_SECTYPE_NONE;

  ret = fbvnc_sendall(sock, sectypes, sizeof(sectypes));
  if (ret < 0)
    {
      return ret;
    }

  ret = fbvnc_recvall(sock, &selected, sizeof(selected));
  if (ret < 0)
    {
      return ret;
    }

  if (selected != RFB_SECTYPE_NONE)
    {
      syslog(LOG_WARNING, "fbvnc: client picked security type %u, "
                          "which was not offered\n", selected);
      return -EPROTO;
    }

  /* Under 3.7 the None type sends no SecurityResult, so ClientInit is
   * next.  Its shared flag is advisory and this server only ever has one
   * client, so it is read and discarded.
   */

  ret = fbvnc_recvall(sock, &shared, sizeof(shared));
  if (ret < 0)
    {
      return ret;
    }

  /* Advertise the framebuffer's own format rather than promoting to
   * 32bpp RGBA.  Promoting is convenient for canvas-based clients but
   * doubles every byte on the wire, and at this resolution that is the
   * difference between a 1.2 MiB and a 2.4 MiB full redraw.
   */

  namelen = strlen(VNC_NAME);
  memset(&sinit, 0, sizeof(sinit));

  rfb_putbe16(sinit.width, g_w);
  rfb_putbe16(sinit.height, g_h);

  sinit.format.bpp       = VNC_NATIVE_BPP;
  sinit.format.depth     = VNC_NATIVE_BPP;
  sinit.format.truecolor = 1;
  sinit.format.rshift    = VNC_NATIVE_RSHIFT;
  sinit.format.gshift    = VNC_NATIVE_GSHIFT;
  sinit.format.bshift    = VNC_NATIVE_BSHIFT;

  rfb_putbe16(sinit.format.rmax, VNC_NATIVE_RMAX);
  rfb_putbe16(sinit.format.gmax, VNC_NATIVE_GMAX);
  rfb_putbe16(sinit.format.bmax, VNC_NATIVE_BMAX);
  rfb_putbe32(sinit.namelen, namelen);

  ret = fbvnc_sendall(sock, &sinit, SIZEOF_RFB_SERVERINIT_S(0));
  if (ret < 0)
    {
      return ret;
    }

  ret = fbvnc_sendall(sock, VNC_NAME, namelen);
  if (ret < 0)
    {
      return ret;
    }

  fbvnc_setnative(fmt);

  syslog(LOG_INFO, "fbvnc: handshake done, %dx%d %dbpp RGB565\n",
         g_w, g_h, VNC_NATIVE_BPP);

  return OK;
}

/****************************************************************************
 * Name: fbvnc_parsepixelfmt
 *
 * Description:
 *   Apply a SetPixelFormat message.  Any 16-bit layout is honoured, which
 *   covers the RGB565/BGR565/RGB555 variants clients actually ask for.
 *
 *   A request for a different depth is refused and the server keeps
 *   sending its native format.  That is a deliberate deviation:  honouring
 *   32bpp would double the bytes per frame, and exhausting the network
 *   buffer pool is a worse failure than a client that has to accept 16bpp.
 *
 ****************************************************************************/

static void fbvnc_parsepixelfmt(FAR const uint8_t *buf,
                                    FAR struct fbvnc_fmt_s *fmt)
{
  FAR const struct rfb_pixelfmt_s *pixelfmt;

  pixelfmt = (FAR const struct rfb_pixelfmt_s *)&buf[3];

  /* Anything the conversion can produce is accepted.  A colour map is
   * not:  it would mean sending the map and indices into it, so such a
   * client keeps the native format and is told why.
   */

  if ((pixelfmt->bpp != 8 && pixelfmt->bpp != 16 && pixelfmt->bpp != 32) ||
      pixelfmt->truecolor == 0)
    {
      syslog(LOG_WARNING, "fbvnc: client asked for %ubpp%s, keeping "
                          "native %dbpp\n", pixelfmt->bpp,
             pixelfmt->truecolor ? "" : " with a colour map",
             VNC_NATIVE_BPP);
      return;
    }

  /* One converted row has to fit the buffer it is built in */

  if ((uint32_t)g_w * (pixelfmt->bpp / 8) > sizeof(g_fbvnc_cvt))
    {
      syslog(LOG_WARNING, "fbvnc: a %ubpp row of %u pixels does not fit "
                          "the %u byte conversion buffer, keeping native "
                          "%dbpp\n", pixelfmt->bpp, g_w,
             (unsigned)sizeof(g_fbvnc_cvt), VNC_NATIVE_BPP);
      return;
    }

  fmt->bpp       = pixelfmt->bpp;
  fmt->bytespp   = pixelfmt->bpp / 8;
  fmt->bigendian = pixelfmt->bigendian != 0;
  fmt->rmax      = rfb_getbe16(pixelfmt->rmax);
  fmt->gmax      = rfb_getbe16(pixelfmt->gmax);
  fmt->bmax      = rfb_getbe16(pixelfmt->bmax);
  fmt->rshift    = pixelfmt->rshift;
  fmt->gshift    = pixelfmt->gshift;
  fmt->bshift    = pixelfmt->bshift;
  fmt->native    = fbvnc_isnative(fmt);

  syslog(LOG_INFO, "fbvnc: pixel format %ubpp rmax=%u gmax=%u bmax=%u "
                   "shifts=%u/%u/%u%s\n",
         fmt->bpp, fmt->rmax, fmt->gmax, fmt->bmax,
         fmt->rshift, fmt->gshift, fmt->bshift,
         fmt->native ? " (native, zero copy)" : " (converted)");
}

/****************************************************************************
 * Name: fbvnc_sendthread
 *
 * Description:
 *   Waits for the reader to say that the client wants a frame, then spends
 *   however long the frame takes writing it, without holding up anything
 *   the client has to say in the meantime.
 *
 ****************************************************************************/

static FAR void *fbvnc_sendthread(FAR void *arg)
{
  int sock = (int)(intptr_t)arg;
  struct timespec now;
  int32_t elapsed;
  int ret;

  while (g_clientrun)
    {
      if (sem_wait(&g_updatesem) < 0)
        {
          continue;
        }

      if (!g_clientrun)
        {
          break;
        }

      /* Cleared before the frame is built, so that a request arriving
       * while it is being sent asks for the next one
       */

      g_updatereq = false;

      /* Hold off until the interval has passed since the last update went
       * out.  A client that asks again the instant it has finished parsing
       * -- which is what they do while a list is being dragged --
       * otherwise keeps a full screen in flight permanently, and the
       * display never catches up.  Delaying the answer is not a protocol
       * violation: the client is already waiting for one.
       */

      if (VNC_MIN_INTERVAL > 0)
        {
          clock_gettime(CLOCK_MONOTONIC, &now);
          elapsed = (now.tv_sec - g_lastupdate.tv_sec) * 1000 +
                    (now.tv_nsec - g_lastupdate.tv_nsec) / 1000000;

          if (elapsed >= 0 && elapsed < VNC_MIN_INTERVAL)
            {
              usleep((VNC_MIN_INTERVAL - elapsed) * 1000);
            }
        }

      ret = fbvnc_sendupdate(sock, &g_fmt);
      clock_gettime(CLOCK_MONOTONIC, &g_lastupdate);

      if (ret == 0)
        {
          /* Nothing to say yet:  the client is still owed a frame, so
           * look again after the interval rather than answering with an
           * empty one.
           */

          g_updatereq = true;
          sem_post(&g_updatesem);
          continue;
        }

      if (ret < 0)
        {
          syslog(LOG_ERR, "fbvnc: update failed: %d\n", ret);
          g_clientrun = false;

          /* Wake the reader out of its recv so the client is dropped */

          shutdown(sock, SHUT_RDWR);
          break;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: fbvnc_handleclient
 ****************************************************************************/

static void fbvnc_handleclient(int sock)
{
  pthread_attr_t attr;
  struct sched_param param;
  uint8_t msgtype;
  uint8_t buf[20];
  uint16_t nencodings;
  uint16_t i;
  bool hashextile;
  bool hastrle;
  int ret;

  ret = fbvnc_handshake(sock, &g_fmt);
  if (ret < 0)
    {
      syslog(LOG_ERR, "fbvnc: handshake failed: %d\n", ret);
      return;
    }

  /* Older clients never send SetEncodings; they get Raw */

  g_encoding = RFB_ENCODING_RAW;
  clock_gettime(CLOCK_MONOTONIC, &g_lastupdate);

  g_fbvnc.connected = true;
  if (g_fbvnc.on_connect != NULL)
    {
      g_fbvnc.on_connect();
    }

  /* The frames go out on a thread of their own, so that this one is always
   * free to read what the client is saying
   */

  g_clientrun = true;
  g_updatereq = false;
  sem_init(&g_updatesem, 0, 0);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, VNC_STACKSIZE);
  param.sched_priority = VNC_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);

  ret = pthread_create(&g_sender, &attr, fbvnc_sendthread,
                       (FAR void *)(intptr_t)sock);
  pthread_attr_destroy(&attr);

  if (ret != 0)
    {
      syslog(LOG_ERR, "fbvnc: cannot start the sender: %d\n", ret);
      sem_destroy(&g_updatesem);
      return;
    }

  pthread_setname_np(g_sender, "fbvncsend");

  while (g_fbvnc.running && g_clientrun)
    {
      ret = fbvnc_recvall(sock, &msgtype, sizeof(msgtype));
      if (ret < 0)
        {
          syslog(LOG_INFO, "fbvnc: client disconnected\n");
          break;
        }

      switch (msgtype)
        {
          case RFB_SETPIXELFMT_MSG:

            /* Three padding bytes then the 16-byte pixel format */

            ret = fbvnc_recvall(sock, buf, 19);
            if (ret < 0)
              {
                goto teardown;
              }

            fbvnc_parsepixelfmt(buf, &g_fmt);
            break;

          case RFB_SETENCODINGS_MSG:
            ret = fbvnc_recvall(sock, buf, 3);
            if (ret < 0)
              {
                goto teardown;
              }

            nencodings = rfb_getbe16(&buf[1]);
            hashextile = false;
            hastrle = false;

            for (i = 0; i < nencodings; i++)
              {
                ret = fbvnc_recvall(sock, buf, 4);
                if (ret < 0)
                  {
                    goto teardown;
                  }

                if ((int32_t)rfb_getbe32(buf) == RFB_ENCODING_HEXTILE)
                  {
                    hashextile = true;
                  }
                else if ((int32_t)rfb_getbe32(buf) == RFB_ENCODING_TRLE)
                  {
                    hastrle = true;
                  }
              }

            /* Fall back to Raw unless the client asked for something this
             * server actually implements.  Claiming an encoding it does
             * not produce would corrupt the stream.
             */

            g_encoding = RFB_ENCODING_RAW;

#ifdef CONFIG_NETUTILS_FBVNC_ENCODING_HEXTILE
            if (hashextile)
              {
                g_encoding = RFB_ENCODING_HEXTILE;
              }
#endif

            /* TRLE last, so that it wins where the client takes both:  a
             * tile of few colours costs bits per pixel there and a
             * sub-rectangle each in Hextile.
             */

#ifdef CONFIG_NETUTILS_FBVNC_ENCODING_TRLE
            if (hastrle)
              {
                g_encoding = RFB_ENCODING_TRLE;
              }
#endif

            syslog(LOG_INFO, "fbvnc: client offered %u encodings, "
                             "using %s\n", nencodings,
                   g_encoding == RFB_ENCODING_TRLE ? "TRLE" :
                   g_encoding == RFB_ENCODING_HEXTILE ? "Hextile" : "Raw");
            break;

          case RFB_FBUPDATEREQ_MSG:
            ret = fbvnc_recvall(sock, buf, 9);
            if (ret < 0)
              {
                goto teardown;
              }

            /* buf[0] is the incremental flag.  Zero means the client
             * wants the whole screen, not just what changed.
             */

            if (buf[0] == 0 && g_fbvnc.on_invalidate != NULL)
              {
                g_fbvnc.on_invalidate();
              }

            /* Requests that arrive while a frame is on its way fold into
             * the one after it
             */

            if (!g_updatereq)
              {
                g_updatereq = true;
                sem_post(&g_updatesem);
              }

            break;

          case RFB_KEYEVENT_MSG:
            ret = fbvnc_recvall(sock, buf, 7);
            if (ret < 0)
              {
                goto teardown;
              }

            /* down-flag, 2 pad bytes, then the keysym */

            if (g_fbvnc.on_key != NULL)
              {
                g_fbvnc.on_key(rfb_getbe32(&buf[3]), buf[0] != 0);
              }
            break;

          case RFB_POINTEREVENT_MSG:
            ret = fbvnc_recvall(sock, buf, 5);
            if (ret < 0)
              {
                goto teardown;
              }

            /* button mask, then x and y */

            if (g_fbvnc.on_pointer != NULL)
              {
                g_fbvnc.on_pointer(rfb_getbe16(&buf[1]),
                                       rfb_getbe16(&buf[3]), buf[0]);
              }
            break;

          case RFB_CLIENTCUTTEXT_MSG:
            ret = fbvnc_recvall(sock, buf, 7);
            if (ret < 0)
              {
                goto teardown;
              }

            /* Drop the text itself.  It is read rather than ignored so
             * that the stream stays in sync.
             */

            for (i = rfb_getbe32(&buf[3]); i > 0; i--)
              {
                ret = fbvnc_recvall(sock, buf, 1);
                if (ret < 0)
                  {
                    goto teardown;
                  }
              }
            break;

          default:
            syslog(LOG_WARNING, "fbvnc: unknown message type %u, "
                                "dropping client\n", msgtype);
            goto teardown;
        }
    }

teardown:

  /* The sender may be halfway through a frame:  drop the connection under
   * it so its write fails, then wait for it before the socket is closed.
   */

  g_clientrun = false;
  shutdown(sock, SHUT_RDWR);
  sem_post(&g_updatesem);
  pthread_join(g_sender, NULL);
  sem_destroy(&g_updatesem);
}

/****************************************************************************
 * Name: fbvnc_thread
 ****************************************************************************/

static FAR void *fbvnc_thread(FAR void *arg)
{
  struct timeval tv;
  int listensock = g_fbvnc.listensock;
  int clientsock;

  while (g_fbvnc.running)
    {
      struct pollfd pfd;

      /* Wait for a client with a timeout rather than in accept():  a
       * thread blocked in accept() cannot be woken -- not by closing the
       * socket under it, and not by a receive timeout -- so a server
       * that is asked to stop would hold its port until someone
       * happened to connect.
       */

      pfd.fd      = listensock;
      pfd.events  = POLLIN;
      pfd.revents = 0;

      if (poll(&pfd, 1, 500) <= 0 || (pfd.revents & POLLIN) == 0)
        {
          continue;
        }

      clientsock = accept(listensock, NULL, NULL);
      if (clientsock < 0)
        {
          /* A connection that died while it sat in the backlog comes out
           * of accept() as an error.  That is that connection's problem,
           * not the listener's:  a server that exits here goes silent
           * the first time a client gives up waiting, which is exactly
           * how it was found.
           *
           * Which errno that is depends on how far the connection got
           * before it died -- ECONNABORTED, ENOTCONN and ETIMEDOUT have
           * all been seen -- so listing the survivable ones is a list
           * that is always missing its next entry.  Only the ones that
           * say the listening socket itself is finished end the loop.
           */

          if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
            {
              syslog(LOG_ERR, "fbvnc: accept failed: %d\n", errno);
              break;
            }

          syslog(LOG_WARNING, "fbvnc: dropped a connection that died "
                              "waiting: %d\n", errno);
          continue;
        }

      syslog(LOG_INFO, "fbvnc: client connected\n");

      /* A stuck send must not park the server forever:  the timeout is
       * what gets it back to accepting clients when a connection wedges.
       */

      tv.tv_sec  = VNC_SEND_TIMEOUT;
      tv.tv_usec = 0;
      setsockopt(clientsock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

      fbvnc_handleclient(clientsock);

      close(clientsock);
      g_fbvnc.connected = false;

      if (g_fbvnc.on_disconnect != NULL)
        {
          g_fbvnc.on_disconnect();
        }
    }

  close(listensock);
  g_fbvnc.listensock = -1;
  g_fbvnc.running = false;
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fbvnc_start
 ****************************************************************************/

int fbvnc_start(FAR const struct fbvnc_cfg_s *cfg)
{
  struct sockaddr_in addr;
  pthread_attr_t attr;
  struct sched_param param;
  int optval;
  int ret;

  if (cfg == NULL || cfg->snapshot == NULL)
    {
      return -EINVAL;
    }

  if (g_fbvnc.running)
    {
      return -EALREADY;
    }

  g_fbvnc.snapshot      = cfg->snapshot;
  g_fbvnc.on_connect    = cfg->on_connect;
  g_fbvnc.on_disconnect = cfg->on_disconnect;
  g_fbvnc.on_invalidate = cfg->on_invalidate;
  g_fbvnc.on_pointer    = cfg->on_pointer;
  g_fbvnc.on_key        = cfg->on_key;

  g_w      = cfg->width  != 0 ? cfg->width  : VNC_WIDTH;
  g_h      = cfg->height != 0 ? cfg->height : VNC_HEIGHT;
  g_stride = cfg->stride != 0 ? cfg->stride : g_w * VNC_BYTESPP;
  g_fbvnc.connected     = false;

  /* Listen before reporting success:  a port already in use has to reach
   * whoever asked for the server, not a thread that exits on its own.
   */

  g_fbvnc.listensock = socket(AF_INET, SOCK_STREAM, 0);
  if (g_fbvnc.listensock < 0)
    {
      return -errno;
    }

  optval = 1;
  setsockopt(g_fbvnc.listensock, SOL_SOCKET, SO_REUSEADDR, &optval,
             sizeof(optval));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = HTONS(VNC_PORT);
  addr.sin_addr.s_addr = HTONL(INADDR_ANY);

  if (bind(g_fbvnc.listensock, (FAR struct sockaddr *)&addr,
           sizeof(addr)) < 0 ||
      listen(g_fbvnc.listensock, 1) < 0)
    {
      ret = -errno;
      close(g_fbvnc.listensock);
      g_fbvnc.listensock = -1;
      return ret;
    }

  syslog(LOG_INFO, "fbvnc: listening on port %d\n", VNC_PORT);
  g_fbvnc.running = true;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, VNC_STACKSIZE);

  param.sched_priority = VNC_PRIORITY;
  pthread_attr_setschedparam(&attr, &param);

  ret = pthread_create(&g_fbvnc.thread, &attr, fbvnc_thread, NULL);
  pthread_attr_destroy(&attr);

  if (ret != 0)
    {
      g_fbvnc.running = false;
      close(g_fbvnc.listensock);
      g_fbvnc.listensock = -1;
      return -ret;
    }

  pthread_setname_np(g_fbvnc.thread, "fbvnc");
  return OK;
}

/****************************************************************************
 * Name: fbvnc_stop
 ****************************************************************************/

void fbvnc_stop(void)
{
  if (g_fbvnc.running)
    {
      g_fbvnc.running = false;

      /* The accept() timeout bounds how long this takes;  the thread
       * closes the socket on its way out, so the port is free by the
       * time this returns.
       */

      pthread_join(g_fbvnc.thread, NULL);
    }
}

/****************************************************************************
 * Name: fbvnc_is_connected
 ****************************************************************************/

bool fbvnc_is_connected(void)
{
  return g_fbvnc.connected;
}
