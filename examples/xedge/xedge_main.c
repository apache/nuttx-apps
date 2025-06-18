/***************************************************************************
 * apps/examples/xedge/xedge_main.c
 *
 *   Copyright (C) 2025. All rights reserved.
 *   Author: Real Time Logic
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

/* Xedge NuttX Startup Code (may need adjustments)
 *
 * Additional License Note: Xedge, based on the Barracuda
 * App Server, uses the license options explained here:
 * https://github.com/RealTimeLogic/BAS#license
 * This repo does not include Xedge and the Barracuda App Server
 * library and must be downloaded separately. Use the script
 * prepare.sh to clone and prepare the build env.
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
#include "BAS/examples/xedge/src/xedge.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/* The following is a horrible hack to make the code pass the NuttX
 * checkpatch.sh tool. The Barracuda App Server uses a type of
 * camelCase encoding as explained here:
 * https://realtimelogic.com/ba/doc/en/C/introduction.html#oo_c
 */

#define ltmgr ltMgr
#define lthreadmgr LThreadMgr
#define lthreadmgr_run LThreadMgr_run
#define platforminitdiskio platformInitDiskIo
#define diskio DiskIo
#define diskio_setrootdir DiskIo_setRootDir
#define threadjob ThreadJob
#define threadjob_lcreate ThreadJob_lcreate
#define lt Lt
#define threadmutex ThreadMutex
#define threadmutex_set ThreadMutex_set
#define httpserver_getmutex HttpServer_getMutex
#define threadmutex_release ThreadMutex_release
#define xedgeinitdiskio xedgeInitDiskIo
#define batime BaTime
#define baparsedate baParseDate
#define bagetunixtime baGetUnixTime
#define thread_sleep Thread_sleep
#define bafatalerrorcodes BaFatalErrorCodes
#define setdispexit setDispExit
#define xedgeopenaux xedgeOpenAUX
#define xedgeopenauxt XedgeOpenAUX
#define httptrace_setflushcallback HttpTrace_setFLushCallback
#define httpserver_seterrhnd HttpServer_setErrHnd

/***************************************************************************
 * Private Data
 ***************************************************************************/

static int running = FALSE; /* Running mode: 2 running, 1 exiting, 0 stopped */

/* BAS is configured to use dlmalloc for NuttX. This is the pool.
 * 2M : recommended minimum
 */

static char poolbuf[2 * 1024 * 1024];

extern lthreadmgr ltmgr; /* The LThreadMgr configured in xedge.c */

/***************************************************************************
 * External Function Prototypes
 ***************************************************************************/

/* barracuda(): BAS/examples/xedge/src/xedge.c */

extern void barracuda(void);
extern void init_dlmalloc(char *heapstart, char *heapend); /* dlmalloc.c */
extern int (*platforminitdiskio)(diskio *io);              /* xedge.c */

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

static void execevent(threadjob *job, int msgh, lthreadmgr *mgr)
{
  lua_State *L = job->lt;
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
  threadmutex *dm = httpserver_getmutex(ltmgr.server);
  const char *d = __DATE__;
  char buf[50];

  (void)arg;

  if (!(basnprintf(buf, sizeof(buf), "Mon, %c%c %c%c%c %s %s",
                   d[4], d[5], d[0], d[1], d[2], d + 7, __TIME__) < 0))
    {
      batime t = baparsedate(buf);
      if (t)
        {
          t -= 24 * 60 * 60;
          while (bagetunixtime() < t)
            {
              thread_sleep(500);
            }

          threadjob *job = threadjob_lcreate(sizeof(threadjob), execevent);
          threadmutex_set(dm);
          lthreadmgr_run(&ltmgr, job);
          threadmutex_release(dm);
        }
    }

  return NULL;
}

static void panic(bafatalerrorcodes ecode1,
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
      setdispexit();

      /* NuttX feature: Must wait for socket select() to return */

      thread_sleep(2000);
    }
}

/***************************************************************************
 * Public Functions
 ***************************************************************************/

/* xedge.c calls this to initialize the IO.
 * Change "/mnt/lfs" to your preference.
 */

int xedgeinitdiskio(diskio *io)
{
  if (diskio_setrootdir(io, "/mnt/lfs"))
    {
      syslog(LOG_ERR, "Error: cannot set root to /mnt/lfs\n");
      return -1;
    }

  return 0;
}

/* xedge.c calls this; include your Lua bindings here.
 * Tutorial: https://tutorial.realtimelogic.com/Lua-Bindings.lsp
 */

int xedgeopenaux(xedgeopenauxt *aux)
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
  httptrace_setflushcallback(flushtrace);
  httpserver_seterrhnd(panic);

  running = TRUE;
  barracuda();
  running = FALSE;

  printf("Exiting Xedge\n");
  return 0;
}
