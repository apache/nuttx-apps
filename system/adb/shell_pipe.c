/****************************************************************************
 * apps/system/adb/shell_pipe.c
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

#include <uv.h>
#include <unistd.h>
#include "adb.h"
#include "shell_pipe.h"
#include "hal/hal_uv_priv.h"

#include <nshlib/nshlib.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void shell_on_data_available(uv_poll_t * handle,
                                    int status, int events)
{
  int ret;
  apacket_uv_t *ap;
  shell_pipe_t *pipe = container_of(handle, shell_pipe_t, handle);

  adb_client_t *client = (adb_client_t *)pipe->handle.data;

  if (status)
    {
      adb_log("status error %d\n", status);

      /* FIXME missing logic here */

      pipe->on_data_cb(pipe, NULL);
      return;
    }

  ap = adb_uv_packet_allocate((adb_client_uv_t *)client, 0);
  if (ap == NULL)
    {
      /* frame allocation failed. Try again later */

      uv_poll_stop(&pipe->handle);
      return;
    }

  int nread = 0;
  do
    {
      ret = read(handle->io_watcher.fd, &ap->p.data[nread], 1);

      if (ret == 0)
        {
          /* EOF */

          break;
        }

      if (ret < 0)
        {
          /* Revisit. EAGAIN should not happen but it happens a lot */

          if (errno == EAGAIN)
            {
              if (nread <= 0)
                {
                  adb_hal_apacket_release(
                    (adb_client_t *)pipe->handle.data, &ap->p);
                  return;
                }
              break;
            }

          /* Release packet and forward error */

          adb_hal_apacket_release((adb_client_t *)pipe->handle.data, &ap->p);
          pipe->on_data_cb(pipe, NULL);
          return;
        }

      /* FIXME CR LF conversion */

      if (ap->p.data[nread++] == '\n')
        {
          ap->p.data[nread++] = '\r';
        }
    }
  while (nread < CONFIG_ADBD_PAYLOAD_SIZE - 1);

  ap->p.msg.data_length = nread;
  pipe->on_data_cb(pipe, &ap->p);
}

static void shell_pipe_close_callback(uv_handle_t *handle)
{
  shell_pipe_t *pipe = container_of(handle, shell_pipe_t, handle);

  /* Close stdout pipe */

  close(pipe->write_fd);

  /* Notify caller pipe is closed */

  pipe->close_cb(pipe);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int shell_pipe_setup(adb_client_t *client, shell_pipe_t *apipe)
{
  apipe->handle.data = client;
  return 0;
}

void shell_pipe_destroy(shell_pipe_t *pipe, void (*close_cb)(shell_pipe_t *))
{
  pipe->close_cb = close_cb;
  close(pipe->write_fd);

  if (uv_fileno((const uv_handle_t *)&pipe->handle, &pipe->write_fd))
    {
      pipe->write_fd = -1;
    }

  uv_close((uv_handle_t *)&pipe->handle, shell_pipe_close_callback);
}

int shell_pipe_write(shell_pipe_t *pipe, const void *buf, size_t count)
{
  /* TODO revisit */

  return write(pipe->write_fd, buf, count);
}

int shell_pipe_start(shell_pipe_t *pipe,
                     void (*on_data_cb)(shell_pipe_t *, apacket *))
{
  pipe->on_data_cb = on_data_cb;
  return uv_poll_start(&pipe->handle, UV_READABLE, shell_on_data_available);
}

int shell_pipe_exec(char * const argv[], shell_pipe_t *apipe,
                    void (*on_data_cb)(shell_pipe_t *, apacket *))
{
  int ret;
  int in_fds[2];
  int out_fds[2];

  adb_client_uv_t *client = (adb_client_uv_t *)apipe->handle.data;

  /* Create pipe for stdin */

  if ((ret = pipe(in_fds)))
    {
      adb_log("failed to open in pipe %d\n", errno);
      goto exit_fail;
    }

  if ((ret = pipe(out_fds)))
    {
      adb_log("failed to open out pipe %d\n", errno);
      goto exit_close_pipe_in;
    }

  apipe->write_fd = in_fds[1];

  /* Setup stdout (read: adb, write: child) */

  ret = dup2(out_fds[1], 1);
  assert(ret == 1);

  ret = close(out_fds[1]);
  assert(ret == 0);

  ret = fcntl(out_fds[0], F_GETFD);
  assert(ret >= 0);
  ret = fcntl(out_fds[0], F_SETFD, ret | FD_CLOEXEC);
  assert(ret == 0);
  ret = fcntl(out_fds[0], F_GETFL);
  assert(ret >= 0);
  ret = fcntl(out_fds[0], F_SETFL, ret | O_NONBLOCK);
  assert(ret >= 0);

  /* Setup stdin */

  ret = dup2(in_fds[0], 0);
  assert(ret == 0);

  ret = close(in_fds[0]);
  assert(ret == 0);

  ret = fcntl(in_fds[1], F_GETFD);
  assert(ret >= 0);
  ret = fcntl(in_fds[1], F_SETFD, ret | FD_CLOEXEC);
  assert(ret == 0);
  ret = fcntl(in_fds[1], F_GETFL);
  assert(ret >= 0);
  ret = fcntl(in_fds[1], F_SETFL, ret | O_NONBLOCK);
  assert(ret == 0);

  ret = uv_poll_init(
      adb_uv_get_client_handle(client)->loop,
      &apipe->handle, out_fds[0]);

  /* TODO check return code */

  assert(ret == 0);

  /* Create shell process */

  ret = task_create("ADB shell", CONFIG_SYSTEM_NSH_PRIORITY,
                    CONFIG_SYSTEM_NSH_STACKSIZE,
                    argv ? nsh_system : nsh_consolemain,
                    argv);

  /* Close stdin and stdout */

  dup2(2, 0);
  dup2(2, 1);

  /* TODO check return code */

  assert(ret >= 0);

  /* Start listening shell process stdout */

  ret = shell_pipe_start(apipe, on_data_cb);

  /* TODO check return code */

  assert(ret == 0);
  return 0;

exit_close_pipe_in:
  close(in_fds[0]);
  close(in_fds[1]);
exit_fail:
  return ret;
}
