/****************************************************************************
 * apps/nshlib/nsh_usbtrace.c
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
#include <debug.h>

#include <nuttx/usb/usbdev_trace.h>

#ifdef CONFIG_NSH_USBDEV_TRACE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_tracecallback
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_tracecallback
 *
 * Description:
 *   This is part of the USB trace logic
 *
 ****************************************************************************/

static int usbtrace_syslog(FAR const char *fmt, ...)
{
  va_list ap;

  /* Let vsyslog do the real work */

  va_start(ap, fmt);
  vsyslog(LOG_INFO, fmt, ap);
  va_end(ap);
  return OK;
}

/****************************************************************************
 * Name: nsh_tracecallback
 *
 * Description:
 *   This is part of the USB trace logic
 *
 ****************************************************************************/

static int nsh_tracecallback(struct usbtrace_s *trace, void *arg)
{
  usbtrace_trprintf(usbtrace_syslog, trace->event, trace->value);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_usbtrace
 *
 * Description:
 *   The function is called from the nsh_session() to dump USB data to the
 *   SYSLOG device.
 *
 * Input Parameters:
 *   None
 *
 * Returned Values:
 *   None
 *
 ****************************************************************************/

void nsh_usbtrace(void)
{
  usbtrace_enumerate(nsh_tracecallback, NULL);
}

#endif /* CONFIG_NSH_USBDEV_TRACE */
