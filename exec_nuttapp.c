/****************************************************************************
 * drivers/sbin/exec_nuttapp.c
 *
 *   Copyright (C) 2011 Uros Platise. All rights reserved.
 *   Author: Uros Platise <uros.platise@isotel.eu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/nuttapp.h>
#include <sched.h>

#include <string.h>
#include <errno.h>


/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Load builtin function prototypes */

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

#include "exec_nuttapp_proto.h"

static const struct nuttapp_s nuttapps[] = {
# include "exec_nuttapp_list.h"
  {.name=NULL}
};

#undef EXTERN
#if defined(__cplusplus)
}
#endif


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
 * Public Functions
 ****************************************************************************/


const char * nuttapp_getname(int index)
{
  if ( index < 0 || index >= (sizeof(nuttapps)/sizeof(struct nuttapp_s)) )
	return NULL;
	
  return nuttapps[index].name;
}
 

int nuttapp_isavail(FAR const char *appname)
{
  int i;
    
  for (i=0; nuttapps[i].name; i++) 
  {
    if ( !strcmp(nuttapps[i].name, appname) ) 
      return i;
  }
  
  errno = ENOENT;
  return -1;
}
 

int exec_nuttapp(FAR const char *appname, FAR const char *argv[])
{
  int i;
  
  if ( (i = nuttapp_isavail(appname)) >= 0 )
  {
#ifndef CONFIG_CUSTOM_STACK
    i = task_create(nuttapps[i].name, nuttapps[i].priority, 
                      nuttapps[i].stacksize, nuttapps[i].main, 
                      (argv) ? &argv[1] : (const char **)NULL);
#else
    i = task_create(nuttapps[i].name, nuttapps[i].priority, nuttapps[i].main,
                      (argv) ? &argv[1] : (const char **)NULL);
#endif

#if CONFIG_RR_INTERVAL > 0
    if (i > 0)
    {
      struct sched_param param;
	
      sched_getparam(0, &param);
	  sched_setscheduler(i, SCHED_RR, &param);
	}
#endif

    return i;
  }
  
  return i;
}
