/****************************************************************************
 * apps/wireless/ieee802154/i8sak/wpanlistener.h
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

#ifndef __APPS_WIRELESS_IEEE802154_I8SAK_WPANLISTENER_H
#define __APPS_WIRELESS_IEEE802154_I8SAK_WPANLISTENER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <semaphore.h>

#include <nuttx/wireless/ieee802154/ieee802154_ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#if !defined(CONFIG_WPANLISTENER_NFRAMERECEIVERS) || CONFIG_WPANLISTENER_NFRAMERECEIVERS <= 0
#  undef CONFIG_WPANLISTENER_NFRAMERECEIVERS
#  define CONFIG_WPANLISTENER_NFRAMERECEIVERS 3
#endif

#if !defined(CONFIG_WPANLISTENER_NEVENTRECEIVERS) || CONFIG_WPANLISTENER_NEVENTRECEIVERS <= 0
#  undef CONFIG_WPANLISTENER_NEVENTRECEIVERS
#  define CONFIG_WPANLISTENER_NEVENTRECEIVERS 3
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*wpanlistener_framecallback_t)
               (FAR struct mac802154dev_rxframe_s *frame, FAR void *arg);

typedef void (*wpanlistener_eventcallback_t) (FAR struct ieee802154_notif_s *notif,
                                              FAR void *arg);

struct wpanlistener_framefilter_s
{
  /* Frame filtering settings here */
  bool acceptall;

};

struct wpanlistener_eventfilter_s
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

struct wpanlistener_framereceiver_s
{
  FAR struct wpanlistener_framereceiver_s *flink;
  wpanlistener_framecallback_t cb;
  struct wpanlistener_framefilter_s filter;
  FAR void *arg;
  bool oneshot;
};

struct wpanlistener_eventreceiver_s
{
  FAR struct wpanlistener_eventreceiver_s *flink;
  wpanlistener_eventcallback_t cb;
  struct wpanlistener_eventfilter_s filter;
  FAR void *arg;
  bool oneshot;
};

struct wpanlistener_s
{
  sem_t exclsem;

  int fd;

  bool threadrun  : 1;
  bool is_setup   : 1;
  bool is_started : 1;

  pthread_t frame_threadid;
  pthread_t event_threadid;

  sq_queue_t eventreceivers;
  sq_queue_t framereceivers;

  sq_queue_t eventreceivers_free;
  sq_queue_t framereceivers_free;

  struct wpanlistener_framereceiver_s
            framereceiver_pool[CONFIG_WPANLISTENER_NFRAMERECEIVERS];

  struct wpanlistener_eventreceiver_s
            eventreceiver_pool[CONFIG_WPANLISTENER_NEVENTRECEIVERS];
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wpanlistener_setup
 *
 * Description:
 *   Initializes the internal struct
 *
 * Parameters:
 *   handle - handle to the wpan listener struct to initialize
 *   fd     - file descriptor to access device
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_setup(FAR struct wpanlistener_s *handle, int fd);

/****************************************************************************
 * Name: wpanlistener_start
 *
 * Description:
 *   Starts internal threads to listen for frames and events.
 *
 * Parameters:
 *   handle - handle to the wpan listener struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_start(FAR struct wpanlistener_s *handle);

/****************************************************************************
 * Name: wpanlistener_stop
 *
 * Description:
 *   Stops internal threads.
 *
 * Parameters:
 *   handle - handle to the wpan listener struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_stop(FAR struct wpanlistener_s *handle);

/****************************************************************************
 * Name: wpanlistener_add_framereceiver
 *
 * Description:
 *   Add a frame receiver.  A frame receiver consists of filter settings for
 *   determining which frames should be passed to the callback, and the corresponding
 *   callback.
 *
 * Parameters:
 *   handle   - handle to the wpan listener struct
 *   callback - callback to be called on reception of frame matching filter
 *                 settings
 *   filter   - struct containing settings for filtering frames
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

int wpanlistener_add_framereceiver(FAR struct wpanlistener_s *handle,
                            wpanlistener_framecallback_t cb,
                            FAR struct wpanlistener_framefilter_s *filter,
                            FAR void * arg, bool oneshot);

/****************************************************************************
 * Name: wpanlistener_add_eventreceiver
 *
 * Description:
 *   Add an event receiver.  An event receiver consists of a callback and flags
 *   for which events should be sent to the callback.
 *
 * Parameters:
 *   handle   - handle to the wpan listener struct
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

int wpanlistener_add_eventreceiver(FAR struct wpanlistener_s *handle,
                            wpanlistener_eventcallback_t cb,
                            FAR struct wpanlistener_eventfilter_s *filter,
                            FAR void * arg, bool oneshot);

/****************************************************************************
 * Name: wpanlistener_remove_framereceiver
 *
 * Description:
 *   Removes a frame receiver.  Listener will no longer call callback. This
 *   function finds the first frame receiver with the provided callback.
 *
 * Parameters:
 *   handle - handle to the wpan listener struct
 *   cb     - callback function to search for.
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_remove_framereceiver(FAR struct wpanlistener_s *handle,
                                      wpanlistener_framecallback_t cb);

/****************************************************************************
 * Name: wpanlistener_remove_eventreceiver
 *
 * Description:
 *   Removes a event receiver.  Listener will no longer call callback. This
 *   function finds the first event receiver with the provided callback.
 *
 * Parameters:
 *   handle - handle to the wpan listener struct
 *   cb     - callback function to search for.
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_remove_eventreceiver(FAR struct wpanlistener_s *handle,
                                      wpanlistener_eventcallback_t cb);

#endif /* __APPS_WIRELESS_IEEE802154_I8SAK_WPANLISTENER_H */
