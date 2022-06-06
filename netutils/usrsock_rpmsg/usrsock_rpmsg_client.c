/****************************************************************************
 * apps/netutils/usrsock_rpmsg/usrsock_rpmsg_client.c
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

#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>

#include <nuttx/fs/fs.h>
#include <nuttx/net/dns.h>
#include <nuttx/rptun/openamp.h>

#include "usrsock_rpmsg.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct usrsock_rpmsg_s
{
  struct rpmsg_endpoint ept;
  const char           *cpuname;
  pid_t                 pid;
  sem_t                 sem;
  struct file           file;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int usrsock_rpmsg_dns_handler(struct rpmsg_endpoint *ept, void *data,
                                     size_t len, uint32_t src, void *priv);
static int usrsock_rpmsg_default_handler(struct rpmsg_endpoint *ept,
                                         void *data, size_t len,
                                         uint32_t src, void *priv_);

static void usrsock_rpmsg_device_created(struct rpmsg_device *rdev,
                                         void *priv_);
static void usrsock_rpmsg_device_destroy(struct rpmsg_device *rdev,
                                         void *priv_);
static int usrsock_rpmsg_ept_cb(struct rpmsg_endpoint *ept, void *data,
                                size_t len, uint32_t src, void *priv);

static void usrsock_rpmsg_ns_unbind(struct rpmsg_endpoint *ept);

#ifdef CONFIG_NETDB_DNSCLIENT
static int usrsock_rpmsg_send_dns_request(void *arg,
                                          struct sockaddr *addr,
                                          socklen_t addrlen);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usrsock_rpmsg_device_created(struct rpmsg_device *rdev,
                                         void *priv_)
{
  struct usrsock_rpmsg_s *priv = priv_;
  int ret;

  if (!strcmp(priv->cpuname, rpmsg_get_cpuname(rdev)))
    {
      priv->ept.priv = priv;

      ret = rpmsg_create_ept(&priv->ept, rdev, USRSOCK_RPMSG_EPT_NAME,
                             RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
                             usrsock_rpmsg_ept_cb, usrsock_rpmsg_ns_unbind);
      if (ret == 0)
        {
#ifdef CONFIG_NETDB_DNSCLIENT
          dns_register_notify(usrsock_rpmsg_send_dns_request, priv);
#endif
          sem_post(&priv->sem);
        }
    }
}

static void usrsock_rpmsg_device_destroy(struct rpmsg_device *rdev,
                                         void *priv_)
{
  struct usrsock_rpmsg_s *priv = priv_;

  if (!strcmp(priv->cpuname, rpmsg_get_cpuname(rdev)))
    {
      rpmsg_destroy_ept(&priv->ept);
      kill(priv->pid, SIGUSR1);
    }
}

static int usrsock_rpmsg_dns_handler(struct rpmsg_endpoint *ept, void *data,
                                     size_t len, uint32_t src, void *priv)
{
#ifdef CONFIG_NETDB_DNSCLIENT
  struct usrsock_rpmsg_dns_event_s *dns = data;

  dns_add_nameserver((struct sockaddr *)(dns + 1), dns->addrlen);
#endif

  return 0;
}

static int usrsock_rpmsg_default_handler(struct rpmsg_endpoint *ept,
                                         void *data, size_t len,
                                         uint32_t src, void *priv_)
{
  struct usrsock_rpmsg_s *priv = priv_;

  while (len > 0)
    {
      ssize_t ret = file_write(&priv->file, data, len);
      if (ret < 0)
        {
          return ret;
        }

      data += ret;
      len  -= ret;
    }

  return 0;
}

static int usrsock_rpmsg_ept_cb(struct rpmsg_endpoint *ept, void *data,
                                size_t len, uint32_t src, void *priv)
{
  struct usrsock_message_common_s *common = data;
  int ret;

  switch (common->msgid)
    {
      case USRSOCK_RPMSG_DNS_EVENT:
        ret = usrsock_rpmsg_dns_handler(ept, data, len, src, priv);
        break;
      default:
        ret = usrsock_rpmsg_default_handler(ept, data, len, src, priv);
        break;
    }

  return ret;
}

#ifdef CONFIG_NETDB_DNSCLIENT
static int usrsock_rpmsg_send_dns_request(void *arg,
                                          struct sockaddr *addr,
                                          socklen_t addrlen)
{
  struct usrsock_rpmsg_s *priv = arg;
  struct rpmsg_endpoint *ept = &priv->ept;
  struct usrsock_rpmsg_dns_request_s *dns;
  uint32_t len;

  dns = rpmsg_get_tx_payload_buffer(ept, &len, true);

  dns->head.reqid = USRSOCK_RPMSG_DNS_REQUEST;
  dns->head.xid = 0;
  dns->head.reserved = 0;
  dns->addrlen = addrlen;
  memcpy(dns + 1, addr, addrlen);

  return rpmsg_send_nocopy(ept, dns, sizeof(*dns) + addrlen);
}
#endif

static void usrsock_rpmsg_ns_unbind(struct rpmsg_endpoint *ept)
{
#ifdef CONFIG_NETDB_DNSCLIENT
  dns_unregister_notify(usrsock_rpmsg_send_dns_request, ept->priv);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct usrsock_rpmsg_s priv =
  {
  };

  int ret;

  if (argv[1] == NULL)
    {
      return -EINVAL;
    }

  priv.cpuname = argv[1];
  priv.pid = getpid();

  sem_init(&priv.sem, 0, 0);
  sem_setprotocol(&priv.sem, SEM_PRIO_NONE);

  sigrelse(SIGUSR1);

  ret = rpmsg_register_callback(&priv,
                                usrsock_rpmsg_device_created,
                                usrsock_rpmsg_device_destroy,
                                NULL,
                                NULL);
  if (ret < 0)
    {
      goto destroy_sem;
    }

  while (1)
    {
      /* Wait until the rpmsg channel is ready */

      do
        {
          ret = sem_wait(&priv.sem);
          if (ret < 0)
            {
              ret = -errno;
            }
        }
      while (ret == -EINTR);

      if (ret < 0)
        {
          goto unregister_callback;
        }

      /* Open the kernel channel */

      ret = file_open(&priv.file, "/dev/usrsock", O_RDWR);
      if (ret < 0)
        {
          ret = -errno;
          goto destroy_ept;
        }

      /* Forward the packet from kernel to remote */

      while (1)
        {
          struct pollfd pfd;
          void  *buf;
          uint32_t len;

          /* Wait the packet ready */

          pfd.ptr = &priv.file;
          pfd.events = POLLIN | POLLFILE;
          ret = poll(&pfd, 1, -1);
          if (ret < 0)
            {
              ret = -errno;
              break;
            }

          /* Read the packet from kernel */

          buf = rpmsg_get_tx_payload_buffer(&priv.ept, &len, true);
          if (!buf)
            {
              ret = -ENOMEM;
              break;
            }

          ret = file_read(&priv.file, buf, len);
          if (ret < 0)
            {
              break;
            }

          /* Send the packet to remote */

          ret = rpmsg_send_nocopy(&priv.ept, buf, ret);
          if (ret < 0)
            {
              break;
            }
        }

      /* Reclaim the resource */

      file_close(&priv.file);

      if (is_rpmsg_ept_ready(&priv.ept))
        {
          goto destroy_ept;
        }

      /* The remote side crash, loop to wait it restore */
    }

destroy_ept:
  rpmsg_destroy_ept(&priv.ept);

unregister_callback:
  rpmsg_unregister_callback(&priv,
                            usrsock_rpmsg_device_created,
                            usrsock_rpmsg_device_destroy,
                            NULL,
                            NULL);
destroy_sem:
  sem_destroy(&priv.sem);
  return ret;
}
