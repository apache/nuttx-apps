/****************************************************************************
 * apps/examples/nng_test/pubsub.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void fatal(FAR const char *func, int rv)
{
  fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
}

FAR char *date(void)
{
  time_t now = time(&now);
  FAR struct tm *info = localtime(&now);
  FAR char *text = asctime(info);
  text[strlen(text) - 1] = '\0';
  return text;
}

FAR void *client_thread(pthread_addr_t pvarg)
{
  nng_socket sock;
  int rv;

  sleep(2);

  if ((rv = nng_sub0_open(&sock)) != 0)
    {
      fatal("nng_sub0_open", rv);
      return NULL;
    }

  /* subscribe to everything (empty means all topics) */

  if ((rv = nng_setopt(sock, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0)
    {
      fatal("nng_setopt", rv);
      return NULL;
    }

  if ((rv = nng_dial(sock, "ipc:///pubsub.ipc", NULL, 0)) != 0)
    {
      fatal("nng_dial", rv);
      return NULL;
    }

  for (; ; )
    {
      FAR char *buf = NULL;
      size_t sz;

      if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
          fatal("nng_recv", rv);
          break;
        }

      printf("CLIENT: RECEIVED %s\n", buf);
      nng_free(buf, sz);
    }

  return NULL;  /* Keeps some compilers from complaining */
}

int main(const int argc, const FAR char *argv[])
{
  pthread_t tid;
  nng_socket sock;
  int rv;

  if ((rv = nng_pub0_open(&sock)) != 0)
    {
      fatal("nng_pub0_open", rv);
      return 1;
    }

  rv = pthread_create(&tid, NULL, client_thread, NULL);
  if (rv != 0)
    {
      fatal("main: Failed to create client thread: %d\n", rv);
      return 1;
    }

  if ((rv = nng_listen(sock, "ipc:///pubsub.ipc", NULL, 0)) < 0)
    {
      fatal("nng_listen", rv);
      return 1;
    }

  for (; ; )
    {
      FAR char *d = date();
      printf("SERVER: PUBLISHING DATE %s\n", d);
      if ((rv = nng_send(sock, d, strlen(d) + 1, 0)) != 0)
        {
          fatal("nng_send", rv);
          break;
        }

      sleep(1);
    }

  return 1;
}
