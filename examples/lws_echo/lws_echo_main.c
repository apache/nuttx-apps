/****************************************************************************
 * apps/examples/lws_echo/lws_echo_main.c
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

#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_LWS_ECHO_PORT
#  define CONFIG_EXAMPLES_LWS_ECHO_PORT 9000
#endif

#define LWS_ECHO_IOBUF_SIZE 256

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct lws_echo_session_s
{
  unsigned char buf[LWS_PRE + LWS_ECHO_IOBUF_SIZE];
  size_t len;
  int pending;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int callback_echo(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user, void *in, size_t len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct lws_protocols g_protocols[] =
{
  {
    "echo-protocol",
    callback_echo,
    sizeof(struct lws_echo_session_s),
    LWS_ECHO_IOBUF_SIZE,
    0, NULL, 0
  },
  LWS_PROTOCOL_LIST_TERM
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: callback_echo
 *
 * Description:
 *   libwebsockets protocol callback for the echo server.
 *
 * Input Parameters:
 *   wsi    - Websocket instance.
 *   reason - Callback reason.
 *   user   - Per-session data.
 *   in     - Received data.
 *   len    - Length of received data.
 *
 * Returned Value:
 *   Zero on success; non-zero on failure.
 *
 ****************************************************************************/

static int callback_echo(struct lws *wsi,
                         enum lws_callback_reasons reason,
                         void *user, void *in, size_t len)
{
  struct lws_echo_session_s *pss = (struct lws_echo_session_s *)user;

  switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
      printf("lws_echo: client connected\n");
      pss->pending = 0;
      break;

    case LWS_CALLBACK_RECEIVE:
      if (len > sizeof(pss->buf) - LWS_PRE)
        {
          len = sizeof(pss->buf) - LWS_PRE;
        }

      memcpy(&pss->buf[LWS_PRE], in, len);
      pss->len = len;
      pss->pending = 1;
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
      if (!pss->pending)
        {
          break;
        }

      lws_write(wsi, &pss->buf[LWS_PRE], pss->len, LWS_WRITE_TEXT);
      pss->pending = 0;
      break;

    case LWS_CALLBACK_CLOSED:
      printf("lws_echo: client disconnected\n");
      break;

    default:
      break;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *   Start a libwebsockets WebSocket echo server.
 *
 * Input Parameters:
 *   Standard argc and argv.
 *
 * Returned Value:
 *   Zero on success; non-zero on failure.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct lws_context_creation_info info;
  struct lws_context *context;
  int n;

  lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

  printf("lws_echo: starting WebSocket echo server on port %d\n",
         CONFIG_EXAMPLES_LWS_ECHO_PORT);
  printf("lws_echo: connect with ws://<device-ip>:%d "
         "(subprotocol echo-protocol)\n",
         CONFIG_EXAMPLES_LWS_ECHO_PORT);

  memset(&info, 0, sizeof(info));
  info.port = CONFIG_EXAMPLES_LWS_ECHO_PORT;
  info.protocols = g_protocols;
  info.vhost_name = "localhost";
  info.options = 0;

  context = lws_create_context(&info);
  if (context == NULL)
    {
      printf("lws_echo: ERROR creating context\n");
      return 1;
    }

  printf("lws_echo: server running\n");

  n = 0;
  while (n >= 0)
    {
      n = lws_service(context, 100);
    }

  lws_context_destroy(context);
  printf("lws_echo: server stopped\n");

  return 0;
}
