/****************************************************************************
 * apps/examples/nxterm/nximage_listener.c
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
#include <errno.h>

#include <nuttx/nx/nx.h>

#include "nximage.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nximage_listener
 ****************************************************************************/

FAR void *nximage_listener(FAR void *arg)
{
  int ret;

  /* Process events forever */

  for (;;)
    {
      /* Handle the next event.  If we were configured blocking, then
       * we will stay right here until the next event is received.  Since
       * we have dedicated a while thread to servicing events, it would
       * be most natural to also select CONFIG_NX_BLOCKING -- if not, the
       * following would be a tight infinite loop (unless we added addition
       * logic with nx_eventnotify and sigwait to pace it).
       */

      ret = nx_eventhandler(g_nximage.hnx);
      if (ret < 0)
        {
          /* An error occurred... assume that we have lost connection with
           * the server.
           */

          printf("nximage_listener: Lost server connection: %d\n", errno);
          exit(EXIT_FAILURE);
        }

      /* If we received a message, we must be connected */

      if (!g_nximage.connected)
        {
          g_nximage.connected = true;
          sem_post(&g_nximage.eventsem);
          printf("nximage_listener: Connected\n");
        }
    }
}
