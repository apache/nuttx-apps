/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak_events.c
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "wireless/ieee802154.h"

#include "i8sak.h"
#include "i8sak_events.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name : i8sak_eventthread
 *
 * Description :
 *   Listen for events from the MAC layer
 ****************************************************************************/

static pthread_addr_t i8sak_eventthread(pthread_addr_t arg)
{
  FAR struct i8sak_s *i8sak = (FAR struct i8sak_s *)arg;
  union ieee802154_macarg_u macarg;
#ifdef CONFIG_NET_6LOWPAN
  struct ieee802154_netmac_s netarg;
#endif
  FAR struct i8sak_eventreceiver_s *receiver;
  FAR struct ieee802154_primitive_s *primitive = NULL;
  int ret = OK;

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      macarg.enable = true;
      ioctl(i8sak->fd, MAC802154IOC_ENABLE_EVENTS,
                  (unsigned long)((uintptr_t)&macarg));
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      netarg.u.enable = true;
      strncpy(netarg.ifr_name, i8sak->ifname, IFNAMSIZ);
      ioctl(i8sak->fd, MAC802154IOC_ENABLE_EVENTS,
                  (unsigned long)((uintptr_t)&netarg));
    }
#endif

  while (i8sak->eventlistener_run)
    {
      if (i8sak->mode == I8SAK_MODE_CHAR)
        {
          ret = ioctl(i8sak->fd, MAC802154IOC_GET_EVENT,
                      (unsigned long)((uintptr_t)&macarg));
          primitive = &macarg.primitive;
        }
#ifdef CONFIG_NET_6LOWPAN
      else if (i8sak->mode == I8SAK_MODE_NETIF)
        {
          ret = ioctl(i8sak->fd, MAC802154IOC_GET_EVENT,
                      (unsigned long)((uintptr_t)&netarg));
          primitive = &netarg.u.primitive;
        }
#endif

      if (ret != OK)
        {
          i8sak->eventlistener_run = false;
          continue;
        }

      ret = sem_wait(&i8sak->eventsem);
      if (ret != OK)
        {
          i8sak->eventlistener_run = false;
          continue;
        }

      /* Loop through event receivers and call callbacks for those receivers
       * listening for this event type.
       */

      receiver = (FAR struct i8sak_eventreceiver_s *)sq_peek(
                    &i8sak->eventreceivers);

      while (receiver != NULL)
        {
          if ((primitive->type == IEEE802154_PRIMITIVE_CONF_DATA &&
              receiver->filter.confevents.data) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_ASSOC &&
              receiver->filter.confevents.assoc) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_DISASSOC &&
              receiver->filter.confevents.disassoc) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_GTS &&
              receiver->filter.confevents.gts) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_RESET &&
              receiver->filter.confevents.reset) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_RXENABLE &&
              receiver->filter.confevents.rxenable) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_SCAN &&
              receiver->filter.confevents.scan) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_START &&
              receiver->filter.confevents.start) ||
              (primitive->type == IEEE802154_PRIMITIVE_CONF_POLL &&
              receiver->filter.confevents.poll) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_ASSOC &&
              receiver->filter.indevents.assoc) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_DISASSOC &&
              receiver->filter.indevents.disassoc) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_BEACONNOTIFY &&
              receiver->filter.indevents.beacon) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_GTS &&
              receiver->filter.indevents.gts) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_ORPHAN &&
              receiver->filter.indevents.orphan) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_COMMSTATUS &&
              receiver->filter.indevents.commstatus) ||
              (primitive->type == IEEE802154_PRIMITIVE_IND_SYNCLOSS &&
              receiver->filter.indevents.syncloss))
            {
              receiver->cb(primitive, receiver->arg);

              if (receiver->oneshot)
                {
                  /* Unlink the receiver from the list */

                  sq_rem((FAR sq_entry_t *)receiver, &i8sak->eventreceivers);

                  /* Link the receiver back into the free list */

                  sq_addlast((FAR sq_entry_t *)receiver,
                              &i8sak->eventreceivers_free);
                }
            }

          receiver = (FAR struct i8sak_eventreceiver_s *)sq_next(
                        (FAR sq_entry_t *)receiver);
        }

      sem_post(&i8sak->eventsem);
    }

  if (i8sak->mode == I8SAK_MODE_CHAR)
    {
      macarg.enable = false;
      ioctl(i8sak->fd, MAC802154IOC_ENABLE_EVENTS,
                  (unsigned long)((uintptr_t)&macarg));
    }
