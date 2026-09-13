/****************************************************************************
 * apps/examples/zbus/zbus_main.c
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

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <system/zbus.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct acc_msg
{
  int x;
  int y;
  int z;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void listener_callback(const struct zbus_channel *chan);

/****************************************************************************
 * Channel and observer definitions
 ****************************************************************************/

ZBUS_LISTENER_DEFINE(acc_listener, listener_callback);
ZBUS_SUBSCRIBER_DEFINE(acc_subscriber, 4);

ZBUS_CHAN_DEFINE(acc_chan,                       /* Name */
                 struct acc_msg,                 /* Message type */
                 NULL,                           /* Validator */
                 NULL,                           /* User data */
                 ZBUS_OBSERVERS(acc_listener,    /* Observers */
                                acc_subscriber),
                 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0));

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void listener_callback(const struct zbus_channel *chan)
{
  const struct acc_msg *msg = zbus_chan_const_msg(chan);

  printf("zbus:  listener: x=%d y=%d z=%d\n", msg->x, msg->y, msg->z);
}

static void *subscriber_thread(void *arg)
{
  const struct zbus_channel *chan;
  struct acc_msg msg;
  int i;

  for (i = 0; i < 5; i++)
    {
      if (zbus_sub_wait(&acc_subscriber, &chan, 2000) != 0)
        {
          printf("zbus:  subscriber: timeout!\n");
          continue;
        }

      if (chan == &acc_chan)
        {
          zbus_chan_read(chan, &msg, 500);
          printf("zbus:  subscriber: x=%d y=%d z=%d\n",
                 msg.x, msg.y, msg.z);
        }
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct acc_msg msg;
  pthread_t thread;
  int ret;
  int i;

  printf("zbus: publishing 5 messages to acc_chan\n");

  ret = pthread_create(&thread, NULL, subscriber_thread, NULL);
  if (ret != 0)
    {
      printf("zbus: could not create subscriber thread: %d\n", ret);
      return 1;
    }

  for (i = 1; i <= 5; i++)
    {
      msg.x = i;
      msg.y = i * 10;
      msg.z = i * 100;

      ret = zbus_chan_pub(&acc_chan, &msg, 1000);
      if (ret != 0)
        {
          printf("zbus: publish error: %d\n", ret);
        }

      /* Mask the listener notifications on the third message to
       * demonstrate the notification mask API.
       */

      if (i == 3)
        {
          zbus_obs_set_chan_notification_mask(&acc_listener, &acc_chan,
                                              true);
          printf("zbus: listener masked\n");
        }
      else if (i == 4)
        {
          zbus_obs_set_chan_notification_mask(&acc_listener, &acc_chan,
                                              false);
          printf("zbus: listener unmasked\n");
        }

      usleep(100 * 1000);
    }

  pthread_join(thread, NULL);

  printf("zbus: done\n");

  return 0;
}
