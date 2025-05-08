/*
  Xedge Startup Code

  Xedge, including this startup code, requires the Barracuda App
  Server library and is licensed using the three license options as
  explained here: https://github.com/RealTimeLogic/BAS#license

*/
#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>
#include <netutils/ntpclient.h>
#include <HttpTrace.h>
#include <BaDiskIo.h>
#include <BaServerLib.h>
#include "BAS/examples/xedge/src/xedge.h"

extern void barracuda();  /* BAS/examples/xedge/src/xedge.c */
extern void init_dlmalloc(char* heapstart, char* heapend); /* dlmalloc.c */
extern int (*platformInitDiskIo)(DiskIo*); /* xedge.c */

/* The LThreadMgr configured in xedge.c */
extern LThreadMgr ltMgr;

/* BAS is configured to use dlmalloc for NuttX. This is the pool.
*/
static char poolBuf[2*1024*1024]; /* 2M : recommended minimum */


/* The following two functions are copied from the example:
   https://github.com/RealTimeLogic/BAS/blob/main/examples/xedge/src/led.c
   Details:
   https://realtimelogic.com/ba/examples/xedge/readme.html#time
 */

/* This callback is called by one of the threads managed by LThreadMgr
 * when a job is taken off the queue and executed. The callback
 * attempts to find the global Lua function '_XedgeEvent', and if the
 * function is found, it will be executed as follows: _XedgeEvent("sntp")
 */
static void executeXedgeEvent(ThreadJob* job, int msgh, LThreadMgr* mgr)
{
   lua_State* L = job->Lt;
   lua_pushglobaltable(L); /* _G */
   lua_getfield(L, -1, "_XedgeEvent");
   if(lua_isfunction(L, -1))  /* Do we have _G._XedgeEvent */
   {
      /* Call _XedgeEvent("sntp") */
      lua_pushstring(L,"sntp"); /* Arg */
      lua_pcall(L, 1, 0, msgh); /* one arg, no return value */
   }
}


/* Thread started by xedgeOpenAUX() */
static void*
checkTimeThread(void *arg)
{
   ThreadMutex* soDispMutex = HttpServer_getMutex(ltMgr.server);
   (void)arg;
   /* Use the compile time macros for date and time and convert the
    * date/time to a value that can be used by function baParseDate
    */
   const char* d = __DATE__;
   char buf[50];
   if (!(basnprintf(buf, sizeof(buf), "Mon, %c%c %c%c%c %s %s",
                    d[4],d[5], d[0],d[1],d[2], d + 7, __TIME__) < 0))
   {
      BaTime compileT = baParseDate(buf);
      if(compileT) /* If OK: Seconds since 1970 */
      {
         compileT -= 24*60*60; /* Give it one day for time zone adj. */
         /* Wait for time to be updated by NTP */
         while(baGetUnixTime() < compileT)
            Thread_sleep(500);
         /* Initiate executing the Lua func _XedgeEvent("sntp") */
         ThreadJob* job=ThreadJob_lcreate(sizeof(ThreadJob), executeXedgeEvent);
         ThreadMutex_set(soDispMutex);
         LThreadMgr_run(&ltMgr, job);
         ThreadMutex_release(soDispMutex);
      }
   }
   /* Exit thread */
   return NULL;
}

/* xedge.c calls this to initialize the IO.
   Change "/mnt/lfs" to your preference.
 */
int xedgeInitDiskIo(DiskIo* io)
{
   if(DiskIo_setRootDir(io,"/mnt/lfs"))
   {
      syslog(LOG_ERR,"Error: cannot set root to /mnt/lfs\n");
      return -1;
   }
   return 0;
}


/*
  xedge.c calls this; include your Lua bindings here.
  Tutorial: https://tutorial.realtimelogic.com/Lua-Bindings.lsp
*/
int xedgeOpenAUX(XedgeOpenAUX* aux)
{
   (void)aux;

   /* Start thread waiting for time to be set */
   pthread_t thread;
   pthread_attr_t attr;
   struct sched_param param;
   pthread_attr_init(&attr);
   pthread_attr_setstacksize(&attr, 4096);
   param.sched_priority = SCHED_PRIORITY_DEFAULT;
   pthread_attr_setschedparam(&attr, &param);
   pthread_create(&thread, &attr, checkTimeThread, NULL);

   return 0; /* OK */
}


static void
myErrHandler(BaFatalErrorCodes ecode1,
             unsigned int ecode2,
             const char* file,
             int line)
{
   syslog(LOG_ERR,"Fatal error in Barracuda %d %d %s %d\n", ecode1, ecode2, file, line);
   exit(1);
}


/* Redirect server's HttpTrace to syslog.
   https://realtimelogic.com/ba/doc/en/C/reference/html/structHttpTrace.html
 */
static void
flushTrace(char* buf, int bufLen)
{
   buf[bufLen]=0; /* Zero terminate. Bufsize is at least bufLen+1. */
   syslog(LOG_INFO,"%s",buf);
}

static BaBool isRunning; /* Flag used for running state */

/* CTRL-C handler makes Xedge perform a graceful shutdown.
 */
static void
sigHandler(int signo)
{
   if(isRunning)
   {
      isRunning=FALSE;
      printf("\nGot SIGTERM; exiting...\n");
      setDispExit(); /* Graceful shutdown : xedge.c */
      Thread_sleep(2000);
   }
}


int main(int argc, FAR char *argv[])
{
   signal(SIGINT, sigHandler);
   signal(SIGTERM, sigHandler);
   ntpc_start(); /* Make sure it runs; time is reqired when using TLS */
   init_dlmalloc(poolBuf, poolBuf + sizeof(poolBuf));
   HttpTrace_setFLushCallback(flushTrace);
   HttpServer_setErrHnd(myErrHandler);
   isRunning=true;
   barracuda(); /* xedge.c; does not return unless setDispExit() is called */
   printf("Exiting Xedge\n");
   return 0;
}
