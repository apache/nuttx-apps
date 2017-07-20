/****************************************************************************
 * apps/wireless/ieee802154/i8sak/wpanlistener.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "wpanlistener.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static pthread_addr_t wpanlistener_framethread(pthread_addr_t arg);
static pthread_addr_t wpanlistener_eventthread(pthread_addr_t arg);

/***************************************************************************
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

int wpanlistener_setup(FAR struct wpanlistener_s *handle, int fd)
{
  int i;

  /* Initialize the frame receiver allocation pool */

  sq_init(&handle->framereceivers);
  sq_init(&handle->framereceivers_free);
  for (i = 0; i < CONFIG_WPANLISTENER_NFRAMERECEIVERS; i++)
    {
      sq_addlast((FAR sq_entry_t *)&handle->framereceiver_pool[i], &handle->framereceivers_free);
    }

  /* Initialize the frame receiver allocation pool */

  sq_init(&handle->eventreceivers);
  sq_init(&handle->eventreceivers_free);
  for (i = 0; i < CONFIG_WPANLISTENER_NEVENTRECEIVERS; i++)
    {
      sq_addlast((FAR sq_entry_t *)&handle->eventreceiver_pool[i], &handle->eventreceivers_free);
    }

  sem_init(&handle->exclsem, 0, 1);

  handle->is_setup = true;
  handle->fd = fd;
  return OK;
}

