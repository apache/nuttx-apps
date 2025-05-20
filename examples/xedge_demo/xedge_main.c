/***************************************************************************
 * apps/examples/xedge_demo/xedge_main.c
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
 ***************************************************************************/

/* Xedge NuttX Startup Code (may need adjustments)
 *
 * Additional License Note: Xedge, based on the Barracuda
 * App Server, uses the license options explained here:
 * https://github.com/RealTimeLogic/BAS#license
 * This repo does not include Xedge and the Barracuda App Server
 * library and must be downloaded separately. The dependencies
 * are automatically downloaded in apps/netutils/xedge/.
 */

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include <netutils/ntpclient.h>
#include <HttpTrace.h>
#include <BaDiskIo.h>
#include <BaServerLib.h>
#include "../../netutils/xedge/BAS/examples/xedge/src/xedge.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/* The code uses identifiers from the third-party Barracuda App Server
 * library, which follows a camelCase naming style:
 * https://realtimelogic.com/ba/doc/en/C/introduction.html#oo_c
 * These identifiers are used directly and will trigger "Mixed Case"
 * warnings in tools like nxstyle, which are safe to ignore in this case.
 */

/***************************************************************************
 * Private Data
 ***************************************************************************/

static int running = FALSE; /* Running mode: 2 running, 1 exiting, 0 stopped */

/* BAS is configured to use dlmalloc for NuttX. This is the pool.
 * 2M : recommended minimum
 */

static char poolbuf[2 * 1024 * 1024];

extern LThreadMgr ltMgr; /* The LThreadMgr configured in xedge.c */

/***************************************************************************
 * External Function Prototypes
 ***************************************************************************/

/* barracuda(): BAS/examples/xedge/src/xedge.c */

extern void barracuda(void);
extern void init_dlmalloc(char *heapstart, char *heapend); /* dlmalloc.c */
extern int (*platformInitDiskIo)(DiskIo *io);              /* xedge.c */

/***************************************************************************
 * Private Functions
 ***************************************************************************/

/* The following two functions are copied from the example:
 * https://github.com/RealTimeLogic/BAS/blob/main/examples/xedge/src/led.c
 * Details:
 * https://realtimelogic.com/ba/examples/xedge/readme.html#time
 */

/* This callback is called by one of the threads managed by LThreadMgr
 * when a job is taken off the queue and executed. The callback
 * attempts to find the global Lua function '_XedgeEvent', and if the
 * function is found, it will be executed as follows: _XedgeEvent("sntp")
 */

static void execevent(ThreadJob *job, int msgh, LThreadMgr *mgr)
{
  lua_State *L = job->Lt;
  lua_pushglobaltable(L);
  lua_getfield(L, -1, "_XedgeEvent");

  if (lua_isfunction(L, -1))
    {
      lua_pushstring(L, "sntp");
      lua_pcall(L, 1, 0, msgh);
    }
}

/* Thread started by xedgeOpenAUX() */

static void *checktimethread(void *arg)
{
  ThreadMutex *dm = HttpServer_getMutex(ltMgr.server);
  const char *d = __DATE__;
  char buf[50];

  (void)arg;

  if (!(basnprintf(buf, sizeof(buf), "Mon, %c%c %c%c%c %s %s",
                   d[4], d[5], d[0], d[1], d[2], d + 7, __TIME__) < 0))
    {
      BaTime t = baParseDate(buf);
      if (t)
        {
          t -= 24 * 60 * 60;
          while (baGetUnixTime() < t)
            {
              Thread_sleep(500);
            }

          ThreadJob *job = ThreadJob_lcreate(sizeof(ThreadJob), execevent);
          ThreadMutex_set(dm);
          LThreadMgr_run(&ltMgr, job);
          ThreadMutex_release(dm);
        }
    }

  return NULL;
}

static void panic(BaFatalErrorCodes ecode1,
                         unsigned int ecode2,
                         const char *file,
                         int line)
{
  syslog(LOG_ERR, "Fatal error in Barracuda %d %d %s %d\n",
         ecode1, ecode2, file, line);
  exit(1);
}

/* Redirect server's HttpTrace to syslog.
 * https://realtimelogic.com/ba/doc/en/C/reference/html/structHttpTrace.html
 */

static void flushtrace(char *buf, int buflen)
{
  buf[buflen] = 0;
  syslog(LOG_INFO, "%s", buf);
}

static void sighandler(int signo)
{
  if (running)
    {
      printf("\nGot SIGTERM; exiting...\n");
      setDispExit();

      /* NuttX feature: Must wait for socket select() to return */

      Thread_sleep(2000);
    }
}

/***************************************************************************
 * Public Functions
 ***************************************************************************/

/* xedge.c calls this to initialize the IO.
 * Change "/mnt/lfs" to your preference.
 */

int xedgeInitDiskIo(DiskIo *io)
{
  if (DiskIo_setRootDir(io, "/mnt/lfs"))
    {
      syslog(LOG_ERR, "Error: cannot set root to /mnt/lfs\n");
      return -1;
    }

  return 0;
}

/* xedge.c calls this; include your Lua bindings here.
 * Tutorial: https://tutorial.realtimelogic.com/Lua-Bindings.lsp
 */

int xedgeOpenAUX(XedgeOpenAUX *aux)
{
  pthread_t thread;
  pthread_attr_t attr;
  struct sched_param param;

  (void)aux;

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 4096);
  param.sched_priority = SCHED_PRIORITY_DEFAULT;
  pthread_attr_setschedparam(&attr, &param);
  pthread_create(&thread, &attr, checktimethread, NULL);

  return 0;
}

int main(int argc, FAR char *argv[])
{
  if (running)
    {
      printf("Already running!\n");
      return 1;
    }

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  ntpc_start();
  init_dlmalloc(poolbuf, poolbuf + sizeof(poolbuf));
  HttpTrace_setFLushCallback(flushtrace);
  HttpServer_setErrHnd(panic);

  running = TRUE;
  barracuda();
  running = FALSE;

  printf("Exiting Xedge\n");
  return 0;
}
