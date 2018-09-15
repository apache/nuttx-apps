/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_events.h
 *
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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

#ifndef __APPS_WIRELESS_IEEE802154_I8SAK_EVENTS_H
#define __APPS_WIRELESS_IEEE802154_I8SAK_EVENTS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <semaphore.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#if !defined(CONFIG_I8SAK_NEVENTRECEIVERS) || CONFIG_I8SAK_NEVENTRECEIVERS <= 0
#  undef CONFIG_I8SAK_NEVENTRECEIVERS
#  define CONFIG_I8SAK_NEVENTRECEIVERS 3
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*i8sak_eventcallback_t) (FAR struct ieee802154_primitive_s *primitive,
                                       FAR void *arg);

struct i8sak_eventfilter_s
{
  struct
  {
    uint32_t assoc        : 1;
    uint32_t disassoc     : 1;
    uint32_t beacon       : 1;
    uint32_t commstatus   : 1;
    uint32_t gts          : 1;
    uint32_t orphan       : 1;
    uint32_t syncloss     : 1;
  } indevents;

  struct {
    uint32_t data         : 1;
    uint32_t assoc        : 1;
    uint32_t disassoc     : 1;
    uint32_t gts          : 1;
    uint32_t commstatus   : 1;
    uint32_t reset        : 1;
    uint32_t rxenable     : 1;
    uint32_t scan         : 1;
    uint32_t start        : 1;
    uint32_t poll         : 1;
  } confevents;
};

struct i8sak_eventreceiver_s
{
  FAR struct i8sak_eventreceiver_s *flink;
  i8sak_eventcallback_t cb;
  struct i8sak_eventfilter_s filter;
  FAR void *arg;
  bool oneshot;
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Forware Reference */

struct i8sak_s;

/****************************************************************************
 * Name: i8sak_eventlistener_start
 *
 * Description:
 *   Starts internal threads to listen for events.
 *
 * Parameters:
 *   handle - handle to the i8sak instance struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int i8sak_eventlistener_start(FAR struct i8sak_s *i8sak);

/****************************************************************************
 * Name: i8sak_eventlistener_stop
 *
 * Description:
 *   Stops internal threads.
 *
 * Parameters
 *   handle - handle to the i8sak instance struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int i8sak_eventlistener_stop(FAR struct i8sak_s *i8sak);

/****************************************************************************
 * Name: i8sak_eventlistener_addreceiver
 *
 * Description:
 *   Add an event receiver.  An event receiver consists of a callback and flags
 *   for which events should be sent to the callback.
 *
 * Parameters:
 *   handle   - handle to the i8sak instance struct
 *   cb       - callback to be called on reception of notification event from
 *              MAC layer that matches one of the masked events
 *   filter   - struct containing event mask bits to indicate whether event
 *              type should trigger callback
 *   arg      - user specified argument to send to the callback
 *   oneshot  - whether the receiver is automatically unregistered after the
 *              first notification
 *
 * Returned Value:
 *   OK if successful; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int i8sak_eventlistener_addreceiver(FAR struct i8sak_s *i8sak,
                            i8sak_eventcallback_t cb,
                            FAR struct i8sak_eventfilter_s *filter,
                            bool oneshot);

/****************************************************************************
 * Name: i8sak_eventlistener_removereceiver
 *
 * Description:
 *   Removes an event receiver.  Listener will no longer call callback. This
 *   function finds the first event receiver with the provided callback.
 *
 * Parameters:
 *   handle - handle to the i8sak instance struct
 *   cb     - callback function to search for.
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int i8sak_eventlistener_removereceiver(FAR struct i8sak_s *handle,
                                       i8sak_eventcallback_t cb);

#endif /* __APPS_WIRELESS_IEEE802154_I8SAK_EVENTS_H */