/****************************************************************************
 * Name: wpanlistener_start
 *
 * Description:
 *   Starts internal threads to listen for frames and events.
 *
 * Parameters:
 *   handle  - handle to the wpan listener struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int wpanlistener_start(FAR struct wpanlistener_s *handle)
{
  int ret;

  handle->threadrun = true;

  ret = pthread_create(&handle->frame_threadid, NULL, wpanlistener_framethread,
                       (void *)handle);
  if (ret != 0)
    {
      fprintf(stderr, "wpanlistener: failed to start frame thread: %d\n", ret);
      return ret;
    }

  ret = pthread_create(&handle->event_threadid, NULL, wpanlistener_eventthread,
                       (void *)handle);
  if (ret != 0)
    {
      fprintf(stderr, "wpanlistener: failed to start event thread: %d\n", ret);
      return ret;
    }

  return OK;
}

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

int wpanlistener_stop(FAR struct wpanlistener_s *handle)
{
  return -ENOTTY;
}

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
                            FAR void * arg, bool oneshot)
{
  FAR struct wpanlistener_framereceiver_s *receiver;
  int ret;

  /* Get exclusive access to the struct */

  ret = sem_wait(&handle->exclsem);
  if (ret != OK)
    {
      fprintf(stderr, "wpanlistener: failed to add receiver: %d\n", ret);
      return ret;
    }

  /* Allocate a receiver struct from the static pool */

  receiver = (FAR struct wpanlistener_framereceiver_s *)sq_remfirst(
                                                    &handle->framereceivers_free);
  if (receiver == NULL)
    {
      fprintf(stderr, "wpanlistener: failed to add receiver: %d\n", ENOMEM);
      sem_post(&handle->exclsem);
      return -ENOMEM;
    }

  receiver->cb = cb;
  memcpy(&receiver->filter, filter, sizeof(struct wpanlistener_framefilter_s));
  receiver->arg = arg;
  receiver->oneshot = oneshot;

  /* Link the receiver into the list */

  sq_addlast((FAR sq_entry_t *)receiver, &handle->framereceivers);

  sem_post(&handle->exclsem);
  return OK;
}

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
                            FAR void * arg, bool oneshot)
{
  FAR struct wpanlistener_eventreceiver_s *receiver;
  int ret;

  /* Get exclusive access to the struct */

  ret = sem_wait(&handle->exclsem);
  if (ret != OK)
    {
      fprintf(stderr, "wpanlistener: failed to add receiver: %d\n", ret);
      return ret;
    }

  /* Allocate a receiver struct from the static pool */

  receiver = (FAR struct wpanlistener_eventreceiver_s *)sq_remfirst(
                                                    &handle->eventreceivers_free);
  if (receiver == NULL)
    {
      fprintf(stderr, "wpanlistener: failed to add receiver: %d\n", ENOMEM);
      sem_post(&handle->exclsem);
      return -ENOMEM;
    }

  receiver->cb = cb;
  memcpy(&receiver->filter, filter, sizeof(struct wpanlistener_eventfilter_s));
  receiver->arg = arg;
  receiver->oneshot = oneshot;

  /* Link the receiver into the list */

  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers);

  sem_post(&handle->exclsem);

  return OK;
}

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
                                      wpanlistener_framecallback_t cb)
{
  FAR struct wpanlistener_framereceiver_s *receiver;
  int ret;

  /* Get exclusive access to the struct */

  ret = sem_wait(&handle->exclsem);
  if (ret != OK)
    {
      fprintf(stderr, "wpanlistener: failed to remove receiver: %d\n", ret);
      return ret;
    }

  /* Search through frame receivers until either we match the callback, or
   * there is no more receivers to check.
   */

  receiver = (FAR struct wpanlistener_framereceiver_s *)sq_peek(
                &handle->framereceivers);

  while (receiver != NULL)
    {
      /* Check if callback matches */

      if (receiver->cb == cb)
        {
          /* Unlink the receiver from the list */

          sq_rem((FAR sq_entry_t *)receiver, &handle->framereceivers);

          /* Link the receiver back into the free list */

          sq_addlast((FAR sq_entry_t *)receiver, &handle->framereceivers_free);

          sem_post(&handle->exclsem);

          return OK;
        }

      receiver = (FAR struct wpanlistener_framereceiver_s *)sq_next(
                    (FAR sq_entry_t *)receiver);
    }

  sem_post(&handle->exclsem);
  fprintf(stderr, "wpanlistener: failed to remove receiver");
  return ERROR;
}

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
                                      wpanlistener_eventcallback_t cb)
{
  FAR struct wpanlistener_eventreceiver_s *receiver;
  int ret;

  /* Get exclusive access to the struct */

  ret = sem_wait(&handle->exclsem);
  if (ret != OK)
    {
      fprintf(stderr, "wpanlistener: failed to remove receiver: %d\n", ret);
      return ret;
    }

  /* Search through frame receivers until either we match the callback, or
   * there is no more receivers to check.
   */

  receiver = (FAR struct wpanlistener_eventreceiver_s *)sq_peek(
                &handle->eventreceivers);

  while (receiver != NULL)
    {
      /* Check if callback matches */

      if (receiver->cb == cb)
        {
          /* Unlink the receiver from the list */

          sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

          /* Link the receiver back into the free list */

          sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);

          sem_post(&handle->exclsem);

          return OK;
        }

      receiver = (FAR struct wpanlistener_eventreceiver_s *)sq_next(
                    (FAR sq_entry_t *)receiver);
    }

  sem_post(&handle->exclsem);
  fprintf(stderr, "wpanlistener: failed to remove receiver");
  return ERROR;
}

/****************************************************************************
 * handleate Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name : wpanlistener_framethread
 *
 * Description :
 *   Listen for frames received at the MAC layer
 ****************************************************************************/

