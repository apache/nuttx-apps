/****************************************************************************
 * apps/system/vncviewer/rfb_protocol.h
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

#ifndef __APPS_SYSTEM_VNCVIEWER_RFB_PROTOCOL_H
#define __APPS_SYSTEM_VNCVIEWER_RFB_PROTOCOL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RFB Protocol Version */

#define RFB_VERSION_STRING      "RFB 003.008\n"
#define RFB_VERSION_LEN         12

/* Security Types */

#define RFB_SEC_INVALID         0
#define RFB_SEC_NONE            1
#define RFB_SEC_VNC_AUTH        2

/* Client-to-Server Message Types */

#define RFB_SET_PIXEL_FORMAT    0
#define RFB_SET_ENCODINGS       2
#define RFB_FB_UPDATE_REQUEST   3
#define RFB_KEY_EVENT           4
#define RFB_POINTER_EVENT       5
#define RFB_CLIENT_CUT_TEXT     6

/* Server-to-Client Message Types */

#define RFB_FB_UPDATE           0
#define RFB_SET_COLORMAP        1
#define RFB_BELL                2
#define RFB_SERVER_CUT_TEXT     3

/* Encoding Types */

#define RFB_ENCODING_RAW        0

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Pixel format (negotiated with server based on LCD) */

struct rfb_pixel_format_s
{
  uint8_t  bits_per_pixel;    /* 16 */
  uint8_t  depth;             /* 16 */
  uint8_t  big_endian;        /* 0 = little-endian */
  uint8_t  true_color;        /* 1 */
  uint16_t red_max;           /* 31 (5 bits) */
  uint16_t green_max;         /* 63 (6 bits) */
  uint16_t blue_max;          /* 31 (5 bits) */
  uint8_t  red_shift;         /* 11 */
  uint8_t  green_shift;       /* 5 */
  uint8_t  blue_shift;        /* 0 */
  uint8_t  padding[3];
};

/* Server init message (parsed) */

struct rfb_server_init_s
{
  uint16_t fb_width;
  uint16_t fb_height;
  struct rfb_pixel_format_s pixel_format;
  char     name[256];
};

/* Framebuffer update rectangle header */

struct rfb_rect_hdr_s
{
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
  int32_t  encoding;
};

/* RFB connection context */

struct rfb_conn_s
{
  int      sockfd;
  uint16_t fb_width;
  uint16_t fb_height;
  uint16_t view_width;
  uint16_t view_height;
  uint8_t  bpp;             /* Bytes per pixel */
  char     name[256];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: rfb_connect
 *
 * Description:
 *   Establish TCP connection to VNC server.
 *
 * Input Parameters:
 *   host - Server IP address string
 *   port - Server port number
 *
 * Returned Value:
 *   Socket fd on success, negative errno on failure.
 *
 ****************************************************************************/

int rfb_connect(const char *host, uint16_t port);

/****************************************************************************
 * Name: rfb_handshake
 *
 * Description:
 *   Perform RFB protocol handshake: version, security, init.
 *
 * Input Parameters:
 *   conn     - RFB connection context (sockfd must be set)
 *   password - VNC password (NULL or empty for no auth)
 *
 * Returned Value:
 *   OK on success, negative errno on failure.
 *
 ****************************************************************************/

int rfb_handshake(struct rfb_conn_s *conn, const char *password);

/****************************************************************************
 * Name: rfb_set_pixel_format
 *
 * Description:
 *   Send SetPixelFormat message based on LCD pixel format.
 *
 ****************************************************************************/

int rfb_set_pixel_format(struct rfb_conn_s *conn, uint8_t fmt);

/****************************************************************************
 * Name: rfb_set_encodings
 *
 * Description:
 *   Send SetEncodings message (Raw only).
 *
 ****************************************************************************/

int rfb_set_encodings(struct rfb_conn_s *conn);

/****************************************************************************
 * Name: rfb_request_update
 *
 * Description:
 *   Send FramebufferUpdateRequest.
 *
 * Input Parameters:
 *   conn        - RFB connection
 *   incremental - true for incremental update
 *   x, y, w, h  - Region to request
 *
 ****************************************************************************/

int rfb_request_update(struct rfb_conn_s *conn, bool incremental,
                       uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/****************************************************************************
 * Name: rfb_recv_update
 *
 * Description:
 *   Receive and process one server message. For FramebufferUpdate,
 *   calls the provided callback for each rectangle.
 *
 * Input Parameters:
 *   conn - RFB connection
 *   rect_cb - Callback for each received rectangle
 *   arg     - User argument passed to callback
 *
 * Returned Value:
 *   OK on success, negative errno on failure.
 *
 ****************************************************************************/

typedef void (*rfb_rect_cb_t)(const struct rfb_rect_hdr_s *rect,
                              const uint8_t *pixels, void *arg);

int rfb_recv_update(struct rfb_conn_s *conn, rfb_rect_cb_t rect_cb,
                    void *arg);

#endif /* __APPS_SYSTEM_VNCVIEWER_RFB_PROTOCOL_H */
