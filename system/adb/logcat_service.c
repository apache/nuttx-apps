/****************************************************************************
 * apps/system/adb/logcat_service_uv.c
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
#include <stdlib.h>

#include <nuttx/syslog/ramlog.h>
#include <unistd.h>

#include "adb.h"
#include "logcat_service.h"
#include "hal/hal_uv_priv.h"

/****************************************************************************
 * Private types
 ****************************************************************************/

typedef struct alog_service_s
{
  adb_service_t service;
  uv_poll_t poll;
  int wait_ack;
} alog_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void logcat_on_data_available(uv_poll_t * handle,
                                     int status, int events);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int alog_on_write(adb_service_t *service, apacket *p)
{
  UNUSED(p);
  UNUSED(service);
  return -1;
}

static void alog_on_kick(struct adb_service_s *service)
{
  alog_service_t *svc = container_of(service, alog_service_t, service);
  if (!svc->wait_ack)
    {
      int ret;
      ret = uv_poll_start(&svc->poll, UV_READABLE, logcat_on_data_available);
      assert(ret == 0);
    }
}

static int alog_on_ack(adb_service_t *service, apacket *p)
{
  UNUSED(p);
  alog_service_t *svc = container_of(service, alog_service_t, service);
  svc->wait_ack = 0;
  alog_on_kick(service);
  return 0;
}

static void close_cb(uv_handle_t *handle)
{
  alog_service_t *service = container_of(handle, alog_service_t, poll);
  free(service);
}

static void alog_close(struct adb_service_s *service)
{
  int fd;
  int ret;
  alog_service_t *svc = container_of(service, alog_service_t, service);

  ret = uv_fileno((uv_handle_t *)&svc->poll, &fd);
  assert(ret == 0);

  close(fd);
  uv_close((uv_handle_t *)&svc->poll, close_cb);
}

static const adb_service_ops_t logcat_ops =
{
  .on_write_frame = alog_on_write,
  .on_ack_frame   = alog_on_ack,
  .on_kick        = alog_on_kick,
  .close          = alog_close
};

static void logcat_on_data_available(uv_poll_t * handle,
                                     int status, int events)
{
  int ret;
  int fd;
  apacket_uv_t *ap;
  alog_service_t *service = container_of(handle, alog_service_t, poll);
  adb_client_uv_t *client = (adb_client_uv_t *)handle->data;

  ap = adb_uv_packet_allocate(client, 0);
  if (ap == NULL)
    {
      uv_poll_stop(handle);
      return;
    }

  if (status)
    {
      adb_log("status error %d\n", status);

      /* Fatal error, stop service */

      goto exit_stop_service;
    }

  assert(uv_fileno((uv_handle_t *)handle, &fd) == 0);
  ret = read(fd, ap->p.data, CONFIG_ADBD_PAYLOAD_SIZE);

  if (ret < 0)
    {
      adb_log("frame read failed %d %d\n", ret, errno);
      if (errno == EAGAIN)
        {
          /* TODO this should never happen */

          goto exit_release_packet;
        }

      /* Fatal error, stop service */

      goto exit_stop_service;
    }

  if (ret == 0)
    {
      goto exit_release_packet;
    }

  service->wait_ack = 1;
  uv_poll_stop(handle);

  ap->p.write_len = ret;
  ap->p.msg.arg0 = service->service.id;
  ap->p.msg.arg1 = service->service.peer_id;
  adb_send_data_frame(&client->client, &ap->p);
  return;

exit_release_packet:
  adb_hal_apacket_release(&client->client, &ap->p);
  return;

exit_stop_service:
  adb_service_close(&client->client, &service->service, &ap->p);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

adb_service_t * logcat_service(adb_client_t *client, const char *params)
{
  int ret;
  alog_service_t *service =
      (alog_service_t *)malloc(sizeof(alog_service_t));

  if (service == NULL)
    {
      return NULL;
    }

  service->service.ops = &logcat_ops;
  service->wait_ack = 0;

  /* TODO parse params string to extract logcat parameters */

  ret = open(CONFIG_SYSLOG_DEVPATH, O_RDONLY | O_CLOEXEC);

  if (ret < 0)
    {
      adb_log("failed to open %s (%d)\n", CONFIG_SYSLOG_DEVPATH, errno);
      free(service);
      return NULL;
    }

  uv_handle_t *handle = adb_uv_get_client_handle(client);
  ret = uv_poll_init(handle->loop, &service->poll, ret);
  assert(ret == 0);

  service->poll.data = client;
  alog_on_kick(&service->service);

  return &service->service;
}