static pthread_addr_t wpanlistener_framethread(pthread_addr_t arg)
{
  FAR struct wpanlistener_s *handle = (FAR struct wpanlistener_s *)arg;
  FAR struct wpanlistener_framereceiver_s *receiver;
  struct mac802154dev_rxframe_s frame;
  int ret;
  int i;

  while (handle->threadrun)
    {
      ret = read(handle->fd, &frame, sizeof(struct mac802154dev_rxframe_s));
      if (ret != OK)
        {
          return NULL;
        }

      /* Get exclusive access to the struct */

      ret = sem_wait(&handle->exclsem);
      if (ret != OK)
        {
          return NULL;
        }

      printf("Frame Received:\n");
      for (i = 0; i < frame.length; i++)
        {
          printf("%02X", frame.payload[i]);
        }

      printf(" \n");
      fflush(stdout);

      /* Loop through frame receivers and call callbacks for those receivers
       * whose filter matches this frame
       */

      receiver = (FAR struct wpanlistener_framereceiver_s *)sq_peek(
                    &handle->framereceivers);

      while (receiver != NULL)
        {
          /* TODO: When filtering options are figured out. Actually filter packets
           * here. For now, all frames get passed to all receivers.
           */

          receiver->cb(&frame, receiver->arg);

          receiver = (FAR struct wpanlistener_framereceiver_s *)sq_next(
                        (FAR sq_entry_t *)receiver);

          /* Check if the receiver was a one-shot receiver, then throw it out
           * if it is
           */

          if (receiver->oneshot)
            {
              /* Unlink the receiver from the list */

              sq_rem((FAR sq_entry_t *)receiver, &handle->framereceivers);

              /* Link the receiver back into the free list */

              sq_addlast((FAR sq_entry_t *)receiver, &handle->framereceivers_free);
            }
        }
      sem_post(&handle->exclsem);
    }

  return NULL;
}

/****************************************************************************
 * Name : wpanlistener_eventthread
 *
 * Description :
 *   Listen for events from the MAC layer
 ****************************************************************************/

static pthread_addr_t wpanlistener_eventthread(pthread_addr_t arg)
{
  FAR struct wpanlistener_s *handle = (FAR struct wpanlistener_s *)arg;
  FAR struct wpanlistener_eventreceiver_s *receiver;
  struct ieee802154_notif_s notif;
  int ret;

  while (handle->threadrun)
    {
      ret = ioctl(handle->fd, MAC802154IOC_GET_EVENT, (unsigned long)((uintptr_t)&notif));
      if (ret != OK)
        {
          return NULL;
        }

      /* Get exclusive access to the struct */

      ret = sem_wait(&handle->exclsem);
      if (ret != OK)
        {
          return NULL;
        }

      /* Loop through event receivers and call callbacks for those receivers
       * listening for this event type.
       */

      receiver = (FAR struct wpanlistener_eventreceiver_s *)sq_peek(
                    &handle->eventreceivers);

      while (receiver != NULL)
        {
          if (notif.notiftype == IEEE802154_NOTIFY_CONF_DATA &&
              receiver->filter.confevents.data)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_ASSOC &&
              receiver->filter.confevents.assoc)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_DISASSOC &&
              receiver->filter.confevents.disassoc)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_GTS &&
              receiver->filter.confevents.gts)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_RESET &&
              receiver->filter.confevents.reset)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_RXENABLE &&
              receiver->filter.confevents.rxenable)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_SCAN &&
              receiver->filter.confevents.scan)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_START &&
              receiver->filter.confevents.start)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_CONF_POLL &&
              receiver->filter.confevents.poll)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_ASSOC &&
              receiver->filter.indevents.assoc)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_DISASSOC &&
              receiver->filter.indevents.disassoc)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_BEACONNOTIFY &&
              receiver->filter.indevents.beacon)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_GTS &&
              receiver->filter.indevents.gts)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_ORPHAN &&
              receiver->filter.indevents.orphan)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_COMMSTATUS &&
              receiver->filter.indevents.commstatus)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          if (notif.notiftype == IEEE802154_NOTIFY_IND_SYNCLOSS &&
              receiver->filter.indevents.syncloss)
            {
              receiver->cb(&notif, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &handle->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver, &handle->eventreceivers_free);
                }
            }

          receiver = (FAR struct wpanlistener_eventreceiver_s *)sq_next(
                        (FAR sq_entry_t *)receiver);
        }
      sem_post(&handle->exclsem);
    }

  return NULL;
}
