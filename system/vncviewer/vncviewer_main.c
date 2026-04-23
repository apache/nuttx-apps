/****************************************************************************
 * apps/system/vncviewer/vncviewer_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <nuttx/video/fb.h>

#include "rfb_protocol.h"
#include "lcd_render.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VNCVIEWER_DEFAULT_PORT  5900
#define VNCVIEWER_DEFAULT_LCD   0

#define COLOR_BLACK             0x0000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct lcd_ctx_s g_lcd;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rect_callback
 *
 * Description:
 *   Called for each row of pixel data received from VNC server.
 *   Writes directly to LCD, clipping to screen bounds.
 *
 ****************************************************************************/

static void rect_callback(const struct rfb_rect_hdr_s *rect,
                          const uint8_t *pixels, void *arg)
{
  /* rect->h is always 1 (row-by-row from rfb_recv_update) */

  if (rect->y < g_lcd.yres && rect->x < g_lcd.xres)
    {
      uint16_t w = rect->w;

      if (rect->x + w > g_lcd.xres)
        {
          w = g_lcd.xres - rect->x;
        }

      lcd_put_row(&g_lcd, rect->x, rect->y, w, (const uint16_t *)pixels);
    }
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(const char *progname)
{
  printf("Usage: %s [options] <host> [port]\n", progname);
  printf("Options:\n");
  printf("  -p <password>  VNC password\n");
  printf("  -d <devno>     LCD device number (default: %d)\n",
         VNCVIEWER_DEFAULT_LCD);
  printf("  -h             Show this help\n");
  printf("Default port: %d\n", VNCVIEWER_DEFAULT_PORT);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: vncviewer_main
 *
 * Description:
 *   VNC Viewer entry point. Connects to a VNC server and displays
 *   the remote framebuffer on the local LCD.
 *
 ****************************************************************************/

int main(int argc, char *argv[])
{
  const char *host = NULL;
  const char *password = "";
  uint16_t port = VNCVIEWER_DEFAULT_PORT;
  int lcd_devno = VNCVIEWER_DEFAULT_LCD;
  struct rfb_conn_s conn;
  int opt;
  int ret;

  /* Parse command line arguments */

  while ((opt = getopt(argc, argv, "p:d:h")) != -1)
    {
      switch (opt)
        {
          case 'p':
            password = optarg;
            break;

          case 'd':
            lcd_devno = atoi(optarg);
            break;

          case 'h':
            show_usage(argv[0]);
            return EXIT_SUCCESS;

          default:
            show_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if (optind < argc)
    {
      host = argv[optind++];
    }

  if (optind < argc)
    {
      port = atoi(argv[optind]);
    }

  if (host == NULL)
    {
      printf("vncviewer: host is required\n");
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  /* Initialize LCD */

  printf("vncviewer: initializing LCD...\n");

  ret = lcd_init(&g_lcd, lcd_devno);
  if (ret < 0)
    {
      printf("vncviewer: LCD init failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Fill screen black */

  lcd_fill(&g_lcd, COLOR_BLACK);

  /* Connect to VNC server */

  memset(&conn, 0, sizeof(conn));

  /* Connection loop with retry */

  for (; ; )
    {
      int rcvbuf;

      conn.sockfd = rfb_connect(host, port);
      if (conn.sockfd < 0)
        {
          printf("vncviewer: connection failed: %d, retrying...\n",
                 conn.sockfd);
          sleep(2);
          continue;
        }

      /* Increase TCP receive buffer to handle bursts */

      rcvbuf = 32768;
      setsockopt(conn.sockfd, SOL_SOCKET, SO_RCVBUF,
                 &rcvbuf, sizeof(rcvbuf));

      /* RFB handshake */

      ret = rfb_handshake(&conn, password);
      if (ret < 0)
        {
          printf("vncviewer: handshake failed: %d, retrying...\n", ret);
          close(conn.sockfd);
          sleep(2);
          continue;
        }

      /* Configure pixel format based on LCD */

      ret = rfb_set_pixel_format(&conn, g_lcd.fmt);
      if (ret < 0)
        {
          printf("vncviewer: set pixel format failed: %d, retrying...\n",
                 ret);
          close(conn.sockfd);
          sleep(2);
          continue;
        }

      /* Set encodings: Raw only */

      ret = rfb_set_encodings(&conn);
      if (ret < 0)
        {
          printf("vncviewer: set encodings failed: %d, retrying...\n", ret);
          close(conn.sockfd);
          sleep(2);
          continue;
        }

      /* Determine view size: min of server desktop and LCD */

      conn.view_width  = conn.fb_width < g_lcd.xres
                         ? conn.fb_width : g_lcd.xres;
      conn.view_height = conn.fb_height < g_lcd.yres
                         ? conn.fb_height : g_lcd.yres;

      printf("vncviewer: view area: %ux%u (server: %ux%u, lcd: %ux%u)\n",
             conn.view_width, conn.view_height,
             conn.fb_width, conn.fb_height,
             g_lcd.xres, g_lcd.yres);

      /* Request initial full framebuffer update */

      ret = rfb_request_update(&conn, false,
                               0, 0, conn.view_width, conn.view_height);
      if (ret < 0)
        {
          printf("vncviewer: initial update request failed: %d, "
                 "retrying...\n", ret);
          close(conn.sockfd);
          sleep(2);
          continue;
        }

      /* Main loop: receive updates and render to LCD */

      printf("vncviewer: entering main loop\n");

      while (true)
        {
          /* Receive and process server message */

          ret = rfb_recv_update(&conn, rect_callback, NULL);
          if (ret < 0)
            {
              printf("vncviewer: recv error: %d\n", ret);
              break;
            }

          /* Request next incremental update */

          ret = rfb_request_update(&conn, true,
                                   0, 0,
                                   conn.view_width, conn.view_height);
          if (ret < 0)
            {
              printf("vncviewer: update request failed: %d\n", ret);
              break;
            }
        }

      /* Connection lost - retry */

      printf("vncviewer: reconnecting in 2 seconds...\n");
      close(conn.sockfd);
      sleep(2);
    }

  /* Cleanup */

  printf("vncviewer: disconnecting\n");
  close(conn.sockfd);
  lcd_uninit(&g_lcd);

  return EXIT_SUCCESS;
}
