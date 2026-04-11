/****************************************************************************
 * apps/netutils/netlib/netlib_dhcp_ntp.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_NETUTILS_DHCPC

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pthread_mutex_t g_dhcp_ntp_lock = PTHREAD_MUTEX_INITIALIZER;
static FAR char *g_dhcp_ntp_servers;
static netlib_dhcp_ntp_callback_t g_dhcp_ntp_callback;
static FAR void *g_dhcp_ntp_callback_arg;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR char *netlib_dhcp_ntp_dup(FAR const char *ntp_server_list)
{
  if (ntp_server_list == NULL || ntp_server_list[0] == '\0')
    {
      return NULL;
    }

  return strdup(ntp_server_list);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int netlib_set_ntp_servers_from_dhcp(FAR const char *ntp_server_list)
{
  FAR char *new_servers;
  FAR char *notify_servers = NULL;
  FAR char *old_servers;
  netlib_dhcp_ntp_callback_t callback;
  FAR void *arg;
  int ret = OK;

  new_servers = netlib_dhcp_ntp_dup(ntp_server_list);
  if (ntp_server_list != NULL && ntp_server_list[0] != '\0' &&
      new_servers == NULL)
    {
      return -ENOMEM;
    }

  pthread_mutex_lock(&g_dhcp_ntp_lock);

  if ((g_dhcp_ntp_servers == NULL && new_servers == NULL) ||
      (g_dhcp_ntp_servers != NULL && new_servers != NULL &&
       strcmp(g_dhcp_ntp_servers, new_servers) == 0))
    {
      pthread_mutex_unlock(&g_dhcp_ntp_lock);
      free(new_servers);
      return OK;
    }

  callback = g_dhcp_ntp_callback;
  arg = g_dhcp_ntp_callback_arg;

  if (callback != NULL && new_servers != NULL)
    {
      notify_servers = strdup(new_servers);
      if (notify_servers == NULL)
        {
          ret = -ENOMEM;
          goto errout_with_lock;
        }
    }

  old_servers = g_dhcp_ntp_servers;
  g_dhcp_ntp_servers = new_servers;
  pthread_mutex_unlock(&g_dhcp_ntp_lock);

  free(old_servers);

  if (callback != NULL)
    {
      callback(notify_servers, arg);
    }

  free(notify_servers);
  return OK;

errout_with_lock:
  pthread_mutex_unlock(&g_dhcp_ntp_lock);
  free(new_servers);
  return ret;
}

int netlib_register_dhcp_ntp_callback(netlib_dhcp_ntp_callback_t callback,
                                      FAR void *arg)
{
  FAR char *notify_servers = NULL;

  if (callback == NULL)
    {
      return -EINVAL;
    }

  pthread_mutex_lock(&g_dhcp_ntp_lock);

  if (g_dhcp_ntp_callback != NULL &&
      (g_dhcp_ntp_callback != callback || g_dhcp_ntp_callback_arg != arg))
    {
      pthread_mutex_unlock(&g_dhcp_ntp_lock);
      return -EBUSY;
    }

  if (g_dhcp_ntp_servers != NULL)
    {
      notify_servers = strdup(g_dhcp_ntp_servers);
      if (notify_servers == NULL)
        {
          pthread_mutex_unlock(&g_dhcp_ntp_lock);
          return -ENOMEM;
        }
    }

  g_dhcp_ntp_callback = callback;
  g_dhcp_ntp_callback_arg = arg;

  pthread_mutex_unlock(&g_dhcp_ntp_lock);

  callback(notify_servers, arg);
  free(notify_servers);
  return OK;
}

int netlib_unregister_dhcp_ntp_callback(netlib_dhcp_ntp_callback_t callback,
                                        FAR void *arg)
{
  int ret = -ENOENT;

  pthread_mutex_lock(&g_dhcp_ntp_lock);

  if (g_dhcp_ntp_callback == callback && g_dhcp_ntp_callback_arg == arg)
    {
      g_dhcp_ntp_callback = NULL;
      g_dhcp_ntp_callback_arg = NULL;
      ret = OK;
    }

  pthread_mutex_unlock(&g_dhcp_ntp_lock);
  return ret;
}

#endif /* CONFIG_NETUTILS_DHCPC */
