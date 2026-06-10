/****************************************************************************
 * apps/examples/webpanel/ws_terminal.c
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
#include <fcntl.h>
#include <errno.h>
#include <sched.h>
#include <spawn.h>

#include <libwebsockets.h>

#include "ws_terminal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_WEBPANEL_WS_PORT
#  define CONFIG_EXAMPLES_WEBPANEL_WS_PORT 8080
#endif

#ifndef CONFIG_EXAMPLES_WEBPANEL_WS_PRIORITY
#  define CONFIG_EXAMPLES_WEBPANEL_WS_PRIORITY 100
#endif

#ifndef CONFIG_EXAMPLES_WEBPANEL_WS_STACKSIZE
#  define CONFIG_EXAMPLES_WEBPANEL_WS_STACKSIZE 8192
#endif

#ifndef CONFIG_EXAMPLES_WEBPANEL_WS_NSH_STACKSIZE
#  define CONFIG_EXAMPLES_WEBPANEL_WS_NSH_STACKSIZE 4096
#endif

#define WS_IOBUF_SIZE  512

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ws_session
{
  int masterfd;
  pid_t nshpid;
  unsigned char txbuf[LWS_PRE + WS_IOBUF_SIZE];
  size_t txlen;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int ws_spawn_nsh(int masterfd, pid_t *pid);
static int ws_callback(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);
static int ws_daemon(int argc, FAR char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct lws_protocols g_protocols[] =
{
  {
    "",
    ws_callback,
    sizeof(struct ws_session),
    WS_IOBUF_SIZE,
    0, NULL, 0
  },
  LWS_PROTOCOL_LIST_TERM
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ws_spawn_nsh
 *
 * Description:
 *   Spawn an NSH instance attached to a PTY slave.
 *
 * Input Parameters:
 *   masterfd - PTY master descriptor.
 *   pid      - Location to receive spawned task PID.
 *
 * Returned Value:
 *   Zero on success; negated errno on failure.
 *
 ****************************************************************************/

static int ws_spawn_nsh(int masterfd, pid_t *pid)
{
  char slavepath[32];
  FAR char *nshargv[] =
  {
    "nsh", NULL
  };

  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  struct sched_param param;
  int ret;

  ret = grantpt(masterfd);
  if (ret < 0)
    {
      return ret;
    }

  ret = unlockpt(masterfd);
  if (ret < 0)
    {
      return ret;
    }

  ret = ptsname_r(masterfd, slavepath, sizeof(slavepath));
  if (ret < 0)
    {
      return ret;
    }

  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_addopen(&actions, 0, slavepath,
                                   O_RDWR, 0);
  posix_spawn_file_actions_adddup2(&actions, 0, 1);
  posix_spawn_file_actions_adddup2(&actions, 0, 2);

  posix_spawnattr_init(&attr);
  param.sched_priority = CONFIG_EXAMPLES_WEBPANEL_WS_PRIORITY;
  posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSCHEDPARAM);
  posix_spawnattr_setschedparam(&attr, &param);
  posix_spawnattr_setstacksize(&attr,
                               CONFIG_EXAMPLES_WEBPANEL_WS_NSH_STACKSIZE);

  ret = posix_spawn(pid, "nsh", &actions, &attr, nshargv, NULL);

  posix_spawn_file_actions_destroy(&actions);
  posix_spawnattr_destroy(&attr);

  return ret == 0 ? 0 : -ret;
}

/****************************************************************************
 * Name: ws_callback
 *
 * Description:
 *   libwebsockets protocol callback for the NSH terminal relay.
 *
 ****************************************************************************/

static int ws_callback(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user, void *in, size_t len)
{
  struct ws_session *sess = (struct ws_session *)user;
  ssize_t n;

  switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
      sess->masterfd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
      if (sess->masterfd < 0)
        {
          printf("ws_terminal: failed to open PTY\n");
          return -1;
        }

      sess->nshpid = -1;
      if (ws_spawn_nsh(sess->masterfd, &sess->nshpid) < 0)
        {
          printf("ws_terminal: failed to spawn NSH\n");
          close(sess->masterfd);
          sess->masterfd = -1;
          return -1;
        }

      /* Make PTY master non-blocking for polling */

      fcntl(sess->masterfd, F_SETFL,
            fcntl(sess->masterfd, F_GETFL) | O_NONBLOCK);

      sess->txlen = 0;
      printf("ws_terminal: client connected\n");

      lws_set_timer_usecs(wsi, 50 * LWS_USEC_PER_SEC / 1000);
      break;

    case LWS_CALLBACK_RECEIVE:
      if (sess->masterfd >= 0 && len > 0)
        {
          write(sess->masterfd, in, len);
        }

      break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
      if (sess->masterfd < 0)
        {
          break;
        }

      n = read(sess->masterfd,
               &sess->txbuf[LWS_PRE],
               WS_IOBUF_SIZE);
      if (n > 0)
        {
          lws_write(wsi, &sess->txbuf[LWS_PRE], n, LWS_WRITE_TEXT);
          lws_set_timer_usecs(wsi, 10 * LWS_USEC_PER_SEC / 1000);
        }
      else
        {
          lws_set_timer_usecs(wsi, 50 * LWS_USEC_PER_SEC / 1000);
        }

      break;

    case LWS_CALLBACK_TIMER:
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLOSED:
      printf("ws_terminal: client disconnected\n");
      if (sess->nshpid > 0)
        {
          task_delete(sess->nshpid);
          sess->nshpid = -1;
        }

      if (sess->masterfd >= 0)
        {
          close(sess->masterfd);
          sess->masterfd = -1;
        }

      break;

    default:
      break;
    }

  return 0;
}

/****************************************************************************
 * Name: ws_daemon
 *
 * Description:
 *   WebSocket terminal daemon using libwebsockets.
 *
 ****************************************************************************/

static int ws_daemon(int argc, FAR char *argv[])
{
  struct lws_context_creation_info info;
  struct lws_context *context;
  int n;

  lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

  memset(&info, 0, sizeof(info));
  info.port = CONFIG_EXAMPLES_WEBPANEL_WS_PORT;
  info.protocols = g_protocols;
  info.vhost_name = "localhost";
  info.options = 0;

  context = lws_create_context(&info);
  if (context == NULL)
    {
      printf("ws_terminal: ERROR creating lws context\n");
      return EXIT_FAILURE;
    }

  printf("WebPanel: WebSocket terminal listening on port %d\n",
         CONFIG_EXAMPLES_WEBPANEL_WS_PORT);

  n = 0;
  while (n >= 0)
    {
      n = lws_service(context, 100);
    }

  lws_context_destroy(context);
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ws_terminal_start
 *
 * Description:
 *   Start the websocket terminal daemon task.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Task PID on success; negated errno value on failure.
 *
 ****************************************************************************/

int ws_terminal_start(void)
{
  int pid;

  pid = task_create("ws_daemon",
                    CONFIG_EXAMPLES_WEBPANEL_WS_PRIORITY,
                    CONFIG_EXAMPLES_WEBPANEL_WS_STACKSIZE,
                    ws_daemon, NULL);
  if (pid < 0)
    {
      printf("WebPanel: ERROR failed to start WebSocket server: %d\n",
             errno);
      return -errno;
    }

  return pid;
}