#ifdef CONFIG_NET_6LOWPAN
  else if (i8sak->mode == I8SAK_MODE_NETIF)
    {
      netarg.u.enable = false;
      strncpy(netarg.ifr_name, i8sak->ifname, IFNAMSIZ);
      ioctl(i8sak->fd, MAC802154IOC_ENABLE_EVENTS,
                  (unsigned long)((uintptr_t)&netarg));
    }
#endif

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i8sak_eventlistener_start
 *
 * Description:
 *   Starts internal threads to listen for events.
 *
 * Parameters:
 *   i8sak  - handle to the i8sak instance struct
 *
 * Returned Value:
 *   OK on success; a negated errno on failure
 *
 * Assumptions:
 *
 ****************************************************************************/

int i8sak_eventlistener_start(FAR struct i8sak_s *i8sak)
{
  int ret;

  i8sak->eventlistener_run = true;

  ret = pthread_create(&i8sak->eventlistener_threadid,
                       NULL, i8sak_eventthread,
                       (void *)i8sak);
  if (ret != 0)
    {
      fprintf(stderr, "failed to start event thread: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: i8sak_eventlistener_stop
 *
 * Description:
 *   Stops event thread.
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

int i8sak_eventlistener_stop(FAR struct i8sak_s *i8sak)
{
  int ret;
  FAR void *value;

  i8sak->eventlistener_run = false;
  pthread_kill(i8sak->eventlistener_threadid, 2);
  ret = pthread_join(i8sak->eventlistener_threadid, &value);
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: pthread_join() failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Name: i8sak_eventlistener_addreceiver
 *
 * Description:
 *   Add an event receiver.
 *   An event receiver consists of a callback and flags for which events
 *   should be sent to the callback.
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
                                    bool oneshot)
{
  FAR struct i8sak_eventreceiver_s *receiver;
  int ret;

  ret = sem_wait(&i8sak->eventsem);
  if (ret != OK)
    {
      fprintf(stderr, "failed to get exclusive access: %d\n", ret);
      return ret;
    }

  /* Allocate a receiver struct from the static pool */

  receiver = (FAR struct i8sak_eventreceiver_s *)sq_remfirst(
                   &i8sak->eventreceivers_free);
  if (receiver == NULL)
    {
      fprintf(stderr, "failed to add receiver: %d\n", ENOMEM);
      sem_post(&i8sak->eventsem);
      return -ENOMEM;
    }

  receiver->cb = cb;
  memcpy(&receiver->filter, filter, sizeof(struct i8sak_eventfilter_s));
  receiver->arg = i8sak;
  receiver->oneshot = oneshot;

  /* Link the receiver into the list */

  sq_addlast((FAR sq_entry_t *)receiver, &i8sak->eventreceivers);

  sem_post(&i8sak->eventsem);

  return OK;
}

/****************************************************************************
 * Name: i8sak_eventlistener_removereceiver
 *
 * Description:
 *   Removes a event receiver.  Listener will no longer call callback. This
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

int i8sak_eventlistener_removereceiver(FAR struct i8sak_s *i8sak,
                                       i8sak_eventcallback_t cb)
{
  FAR struct i8sak_eventreceiver_s *receiver;
  int ret;

  ret = sem_wait(&i8sak->eventsem);
  if (ret != OK)
    {
      fprintf(stderr, "failed to get exclusive access: %d\n", ret);
      return ret;
    }

  /* Search through frame receivers until either we match the callback, or
   * there is no more receivers to check.
   */

  receiver = (FAR struct i8sak_eventreceiver_s *)sq_peek(
                &i8sak->eventreceivers);

  while (receiver != NULL)
    {
      /* Check if callback matches */

      if (receiver->cb == cb)
        {
          /* Unlink the receiver from the list */

          sq_rem((FAR sq_entry_t *)receiver, &i8sak->eventreceivers);

          /* Link the receiver back into the free list */

          sq_addlast((FAR sq_entry_t *)receiver,
                      &i8sak->eventreceivers_free);

          sem_post(&i8sak->eventsem);

          return OK;
        }

      receiver = (FAR struct i8sak_eventreceiver_s *)sq_next(
                    (FAR sq_entry_t *)receiver);
    }

  sem_post(&i8sak->eventsem);
  fprintf(stderr, "failed to remove receiver");
  return ERROR;
}
