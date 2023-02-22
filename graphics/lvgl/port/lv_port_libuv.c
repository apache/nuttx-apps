/****************************************************************************
 * apps/graphics/lvgl/port/lv_port_libuv.c
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

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <uv.h>
#include <stdlib.h>
#include <lvgl/lvgl.h>
#include "lv_port_libuv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct lv_uv_ctx_s
{
  int fd;
  uv_timer_t ui_timer;
  uv_poll_t ui_refr;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ui_timer_cb
 ****************************************************************************/

static void ui_timer_cb(FAR uv_timer_t *handle)
{
  uint32_t sleep_ms;
  LV_LOG_TRACE("start");

  sleep_ms = lv_timer_handler();

  /* Prevent busy loops. */

  if (sleep_ms == 0)
    {
      sleep_ms = 1;
    }

  uv_timer_start(handle, ui_timer_cb, sleep_ms, 0);
  LV_LOG_TRACE("stop, sleep %" PRIu32 " ms", sleep_ms);
}

/****************************************************************************
 * Name: ui_refr_cb
 ****************************************************************************/

static void ui_refr_cb(FAR uv_poll_t *handle, int status, int events)
{
  LV_LOG_TRACE("start");
  _lv_disp_refr_timer(NULL);
  LV_LOG_TRACE("stop");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lv_port_libuv_init
 *
 * Description:
 *   Add the UI event loop to the uv_loop.
 *
 * Input Parameters:
 *   loop - Pointer to uv_loop.
 *
 * Returned Value:
 *   Pointer to UI event context.
 *
 ****************************************************************************/

FAR void *lv_port_libuv_init(FAR void *loop)
{
  int fd;
  FAR uv_loop_t *ui_loop = loop;
  FAR lv_disp_t *disp;
  FAR struct lv_uv_ctx_s *uv_ctx;

  LV_LOG_INFO("dev: " CONFIG_LV_PORT_UV_POLL_DEVICEPATH " opening...");
  fd = open(CONFIG_LV_PORT_UV_POLL_DEVICEPATH, O_WRONLY);
  if (fd < 0)
    {
      LV_LOG_ERROR(CONFIG_LV_PORT_UV_POLL_DEVICEPATH
                   " open failed: %d", errno);
      return NULL;
    }

  disp = lv_disp_get_default();
  LV_ASSERT_NULL(disp);

  if (disp->refr_timer == NULL)
    {
      LV_LOG_ERROR("disp->refr_timer is NULL");
      close(fd);
      return NULL;
    }

  /* Remove default refr timer. */

  lv_timer_del(disp->refr_timer);
  disp->refr_timer = NULL;

  uv_ctx = malloc(sizeof(struct lv_uv_ctx_s));
  LV_ASSERT_MALLOC(uv_ctx);

  memset(uv_ctx, 0, sizeof(struct lv_uv_ctx_s));
  uv_ctx->fd = fd;

  LV_LOG_INFO("init ui_timer...");
  uv_timer_init(ui_loop, &uv_ctx->ui_timer);
  uv_timer_start(&uv_ctx->ui_timer, ui_timer_cb, 1, 1);

  LV_LOG_INFO("init ui_refr...");
  uv_poll_init(ui_loop, &uv_ctx->ui_refr, fd);
  uv_poll_start(&uv_ctx->ui_refr, UV_WRITABLE, ui_refr_cb);

  LV_LOG_INFO("ui loop start OK");
  return uv_ctx;
}

/****************************************************************************
 * Name: lv_port_libuv_uninit
 *
 * Description:
 *   Remove the UI event loop.
 *
 * Input Parameters:
 *   Pointer to UI event context.
 *
 ****************************************************************************/

void lv_port_libuv_uninit(FAR void *ctx)
{
  FAR struct lv_uv_ctx_s *uv_ctx = ctx;

  if (uv_ctx == NULL)
    {
      LV_LOG_WARN("uv_ctx is NULL");
      return;
    }

  uv_close((FAR uv_handle_t *)&uv_ctx->ui_timer, NULL);
  uv_close((FAR uv_handle_t *)&uv_ctx->ui_refr, NULL);
  close(uv_ctx->fd);
  free(ctx);
  LV_LOG_INFO("ui loop close OK");
}
