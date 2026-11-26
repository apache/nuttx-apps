/****************************************************************************
 * apps/include/netutils/fbvnc.h
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

#ifndef __APPS_INCLUDE_NETUTILS_FBVNC_H
#define __APPS_INCLUDE_NETUTILS_FBVNC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Dirty rectangle descriptor */

struct fbvnc_rect_s
{
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
};

/* Snapshot callback.
 *
 * Called by the server whenever a client asks for a framebuffer update.
 * The caller owns the pixel data:  this is what keeps the server free of
 * a framebuffer of its own, so that it can stream the display's own
 * memory without a copy.
 *
 * The callback fills in the rectangles that changed since the last call
 * and returns the base of the framebuffer, or NULL if no snapshot could
 * be taken.  Returning zero rectangles is not an error;  it means
 * nothing changed.
 *
 * Note that this runs on the server thread.  Do not block on a lock that
 * the render thread may already hold.
 */

typedef CODE FAR const uint8_t *
  (*fbvnc_snapshot_t)(FAR void *ctx, FAR struct fbvnc_rect_s *rects,
                      uint32_t maxrects, FAR uint32_t *nrects);

/* Connection notification callback */

typedef CODE void (*fbvnc_event_t)(FAR void *ctx);

/* Remote input callbacks.  Both run on the server thread:  hand the event
 * to the UI thread, do not call into the UI from here.
 *
 * Pointer:  position in framebuffer coordinates plus the RFB button mask
 * (bit 0 = left, 1 = middle, 2 = right, bits 3-6 = scroll wheel notches
 * encoded as press+release pairs).
 *
 * Key:  the X11 keysym as the client sent it.  Printable characters
 * arrive already shifted, Shift+a comes in as 'A', so only special
 * keys (arrows, enter, backspace...) need translating.
 */

typedef CODE void (*fbvnc_pointer_t)(FAR void *ctx, uint16_t x,
                                     uint16_t y, uint8_t buttons);
typedef CODE void (*fbvnc_key_t)(FAR void *ctx, uint32_t keysym,
                                 bool pressed);

/* Server configuration.  fbvnc_start() copies it, so the caller may let
 * it go out of scope once the server is running.
 */

struct fbvnc_cfg_s
{
  fbvnc_snapshot_t snapshot;      /* Mandatory */
  fbvnc_event_t    on_connect;    /* Optional, may be NULL */
  fbvnc_event_t    on_disconnect; /* Optional, may be NULL */

  /* Optional.  Invoked when the client asks for a non-incremental
   * update, i.e. it wants the whole screen again.  Use it to force the
   * next snapshot to report the full canvas as dirty.
   */

  fbvnc_event_t    on_invalidate;

  /* Optional.  Remote input; NULL means the events are discarded. */

  fbvnc_pointer_t  on_pointer;
  fbvnc_key_t      on_key;

  /* Handed back to every callback above, so that a caller serving more
   * than one framebuffer can tell them apart without a global.
   */

  FAR void *ctx;

  /* Geometry of the served framebuffer.  Zero means the compile-time
   * defaults;  a daemon serving an arbitrary framebuffer fills these in
   * from what the device reports.  Only 16-bit RGB565 is served either
   * way.
   */

  uint16_t width;
  uint16_t height;
  uint16_t stride;               /* Bytes per row */

  /* TCP port to listen on.  Zero means CONFIG_NETUTILS_FBVNC_PORT, so
   * that what used to be fixed at compile time can now be chosen when
   * the server starts.
   */

  uint16_t port;
};

/* Opaque server instance, returned by fbvnc_start() and handed back to
 * every other call.  Nothing about a running server lives outside it.
 */

struct fbvnc_s;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: fbvnc_start
 *
 * Description:
 *   Create a VNC server and start listening.  The server accepts one
 *   client at a time.  Call fbvnc_run() to serve it.
 *
 * Input Parameters:
 *   cfg - Server configuration.  The snapshot callback is mandatory.
 *
 * Returned Value:
 *   The server instance on success;  NULL on failure, with errno set.
 *
 ****************************************************************************/

FAR struct fbvnc_s *fbvnc_start(FAR const struct fbvnc_cfg_s *cfg);

/****************************************************************************
 * Name: fbvnc_run
 *
 * Description:
 *   Serve clients until fbvnc_stop() is called, in the calling thread.
 *   A caller that has nothing else to do runs the server here rather
 *   than paying for a thread that only waits for it.
 *
 * Input Parameters:
 *   dev - The instance from fbvnc_start().
 *
 * Returned Value:
 *   Zero (OK) once stopped;  a negated errno value on failure.
 *
 ****************************************************************************/

int fbvnc_run(FAR struct fbvnc_s *dev);

/****************************************************************************
 * Name: fbvnc_stop
 *
 * Description:
 *   Stop the server and release it.  Closes the client connection if
 *   there is one and makes fbvnc_run() return.
 *
 * Input Parameters:
 *   dev - The instance from fbvnc_start().
 *
 ****************************************************************************/

void fbvnc_stop(FAR struct fbvnc_s *dev);

/****************************************************************************
 * Name: fbvnc_is_connected
 *
 * Description:
 *   Return true if a VNC client is currently connected.
 *
 * Input Parameters:
 *   dev - The instance from fbvnc_start().
 *
 ****************************************************************************/

bool fbvnc_is_connected(FAR struct fbvnc_s *dev);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_FBVNC_H */
