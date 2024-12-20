/****************************************************************************
 * apps/lte/alt1250/alt1250_usockif.c
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/in.h>
#include <assert.h>

#include "alt1250_dbg.h"
#include "alt1250_usockif.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: write_to_usock
 ****************************************************************************/

static int write_to_usock(int fd, FAR void *buf, size_t sz)
{
  int ret;

  ret = write(fd, buf, sz);
  if (ret < 0)
    {
      ret = -errno;
    }
  else if (ret != sz)
    {
      ret = -ENOSPC;
    }
  else
    {
      ret = OK;
    }

  return ret;
}

/****************************************************************************
 * name: send_dataack
 ****************************************************************************/

static int send_dataack(int fd, uint32_t ackxid, int32_t ackresult,
                        uint16_t valuelen, uint16_t valuelen_nontrunc,
                        FAR uint8_t *value_ptr, FAR uint8_t *buf_ptr)
{
  int ret;
  struct usrsock_message_datareq_ack_s dataack;

  dataack.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  dataack.reqack.head.flags = 0;
  dataack.reqack.head.events = 0;
  dataack.reqack.xid = ackxid;
  dataack.reqack.result = ackresult;

  dataack.valuelen = valuelen;
  dataack.valuelen_nontrunc = valuelen_nontrunc;

  ret = write_to_usock(fd, &dataack, sizeof(dataack));
  if (ret < 0)
    {
      return ret;
    }

  if ((valuelen > 0) && (value_ptr != NULL))
    {
      ret = write_to_usock(fd, value_ptr, valuelen);
      if (ret < 0)
        {
          return ret;
        }
    }

  if ((ackresult > 0) && (buf_ptr != NULL))
    {
      ret = write_to_usock(fd, buf_ptr, ackresult);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

/****************************************************************************
 * name: read_wrapper
 ****************************************************************************/

static int read_wrapper(int fd, FAR void *buf, size_t sz)
{
  int ret;

  ret = read(fd, buf, sz);
  if (ret < 0)
    {
      ret = -errno;
      dbg_alt1250("failed to read usersock: %d\n", errno);
      return ret;
    }

  if (ret != sz)
    {
      dbg_alt1250("unexpected read size: %d expected: %u\n", ret, sz);
      return -EMSGSIZE;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: init_usock_device
 ****************************************************************************/

int init_usock_device(void)
{
  return open(DEV_USRSOCK, O_RDWR);
}

/****************************************************************************
 * name: finalize_usock_device
 ****************************************************************************/

void finalize_usock_device(int fd)
{
  close(fd);
}

/****************************************************************************
 * name: reset_usock_device
 ****************************************************************************/

int reset_usock_device(int fd)
{
  close(fd);
  return open(DEV_USRSOCK, O_RDWR);
}

/****************************************************************************
 * name: usockif_readrequest
 ****************************************************************************/

int usockif_readrequest(int fd, FAR struct usrsock_request_buff_s *buf)
{
  int ret;
  int rsize = 0;
  FAR char *pbuf = NULL;

  /* Read common header */

  ret = read_wrapper(fd, buf, sizeof(struct usrsock_request_common_s));
  if (ret < 0)
    {
      return ret;
    }

  /* Read each request */

  switch (buf->request.head.reqid)
    {
      case USRSOCK_REQUEST_SOCKET:
        rsize = sizeof(struct usrsock_request_socket_s);
        break;
      case USRSOCK_REQUEST_CLOSE:
        rsize = sizeof(struct usrsock_request_close_s);
        break;
      case USRSOCK_REQUEST_CONNECT:
        rsize = sizeof(struct usrsock_request_connect_s);
        break;
      case USRSOCK_REQUEST_SENDTO:
        rsize = sizeof(struct usrsock_request_sendto_s);
        break;
      case USRSOCK_REQUEST_RECVFROM:
        rsize = sizeof(struct usrsock_request_recvfrom_s);
        break;
      case USRSOCK_REQUEST_SETSOCKOPT:
        rsize = sizeof(struct usrsock_request_setsockopt_s);
        break;
      case USRSOCK_REQUEST_GETSOCKOPT:
        rsize = sizeof(struct usrsock_request_getsockopt_s);
        break;
      case USRSOCK_REQUEST_GETSOCKNAME:
        rsize = sizeof(struct usrsock_request_getsockname_s);
        break;
      case USRSOCK_REQUEST_GETPEERNAME:
        rsize = sizeof(struct usrsock_request_getpeername_s);
        break;
      case USRSOCK_REQUEST_BIND:
        rsize = sizeof(struct usrsock_request_bind_s);
        break;
      case USRSOCK_REQUEST_LISTEN:
        rsize = sizeof(struct usrsock_request_listen_s);
        break;
      case USRSOCK_REQUEST_ACCEPT:
        rsize = sizeof(struct usrsock_request_accept_s);
        break;
      case USRSOCK_REQUEST_IOCTL:
        rsize = sizeof(struct usrsock_request_ioctl_s);
        break;
      case USRSOCK_REQUEST_SHUTDOWN:
        rsize = sizeof(struct usrsock_request_shutdown_s);
        break;
      default:
        dbg_alt1250("unexpected request id: %d\n", buf->request.head.reqid);
        return -EINVAL;
        break;
    }

  pbuf = (FAR char *)buf + sizeof(struct usrsock_request_common_s);
  rsize -= sizeof(struct usrsock_request_common_s);

  ret = read_wrapper(fd, pbuf, rsize);
  if (ret < 0)
    {
      return ret;
    }

  return sizeof(struct usrsock_request_common_s) + ret;
}

/****************************************************************************
 * name: usockif_readreqioctl
 ****************************************************************************/

int usockif_readreqioctl(int fd, FAR struct usrsock_request_buff_s *buf)
{
  FAR struct usrsock_request_ioctl_s *req = &buf->request.ioctl_req;
  FAR void *pbuf = &buf->req_ioctl;
  int rsize;

  switch (req->cmd)
    {
      case FIONBIO:
        rsize = sizeof(int);
        break;
      case SIOCLTECMD:
        rsize = sizeof(struct lte_ioctl_data_s);
        break;
      case SIOCSIFFLAGS:
      case SIOCGIFFLAGS:
        rsize = sizeof(struct ifreq);
        break;
      case SIOCDENYINETSOCK:
        rsize = sizeof(uint8_t);
        break;
      case SIOCSMSENSTREP:
      case SIOCSMSGREFID:
      case SIOCSMSSSCA:
        rsize = sizeof(struct lte_smsreq_s);
        break;
      default:
        dbg_alt1250("Unsupported command:0x%08lx\n", req->cmd);
        return -ENOTTY;
        break;
    }

  if (req->arglen != rsize)
    {
      dbg_alt1250("unexpected size: %d, expect: %d\n", req->arglen, rsize);
      ASSERT(0);
      return -EINVAL;
    }

  return read_wrapper(fd, pbuf, rsize);
}

/****************************************************************************
 * name: usockif_readreqaddr
 ****************************************************************************/

int usockif_readreqaddr(int fd, FAR struct sockaddr_storage *addr,
                        size_t sz)
{
  if ((sz != sizeof(struct sockaddr_in)) &&
      (sz != sizeof(struct sockaddr_in6)))
    {
      dbg_alt1250("Invalid addrlen: %u\n", sz);
      return -EINVAL;
    }

  return read_wrapper(fd, addr, sz);
}

/****************************************************************************
 * name: usockif_readreqsendbuf
 ****************************************************************************/

int usockif_readreqsendbuf(int fd, FAR uint8_t *sendbuf, size_t sz)
{
  return read_wrapper(fd, sendbuf, sz);
}

/****************************************************************************
 * name: usockif_readreqoption
 ****************************************************************************/

int usockif_readreqoption(int fd, FAR uint8_t *option, size_t sz)
{
  return read_wrapper(fd, option, sz);
}

/****************************************************************************
 * name: usockif_discard
 ****************************************************************************/

void usockif_discard(int fd, size_t sz)
{
  char dummy;

  /* If the seek is called with the exact size, the seek will
   * result in an error. In order to avoid this, the process of
   * read is performed after seeking the specified size minus one byte.
   */

  if (lseek(fd, sz - 1, SEEK_CUR) >= 0)
    {
      read(fd, &dummy, 1);
    }
}

/****************************************************************************
 * name: usockif_sendack
 ****************************************************************************/

int usockif_sendack(int fd, int32_t usock_result, uint32_t usock_xid,
                    bool inprogress)
{
  struct usrsock_message_req_ack_s ack;

  ack.head.msgid = USRSOCK_MESSAGE_RESPONSE_ACK;
  ack.head.flags = inprogress ? USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS : 0;
  ack.head.events = 0;
  ack.xid = usock_xid;
  ack.result = usock_result;

  return write_to_usock(fd, &ack, sizeof(ack));
}

/****************************************************************************
 * name: usockif_senddataack
 ****************************************************************************/

int usockif_senddataack(int fd, int32_t usock_result, uint32_t usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo)
{
  return send_dataack(fd, usock_xid, usock_result, ackinfo->valuelen,
                      ackinfo->valuelen_nontrunc, ackinfo->value_ptr,
                      ackinfo->buf_ptr);
}

/****************************************************************************
 * name: usockif_sendevent
 ****************************************************************************/

int usockif_sendevent(int fd, int usockid, int event)
{
  struct usrsock_message_socket_event_s msg;

  msg.head.msgid = USRSOCK_MESSAGE_SOCKET_EVENT;
  msg.head.flags = USRSOCK_MESSAGE_FLAG_EVENT;
  msg.usockid = usockid;
  msg.head.events = event;

  return write_to_usock(fd, &msg, sizeof(msg));
}
